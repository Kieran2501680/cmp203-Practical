#pragma once
#include "Skateboard/Scene/Scene.h"
#include "Skateboard/ComponentSystems/Systems/BaseSystem.h"

#include "CMP203/Renderer203.h"

class TutorialScene : public Skateboard::Scene {
public:
	explicit TutorialScene(const std::string& name);

	TutorialScene() = delete;

	virtual void OnHandleInput(Skateboard::TimeManager* time) override;
	virtual void OnUpdate(Skateboard::TimeManager* time) override;
	virtual void OnRender() override;
	virtual void OnEvent(Event& e) override;
	virtual void OnImGuiRender() override;

private:
	bool bWireframe = false;
	CMP203::Renderer203 Renderer;
};
