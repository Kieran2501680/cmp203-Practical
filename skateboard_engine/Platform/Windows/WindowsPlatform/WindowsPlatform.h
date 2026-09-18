#pragma once
#include "windowspch.h"

#include "Skateboard/Platform.h"
#include "Skateboard/Time/TimeManager.h"

#define MINIMUM_WINDOW_SIZE_WIDTH 640
#define MINIMUM_WINDOW_SIZE_HEIGHT 480

#include "Skateboard/Events/AppEvents.h"
#include "Skateboard/Events/MouseEvent.h"
#include "Skateboard/Events/KeyEvent.h"

struct ImGuiViewport;
namespace Skateboard
{
	class WindowsPlatform : public Platform
	{
	public:
		WindowsPlatform();
		DISABLE_COPY_AND_MOVE(WindowsPlatform);

		virtual ~WindowsPlatform() final override;

		void Init(const PlatformDescription& props) final override;

		// Update the system app
		virtual bool Update() final override;

		void ResizeBackBuffers(WPARAM wParam, LPARAM lParam);

		HWND GetWindow() const { return m_ActiveWindow; }

		void Terminate() override;
		void ToggleFullscreen() override;


		virtual void InitImGui() final override;
		virtual void BeginImGuiPass() final override;
		virtual void EndImGuiPass() final override;
		virtual void ShutdownImGui() final override;
		//static void ImGui_ImplWin32_CreateWindow_Custom(ImGuiViewport* viewport);
	private:
		// Inistialise the window
		BOOL InitWindowsApp(PlatformDescription& props);

		void GoFullScreen();
		// Procedure to handle the messages of the window
		// Needs to be static otherwise it cannot be used when initialing the window, since it is a member of the class System
		static LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
		static LRESULT CALLBACK ImGuiWindowProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	protected:
		std::unique_ptr<TimeManager> CreateTimeManager_() override;

	private:
		//TODO: At some point when the event system is complete, remove this
		//		and use the dispatch system to forward data into higher layers.
		HWND m_MainWindow;
		HWND m_ActiveWindow;

		uint2 m_WindowClientSize;
		RECT  m_WindowRectSave;

		bool m_FullscreenMode;
		bool m_ApplicationMinimised;
		bool m_ApplicationMaximised;
		bool m_ApplicationResizing;
		bool m_ApplicationIsInSizingModalLoop;
	};

}