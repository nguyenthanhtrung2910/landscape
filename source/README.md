# Состав собственной разработки и внешних технологий

## Какая версия и редакция UNIGINE использована?

Использован UNIGINE SDK 2 Community версии 2.22.0.1. Версия UnigineEditor — 2.22.0.0.

## Какие языки и собственные скрипты отвечали за генерацию?

Генерация пары «изображение — маска» реализована на C++ в компоненте `ImagesGenerator`:
[imagegenerator.cpp](components/imagegenerator.cpp#L12-L139).

# Алгоритм разметки и синхронизация

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

Для создания маски используется отдельный проход рендеринга UNIGINE — [Auxiliary pass][aux].
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

# Cмысл рамок и контроль качества

## Маска показывает только видимые пиксели машины или также скрытые за 
зданиями/деревьями части?
=====================================================

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

# Точность аннотаций и устранение ошибок компонентной конвертации.

## Почему маски сохранены в JPEG, каковы качество сжатия, цветовое пространство и исходная разрядность?
=====================================================

Ну не знаю, можно всегда сохранять как PNG. Изображение перед сохранением
приводится к формату `Image::FORMAT_RGB8`: три канала — красный, зелёный и синий.
Значение каждого канала — целое число от 0 до 255 (8 бит, или 1 байт).
Один пиксель занимает 24 бита (3 байта) до сжатия JPEG.

# Объекты сцены и сторонняя интеллектуальная собственность
