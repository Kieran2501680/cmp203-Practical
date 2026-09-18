#include "sktbdpch.h"

#define	SKTBD_LOG_COMPONENT "ApplicationBase"
#include "Skateboard/Log.h"

#include "Application.h"

#include "Graphics/RHI/RenderCommand.h"
#include "Graphics/RHI/ResourceFactory.h"

namespace Skateboard
{
	Application::Application(const PlatformDescription& platform_desc) :
		m_Platform(Platform::GetPlatform()),
		p_ImGuiMasterOverlay(new ImGuiLayer())	// Ownership is transferred to the layer stack, no need to worry about delete
	{
		// Set the platforms event call back function
		m_Platform.SetOnEventCallback(BIND_EVENT(Application::OnEvent));
		m_Platform.Init(platform_desc);

		// Initialise the render interfaces
		SKTBD_MSG_TRACE("Registering Graphics Context with RenderCommand")
		RenderCommand::RegisterRenderCommand(GraphicsContext::Context->GetAPI());

		SKTBD_MSG_TRACE("Registering Graphics Context with ResourceFactory")
		ResourceFactory::RegisterResourceFactory(GraphicsContext::Context->GetResourceFactory());

		//TODO: Need a more graceful solution, for now should work.


		SKTBD_MSG_ASSERT(!s_Instance, "Cannot create two application instances. Consider using layers to create other windows!");
		s_Instance = this;

		PushOverlay(p_ImGuiMasterOverlay);
	}

	void Application::Run()
	{
		bool running = true;
		while (running)
		{
			// Release some CPU consumption if the application is not used
			/*if (m_ApplicationPaused)
			{
				Sleep(100);
				continue;
			}*/

			// Update the platform (could be window messages, controller inputs, ...)
			// We will avoid uncessary rendering by quitting immediately if the context is destroyed
			if (!m_Platform.Update())
				break;

			// Get delta time from the platform (the procedure of getting time may differ!)
			TimeManager* timeManager = Platform::GetTimeManager();

			// Generic game loop
			for (Layer* layer : m_LayerStack)
				running &= layer->OnHandleInput(timeManager);
			for (Layer* layer : m_LayerStack)
				running &= layer->OnUpdate(timeManager);

			//Perform any internal graphics context update (eg. uploading resources to GPU or resize back buffers)
			Skateboard::GraphicsContext::Update();

			// Prepare internal engine for rendering scene.
			Skateboard::GraphicsContext::BeginFrame();

			// Render all the layers main graphics
			for (Layer* layer : m_LayerStack)
				layer->OnRender();

			// Render all the user interface
			p_ImGuiMasterOverlay->Begin();
			for (Layer* layer : m_LayerStack)
				layer->OnImGuiRender();
			p_ImGuiMasterOverlay->End();

			// Execute the rendering work recorded on the GPU
			Skateboard::GraphicsContext::EndFrame();
		}

		// When exitting the app, ensure all GPU work has concluded
		SKTBD_MSG_INFO("Exiting App, GPU idle expected..");
		Skateboard::GraphicsContext::WaitUntilIdle();
	}

	void Application::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);

		dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT(Application::OnWindowResize));

		Platform::GetPlatform().OnEvent(e);

		// Go through each layer on the stack and pass the event forward to them
		for (auto it = m_LayerStack.end(); it != m_LayerStack.begin(); )
		{
			(*--it)->OnEvent(e);

			if (e.IsHandled())
				break;
		}
	}

	bool Application::OnWindowResize(WindowResizeEvent& e)
	{
		GraphicsContext::Resize(e.GetWidth(), e.GetHeight(), e.IsFullscreen());

		return false;
	}
}