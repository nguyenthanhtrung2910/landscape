# 1. Состав собственной разработки и внешних технологий

## Какая версия и редакция UNIGINE использована?

Использован UNIGINE SDK 2 Community версии 2.22.0.1. Версия UnigineEditor — 2.22.0.0.

## Какие языки и собственные скрипты отвечали за генерацию?

Генерация пары «изображение — маска» реализована на C++ в компоненте `ImagesGenerator`:
[imagegenerator.cpp](components/imagegenerator.cpp#L12-L139).

# 2. Алгоритм разметки и синхронизация

## Как создавались картина и её белая маска?

Сначала `ImagesGenerator` создаёт цветовую текстуру на GPU.
`renderTexture2D()` рендерит сцену с камеры `target_player` в эту текстуру.
Вызов `renderTexture2D()` отправляет команды в очередь GPU и возвращает
управление CPU без ожидания готового кадра. Кадр становится доступен позже,
когда GPU завершает рендеринг.

Если вызвать рендер только в кадре сохранения, готового изображения сразу
не будет. Можно отправить команду заранее, но интервал `capture_frequency`
можно менять, а задержка GPU зависит от нагрузки. Поэтому мы непрерывно
вызываем команду рендеринга, и каждая команда сохранения сохраняет
любой готовый кадр на GPU.

Перед первым сохранением компонент рендерит `warmup_frames` кадров без записи:
некоторым эффектам нужны накопленные данные предыдущих кадров, которых
в начале рендеринга ещё нет. После прогрева рендер продолжается каждый кадр,
поэтому эти данные сохраняются между записями и повторный прогрев не нужен.

Полная функция [`ImagesGenerator::update()`](components/imagegenerator.cpp#L42-L103):

```cpp
void ImagesGenerator::update()
{
    // Manual capture mode with warmup
    if (m_state == STATE_WARMUP)
    {
        const ivec2 target_size = size.get();
        // First warmup frame: disable velocity buffer to avoid temporal artifacts
        if (m_warmupCount == 0)
        {
            m_viewport->appendSkipFlags(Viewport::SKIP_VELOCITY_BUFFER);
            m_viewport->renderTexture2D(
                m_targetPlayer->getCamera(),
                m_colorTexture,
                target_size.x,
                target_size.y);
            m_viewport->removeSkipFlags(Viewport::SKIP_VELOCITY_BUFFER);
        }
        else
        {
            // Subsequent warmup frames: disable exposure adaptation for stability
            float exposure_adaptation = Render::getExposureAdaptation();
            Render::setExposureAdaptation(0.0f);
            m_viewport->renderTexture2D(
                m_targetPlayer->getCamera(),
                m_colorTexture,
                target_size.x,
                target_size.y);
            Render::setExposureAdaptation(exposure_adaptation);
        }

        ++m_warmupCount;
        if (m_warmupCount == warmup_frames.get())
            m_state = STATE_CAPTURING;
        return;
    }
    if (m_state == STATE_CAPTURING)
    {
        const ivec2 target_size = size.get();
        if (m_framesUntilCapture <= 1)
        {
            m_activeCaptureIndex = ++m_captureIndex;
            m_viewport->renderTexture2D(
                m_targetPlayer->getCamera(),
                m_colorTexture,
                target_size.x,
                target_size.y);
            saveTextureAsync(
                m_colorTexture,
                "unigine_dataset_test_1/image_%d.jpg",
                m_activeCaptureIndex);
            m_framesUntilCapture = capture_frequency.get();
        }
        else
        {
            m_viewport->renderTexture2D(
                m_targetPlayer->getCamera(),
                m_colorTexture,
                target_size.x,
                target_size.y);
            --m_framesUntilCapture;
        }
    }
}
```

Перенос изображения с GPU на CPU и сохранение файла на диск выполняются
асинхронно относительно основного игрового потока. Они происходят в 
отдельном потоке и не блокируют основной игровой поток.

Код сохранения:

```cpp
void ImagesGenerator::saveTextureAsync(
    const TexturePtr &texture, const char *pattern, int capture_index)
{
    if (!texture)
        return;

    Render::asyncTransferTextureToImage(
        nullptr,
        MakeCallback([capture_index, pattern](ImagePtr image)
                     {
				if (image->getFormat() != Image::FORMAT_RGB8)
					image->convertToFormat(Image::FORMAT_RGB8);
				if (!Render::isFlipped())
					image->flipY();

				String full_name = String::format(pattern, capture_index);
				image->save(full_name.get()); }),
        texture);
}
```

Для создания маски используется отдельный проход рендеринга UNIGINE — [Auxiliary pass](https://developer.unigine.com/en/docs/latest/principles/render/sequence/#auxiliary).
Проход рендеринга (Render Pass) — это отдельный этап визуализации, на котором 
GPU считывает данные из входных буферов и при необходимости изменяет их или 
записывает результаты в новые буферы. Один кадр может состоять из нескольких таких 
этапов.

У материалов танков включён Auxiliary pass, а Auxiliary Color задан белым.
Поэтому в дополнительной текстуре рендеринга пиксели, соответствующие танкам,
получают белый цвет. Эта текстура используется как маска.

При создании компонента его viewport регистрирует `captureAuxiliary()` как
обработчик события после отрисовки Auxiliary decals. UNIGINE вызывает его
во время рендеринга, когда дополнительная текстура уже доступна:

```cpp
m_viewport->getEventEndAuxiliaryDecals().connect(
    this, &ImagesGenerator::captureAuxiliary);
```

Обработчик получает текстуру Auxiliary через `Renderer::getTextureAuxiliary()`
и передаёт её на сохранение под тем же номером, что и цветное изображение:

```cpp
void ImagesGenerator::captureAuxiliary()
{
    if (m_framesUntilCapture <= 1)
    {
        TexturePtr auxiliaryTexture = Renderer::getTextureAuxiliary();
        if (!auxiliaryTexture)
        {
            Log::warning("ImagesGenerator: auxiliary texture is unavailable.\n");
            return;
        }
        saveTextureAsync(
            auxiliaryTexture,
            "unigine_dataset_test_1/mask_%d.jpg",
            m_activeCaptureIndex);
    }
}
```

## Совпадают ли камера, разрешение, кадр симуляции и правила перекрытия?

Да. RGB и маска создаются в одном viewport за один вызов `renderTexture2D()`
с камерой `target_player`. Поэтому матрицы камеры, состояние сцены и
запрашиваемое разрешение (`size`) у них общие.

# 3. Cмысл рамок и контроль качества

## Маска показывает только видимые пиксели машины или также скрытые за зданиями/деревьями части?

Маска показывает только видимые пиксели техники. Части, закрытые зданиями,
деревьями или другими объектами, отсекаются буфером глубины и не попадают
в результат рендеринга маски

## Учитываются ли прозрачные материалы, тени, антиалиасинг?

<table>
  <tr>
    <th>Тип</th>
    <th>Тени</th>
    <th>Антиалиасинг</th>
    <th>Прозрачные материалы</th>
  </tr>
  <tr>
    <td>Изображение</td>
    <td colspan="3">Да, конечно, обеспечивается UNIGINE движком</td>
  </tr>
  <tr>
    <td>Маска</td>
    <td>Нет</td>
    <td>Да</td>
    <td>У текущих танков нет прозначных поверхностей, нужно отдельным тестом проверить</td>
  </tr>
</table>

# 4. Точность аннотаций и устранение ошибок компонентной конвертации.

## Почему маски сохранены в JPEG, каковы качество сжатия, цветовое пространство и исходная разрядность?

Ну не знаю, можно всегда сохранять как PNG. Изображение перед сохранением
приводится к формату `Image::FORMAT_RGB8`: три канала — красный, зелёный и синий.
Значение каждого канала — целое число от 0 до 255 (8 бит, или 1 байт).
Один пиксель занимает 24 бита (3 байта) до сжатия JPEG.

# 5. Объекты сцены и сторонняя интеллектуальная собственность
## Какие 3D-модели техники и окружающей среды использованы, сколько уникальных моделей, кто их правообладатель?
Модели зданий взяты из дополнения UNIGINE Add-On Store:
[здания](https://store.unigine.com/en/add-on/4e29ae10-c2ad-4dc7-83bf-10e2092ce73e/description).
Модели деревьев взяты из дополнения UNIGINE Add-On Store:
[деревья](https://store.unigine.com/en/add-on/052ccbe9-ba94-4050-8f19-1d11e30b00e2/description).
Модели бронетехники взяты со Sketchfab:

- M1A1 Abrams:
  [Sketchfab](https://sketchfab.com/3d-models/m1a1-abrams-3f72a1ca8bc742109b89b04d5a078977).
- M163A2 VADS:
  [Sketchfab](https://sketchfab.com/3d-models/m163a2-vads-b519c7ac1af34d5a970b96d59ca0ab1a).
- BRM-1:
  [Sketchfab](https://sketchfab.com/3d-models/brm-1-921ada367c634e3da276c05df30627be).

# 6. Отсутствие утечки между выборками
## Что именно обозначает номер image_N в test_2: время, шаг камеры, независимая генерация? 
Картины сохранены после того как исползователь нажимает кнопку на интерфейсе, 
периодически через `capture_frequency` кадров. В `image_N`, N - это просто нумерация
картины.
## Есть ли пропуски/повторы траектории, циклы движения?
Камера закреплёна на дроне и картины снимаются с этой камерой. Для набора данных с 
одним танком, дрон летает по спиральной траектории (helix, параметры у него можно
изменить) с танком в центре.
А для сцены с многими танками нет сценарий движения дрона пока, дрон управляется клавиатурой.
Ты про какой набор спрашиваешь? Нужен набор от конкретной сценарии движения дрона или что-то другое, я переделаю.

# 7. Достоверная оценка обобщения
## Сколько независимых сцен содержит test_2? Можно ли получить дополнительные сцены для отдельного val/test? 
Вопрос не понятно. Что такое test_2? Новые независимые наборы всегда можно создать 
изменением сценарии движения дрона или среды вокруг танка.

# 8. Получение изображений

## Тип камеры, FOV/фокусное расстояние, высота, поза, clipping planes, частота сохранения, движение камеры и расстояния до объектов?

Используется перспективная камера с полем зрения (FOV) 60°.
Ближняя плоскость отсечения — 0,3 единицы, дальняя — 100 000 единиц.
Частота сохранения задаётся параметром `capture_frequency`.
Высота, положение и ориентация камеры, а также расстояния до объектов
зависят от её движения и меняются во время съёмки.

# 9. Вариативность синтетических данных

## Какие изменения освещения, погоды, времени суток, размещения объектов действительно реализованы? Случайные ли они; какие распределения и seed?
Размещение танков случайное и имеет свой seed. [Mesh Clutter](https://developer.unigine.com/en/docs/future/objects/objects/mesh_clutter/?implementationLanguage=cpp).
Освещение, погода, время суток пока не реализуются.