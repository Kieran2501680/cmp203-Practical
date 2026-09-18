#include <Skateboard.h>
#include "Skateboard/EntryPoint.h"

#include "AssetEditorLayer.h"

class ToolboxApp : public Skateboard::Application
{
public:
	ToolboxApp() : Application({ L"Toolbox", {1280 ,720}, Skateboard::GraphicsContextDescription(), Skateboard::PlatformDescriptionFlags_InitGraphicsContextWithWindowSize})
	{
		PushLayer(new Toolbox::AssetEditorLayer());
	}
};

Skateboard::Application* Skateboard::CreateApplication(int argc, char** argv)
{
	return new ToolboxApp();
}