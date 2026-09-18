#include <Skateboard.h>
#include "Skateboard/EntryPoint.h"

#include "DefaultGameLayer.h"

class GameApp : public Skateboard::Application
{
public:
	GameApp(const Skateboard::PlatformDescription& desc) : Application(desc)
	{
		PushLayer(new DefaultGameLayer());
	}
};

Skateboard::Application* Skateboard::CreateApplication(int argc, char** argv)
{
	PlatformDescription description;

	description.GraphicsContextInitDesc = GraphicsContextDescription::Default();
	description.Title = L"Skateboard Engine";
	description.WindowSize = { 1280, 720 };
	description.Flags = PlatformDescriptionFlags_None;

	return new GameApp(description);
}