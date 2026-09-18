#include "AssetImport.h"

AssetImport::AssetImport() : Skateboard::Scene("Asset Import Scene"),
Registry(),
Importer()
{

}

void AssetImport::OnHandleInput(Skateboard::TimeManager* time)
{
}

void AssetImport::OnUpdate(Skateboard::TimeManager* time)
{
}

void AssetImport::OnRender()
{
}

void AssetImport::OnImGuiRender()
{
	ImGui::Begin("Asset Importer");

	if(ImGui::Button("Import Mesh"))
	{
		
	}

	ImGui::End();
}
