#pragma once
#include "Skateboard/Scene/Scene.h"

//You can use cas to skateboard implementation by adding platform relevant headers directly;
//For D3D you mostly need information about the context;
#include "Windows/WindowsPlatform/DirectX12/Graphics/RHI/D3DGraphicsContext.h"
#include "Windows/WindowsPlatform/DirectX12/Graphics/Resources/D3DBuffer.h"
#include "Windows/WindowsPlatform/DirectX12/Graphics/Resources/D3DView.h"
#include "Windows/WindowsPlatform/DirectX12/Graphics/Resources/D3DPipeline.h"

#include "BindlessRootSignature.h"
#include "Triangle/Triangle.h"
#include "LightsAndShadows/LightAndShadow.h"
#include "PostProcess/ChromaticAberration/ChAberrEffect.h"

using namespace Microsoft::WRL;

class D3DScene : public Skateboard::Scene {
public:
	explicit D3DScene(const std::string& name);

	D3DScene() = delete;

	~D3DScene();

	virtual void OnHandleInput(Skateboard::TimeManager* time) override;
	virtual void OnUpdate(Skateboard::TimeManager* time) override;
	virtual void OnRender() override;
	virtual void OnEvent(Event& e) override;
	virtual void OnImGuiRender() override;
private:

	Skateboard::PerspectiveCamera m_camera;

	Skateboard::SamplerRef DefaultSampler;

	ChromaticAberrationEffect ChromaAberration;
};