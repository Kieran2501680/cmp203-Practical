// includes all the basic structures and most standard library containers used throughout Skateboard
#include "sktbdpch.h"

#include "TutorialScene.h"

#include "Skateboard/Application.h"
#include "Skateboard/Platform.h"
#include "Skateboard/Assets/AssetManager.h"

TutorialScene::TutorialScene(const std::string& name): 
	Scene(name)
{
	Renderer.Init();
	// Disable lighting, until we have a light in the scene
	Renderer.UnsetPipelineFlags(CMP203::LIT);
}

void TutorialScene::OnHandleInput(Skateboard::TimeManager* time)
{
	Scene::OnHandleInput(time);
}

void TutorialScene::OnUpdate(Skateboard::TimeManager* time)
{
	Scene::OnUpdate(time);
}

void TutorialScene::OnRender()
{

}

void TutorialScene::OnEvent(Event& e)
{
	EventDispatcher Dispatcher(e);
	Dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& e) -> bool {
		Renderer.OnResize(e.GetWidth(), e.GetHeight()); return false; });
}

void TutorialScene::OnImGuiRender()
{
	ImGui::Begin("ImGui");// Creates new ImGui window

	ImGui::Text("Hello CMP203!");
	ImGui::Text("FPS: %f", Skateboard::Platform::GetTimeManager()->FPS());
	ImGui::Text("Mouse position X: %d, Y: %d", Input::GetMousePos().x, Input::GetMousePos().y);
	// If the checkbox is clicked, toggle the wireframe mode
	if (ImGui::Checkbox("wireframe", &bWireframe))
	{
		if (bWireframe)
			Renderer.SetPipelineFlags(CMP203::PipelineFlags::WIREFRAME);
		else
			Renderer.UnsetPipelineFlags(CMP203::PipelineFlags::WIREFRAME);

	}
	ImGui::End();
}
