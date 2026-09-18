#pragma once
#include "sktbdpch.h"

#include "Skateboard/Graphics/RHI/GraphicsContext.h"
#include "Skateboard/Time/TimeManager.h"
#include "Skateboard/Events/Event.h"
#include "Skateboard/User.h"
#include "Skateboard/Input/DeviceManager.h"

#include "Input.h"
#include "Assets/AssetManager.h"

namespace Skateboard
{
	class Logger;

	enum PlatformDescriptionFlags_ : uint8_t
	{
		PlatformDescriptionFlags_None = 0,
		PlatformDescriptionFlags_WindowLess = 1,
		PlatformDescriptionFlags_WindowSizeAuto = 1 << 2,
	};
	ENUM_FLAG_OPERATORS(PlatformDescriptionFlags_)

	struct PlatformDescription
	{
		std::wstring Title;

		uint2 WindowSize;

		GraphicsContextDescription GraphicsContextInitDesc;
		PlatformDescriptionFlags_ Flags;
	};

	class Platform
	{
		DISABLE_COPY_AND_MOVE(Platform)
		friend class Application;

	public:
		Platform() {};
		virtual ~Platform()
		{
			// release subordinate functions
			m_AssetManager.reset();
			m_GraphicsContext.reset();
			m_Timer.reset();
			m_Users.reset();
			m_Devices.reset();
			m_Logger.reset();
		}

		// look at https://refactoring.guru/design-patterns/singleton
		static Platform& GetPlatform();

		static TimeManager* GetTimeManager() { return GetPlatform().m_Timer.get(); }
		static UserManager* GetUserManager() { return GetPlatform().m_Users.get(); }
		static DeviceManager* GetDeviceManager() { return GetPlatform().m_Devices.get(); }
		static GraphicsContext* GetGraphicsContext() { return GetPlatform().m_GraphicsContext.get(); }
		static AssetManager* GetAssetManager() { return GetPlatform().m_AssetManager.get(); }
		static Logger* GetLogger() { return GetPlatform().m_Logger.get(); }

		static std::unique_ptr<TimeManager> CreateTimeManager() { return GetPlatform().CreateTimeManager_(); }

		inline static std::optional<User> m_GamePadImGuiControl = std::optional<User>(); // to navigate imgui with a controller // Works only on PS5 but could be extended using new Imgui Navigation features 

		virtual void Init(const PlatformDescription& desc)
		{
			SKTBD_LOG_TRACE("Platform", "Registering Device Manager with Input")
			Input::RegisterDeviceManager(GetDeviceManager());
			m_GraphicsContext->Init(desc.GraphicsContextInitDesc, CreateTimeManager());
			m_AssetManager->Init();
			m_Devices->Init();
			m_Users->Init();
		}

		virtual bool Update()
		{
			m_Timer->Update();
			m_Users->Update();
			m_Devices->Update();
			return true;
		}

		//maybe restrict access to these
		static  void PlatformDispatchEvent(Event& e) { Platform::GetPlatform().EventCallback(e); }

		virtual void InitImGui() = 0;
		virtual void BeginImGuiPass() = 0;
		virtual void EndImGuiPass() = 0;
		virtual void ShutdownImGui() = 0;

		
	protected:
		virtual void SetOnEventCallback(std::function<void(Event&)> callback) { EventCallback = callback; }
		virtual void OnEvent(Event& e) { m_Users->OnEvent(e); m_Devices->OnEvent(e);}

		virtual std::unique_ptr<TimeManager> CreateTimeManager_() = 0;

		virtual void Terminate(){}
		virtual void ToggleFullscreen() {}

	protected:
		std::function<void(Event&)> EventCallback;

		std::unique_ptr<GraphicsContext> m_GraphicsContext;
		std::unique_ptr<TimeManager> m_Timer;
		std::unique_ptr<UserManager> m_Users;
		std::unique_ptr<DeviceManager> m_Devices;
		std::unique_ptr<AssetManager> m_AssetManager;
		std::unique_ptr<Logger> m_Logger;
	};

	//because everything is in the same static lib we can create this symbol anywhere
	#define IMPLEMENT_PLATFORM(PlatformClass) Platform& Platform::GetPlatform() { static PlatformClass Platform_Inst; return Platform_Inst; }

}
