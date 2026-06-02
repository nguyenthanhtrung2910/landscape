#pragma once

#include <UnigineComponentSystem.h>
#include <UniginePlayers.h>
#include <UnigineTextures.h>
#include <UnigineViewport.h>

class ImagesGenerator : public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(ImagesGenerator, Unigine::ComponentBase);

	COMPONENT_INIT(init);
	COMPONENT_UPDATE(update);
	COMPONENT_SHUTDOWN(shutdown);
	PROP_PARAM(Node, target_player);
	PROP_PARAM(IVec2, size, Unigine::Math::ivec2(1920, 1080));
	PROP_PARAM(Int, capture_frequency, 30);
	PROP_PARAM(Int, warmup_frames, 10);

private:
	void init();
	void shutdown();
	void update();
	void captureAuxiliary();
	void saveTextureAsync(const Unigine::TexturePtr &texture, const char *pattern, int capture_index);
	void startCapture();
	void stopCapture();

private:
	enum State
	{
		STATE_IDLE,
		STATE_WARMUP,
		STATE_CAPTURING
	};

	Unigine::ViewportPtr m_viewport{};
	Unigine::TexturePtr m_colorTexture{};
	Unigine::PlayerPtr m_targetPlayer{};
	int m_framesUntilCapture{0};
	int m_captureIndex{0};
	int m_activeCaptureIndex{0};

	State m_state{STATE_IDLE};
	int m_warmupCount{0};
};
