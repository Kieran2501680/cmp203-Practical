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

	DrawQuad(float3(1.f, 1.f, 0.f));
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
	Renderer.Begin();

	Renderer.DrawVertices(m_vertices.data(), m_vertices.size(), m_indicies.data(), m_indicies.size());

	Renderer.End();
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
	ImGui::Text("Verticies: %i", m_vertices.size());
	ImGui::Text("Indicies: %i", m_indicies.size());
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

void TutorialScene::DrawTriangles(float3 colour)
{
	std::vector<float3> positions = {
		{-1.f, 1.f, 0.f},
		{-1.f, -1.f, 0.f},
		{1.f, -1.f, 0.f}
	};

	//loop 3 times for each vertex

	for (int i = 0; i < 3; i++) {

	}
}

void TutorialScene::DrawQuad(float3 colour)
{
	std::vector<CMP203::Vertex> localVertices;

	std::vector<float3> positions = {
		{-1.f, 1.f, 0.f},
		{-1.f, -1.f, 0.f},
		{1.f, -1.f, 0.f},
		{1.f, 1.f, 0.f}
	};

	for (int i = 0; i < 4; i++) {
		CMP203::Vertex vert{positions[i]};
		vert.Colour = colour;

		localVertices.push_back(vert);
	}

	int vectStart = m_vertices.size();

	//indicies need the order [0, 1, 2, 0, 2, 3]
	for (int i = 0; i < 2; i++) {
		m_indicies.push_back(vectStart);

		for (int j = 1; j < 3; j++) {
			m_indicies.push_back(vectStart + j + i);
		}
	}

	for (auto i : localVertices) {
		m_vertices.push_back(i);
	}
}

void TutorialScene::DrawTriangleFan(float3 colour)
{

}