#include "D3DScene.h"

//D3Dx12 is a helper library provided by Microsoft to help with some of the boilerplate code required when using DirectX 12
//it is included in the DirectX 12 Agility SDK, include for it can be added via NUGet package manager or like in this case whole folder can be added to the project files from the Skateboard engine Nuget
//[SLN Directory]\packages\Microsoft.Direct3D.D3D12.x.xxx.x\build\native\include
#include <numeric>

#include "AsyncCompute/AsyncCompute.h"
#include "d3dx12/d3dx12.h"
#include "PostProcess/PostProcessChain.h"
#include "Skateboard/Application.h"
#include "Raytracing/Raytracer.h"
#include "LightsAndShadows/Vertex.h"
#include "LightsAndShadows/DefferedPass/DeferredPass.h"
#include "LightsAndShadows/ForwardPass/ForwardPass.h"
#include "LightsAndShadows/Meshlets/Meshlets.h"

D3DScene::D3DScene(const std::string& name) : Scene(name)
{
    auto m_device = Skateboard::D3D::gD3DContext->GetDevice();

    auto sphere = Skateboard::SceneBuilder::BuildSphere(Vertex::GetSKTBDLayout());
    Skateboard::AssetManager::CreateModelFromBuffers("sphere", { sphere });

    auto plane = Skateboard::SceneBuilder::BuildPlane(Vertex::GetSKTBDLayout());
    Skateboard::AssetManager::CreateModelFromBuffers("plane", { plane });

	//Skateboard::AssetManager::LoadModel(L"assets/models/torus", "torus", LightAndShadowRenderer::Vertex::GetSKTBDLayout());
	//Skateboard::AssetManager::LoadModel(L"assets/models/torus", "torus", LightAndShadowRenderer::Vertex::GetSKTBDLayout());

   

	auto crate = Skateboard::AssetManager::LoadTexture(L"assets/crate", "crate");
	auto coolTexture = Skateboard::AssetManager::LoadTexture(L"assets/TestImages/cool-cool-emoji", "cool");
	auto stone = Skateboard::AssetManager::LoadTexture(L"assets/stone", "stone");

    SamplerDesc samplerdesc; samplerdesc = SamplerDesc::InitAsDefaultTextureSampler();
    DefaultSampler = ResourceFactory::CreateSampler(samplerdesc,L"DefaultSampler");

    m_camera = Skateboard::PerspectiveCamera(45.0f, (float)Skateboard::D3D::gD3DContext->GetClientWidth() / (float)Skateboard::D3D::gD3DContext->GetClientHeight(), 100.f, 0.01f);

    BindlessRootSignature::Prepare();
    PostProcessChain::Prepare();
    Triangle::Prepare();
	LightAndShadowRenderer::Prepare();

    Meshlets::Prepare();

	/* auto meshdata = Skateboard::AssetManager::LoadModelCPU(L"assets/models/torus", Vertex::GetSKTBDLayout());
	Skateboard::AssetManager::CreateModelFromBuffers("torus", meshdata);*/

	MeshInfo meshinfo_0;
	meshinfo_0.filename = L"assets/models/torus";
	meshinfo_0.tag = "torus";

	auto meshes = std::vector<Mesh*>{};

    meshes = Meshlets::PreprocessMeshesIntoMeshlets({ meshinfo_0 }, Vertex::GetSKTBDLayout());

    ForwardPass::Prepare();

    ChromaAberration.Prepare(BindlessRootSignature::GetSignature());

    LightAndShadowRenderer::SetCameraData(m_camera.GetViewMatrix(),m_camera.GetProjectionMatrix(), m_camera.GetPosition());

    auto instance_0 =  LightAndShadowRenderer::PushInstance({ glm::eulerAngleXYZ(-3.14f/6.f,0.f,0.f), crate->GetViewIndex(), DefaultSampler->GetSamplerIndex(), 2, 1});
    auto instance_1 = LightAndShadowRenderer::PushInstance({ glm::translate(float3{0.f,-3.f,0.f})*glm::scale(float3{10,1,10}), coolTexture->GetViewIndex(), DefaultSampler->GetSamplerIndex(), 2,1});

    AsyncCompute::Prepare();
    AsyncCompute::Begin();
    //build acceleration structures
    {
        auto CommandList = AsyncCompute::GetAsyncList();

        //meshes.push_back(AssetManager::GetModel("torus"));
		meshes.push_back(AssetManager::GetModel("plane"));

        auto BlasAddreses = DXRTracing::BuildBottomLevelAccelerationStructures(meshes, CommandList);

        CD3DX12_BUFFER_BARRIER bufferBarriers[1];
        bufferBarriers[0] = CD3DX12_BUFFER_BARRIER(
            D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE,
            D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE,
            D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_WRITE,
            D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_READ,
            DXRTracing::GetBLAS()->GetResource()
        );
        CD3DX12_BARRIER_GROUP barrierGroup = CD3DX12_BARRIER_GROUP(1, bufferBarriers);

        CommandList->Barrier(1, &barrierGroup);

        std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instances(2);

        {
            instances[0].Flags = D3D12_RAYTRACING_INSTANCE_FLAGS::D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
            instances[0].AccelerationStructure = BlasAddreses[0];
            instances[0].InstanceContributionToHitGroupIndex = 0;
            instances[0].InstanceID = instance_0;
            instances[0].InstanceMask = 0xFF;
            glm::float4x4 transform = glm::eulerAngleXYZ(-3.14f / 6.f, 0.f, 0.f);
            auto src = glm::transpose(transform);
            memcpy(instances[0].Transform, &src, sizeof(float3x4));
        }

        {
            instances[1].Flags = D3D12_RAYTRACING_INSTANCE_FLAGS::D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
            instances[1].AccelerationStructure = BlasAddreses[1];
            instances[1].InstanceContributionToHitGroupIndex = 0;
            instances[1].InstanceID = instance_1;
            instances[1].InstanceMask = 0xFF;
            glm::float4x4 transform = (glm::translate(float3{ 0.f,-3.f,0.f }) * glm::scale(float3{ 10,1,10 }));
            auto src = glm::transpose(transform);
            memcpy(instances[1].Transform, &src, sizeof(float3x4));
        }

        DXRTracing::BuildTopLevelAccelerationStructures(instances, CommandList);

    	bufferBarriers[0] = CD3DX12_BUFFER_BARRIER(
            D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE,
            D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE,
            D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_WRITE,
            D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_READ,
            DXRTracing::GetTLAS().first->GetResource()
        );
        CommandList->Barrier(1, &barrierGroup);
    }

    AsyncCompute::End();
    AsyncCompute::InsertWaitIntoGraphicsQueue();

    DeferredPass::Prepare();

    DXRTracing::Description desc;
    desc.SetLibrary(L"RayLibrary");

    //desc.AddRaygen(L"MyRaygenShader");
    //desc.AddHitGroup(D3D12_HIT_GROUP_TYPE_TRIANGLES, L"MyHitGroup", nullptr, L"MyClosestHitShader", nullptr);
    //desc.AddMiss(L"MyMissShader");
    //desc.SetConfigSetConfig(sizeof(float4), sizeof(float2), 1, 1);


    desc.AddRaygen(L"AmbientOcclusionRaygen");
    desc.AddHitGroup(D3D12_HIT_GROUP_TYPE_TRIANGLES, L"AO", L"AoAnyHit", L"AoClosestHit", nullptr);
    desc.AddHitGroup(D3D12_HIT_GROUP_TYPE_TRIANGLES, L"Shadow", nullptr, L"ShadowClosestHit", nullptr);
    desc.AddMiss(L"AoMiss");
    desc.AddMiss(L"ShadowMiss");
    desc.SetConfigSetConfig(sizeof(float4), sizeof(float2), 1, 1);

    DXRTracing::Prepare(desc, BindlessRootSignature::GetSignature());
    DXRTracing::BuildShaderTable(desc,0);

    LightAndShadowRenderer::SetGeometryInfoAndStorageBufferIDX(std::get<D3D::SHADER_VIEW_HANDLE>(DXRTracing::m_PrimitiveData.Get()).GetIndex(), AssetManager::GetBackingByteAddressBufferSRV()->GetViewIndex());
	LightAndShadowRenderer::SetRaytracingSceneIDX(DXRTracing::GetTLAS().second.GetIndex());

    LightAndShadowRenderer::SetAmbientLight(float4(0.2f, 0.1f, 0.01f, 1));
}

D3DScene::~D3DScene()
{
    //things in those are defined statically, as they contain refrences those need to be released manually
    LightAndShadowRenderer::Shutdown();
    Triangle::Shutdown();
}

void D3DScene::OnHandleInput(Skateboard::TimeManager* time)
{
	static float3 UP = { 0,1,0 };
	static float3 RIGHT = { 1,0,0 };
	static float3 FORWARD = { 0,0,1 };

    static float m_speed = 10.f;
	static float m_rotation_sensitivity = 0.5f;

	auto rotation = glm::quat(glm::radians(m_camera.GetRotation()));
	auto position = m_camera.GetPosition();
	auto dt = time->DeltaTime();

    if(Input::IsKeyDown(Keys::sc_w))
    {
        m_camera.SetPosition(position += glm::rotate(rotation, FORWARD) * m_speed * dt);
    }

    if (Input::IsKeyDown(Keys::sc_s))
    {
        m_camera.SetPosition(position += glm::rotate(rotation, -FORWARD) * m_speed * dt);
    }

    if (Input::IsKeyDown(Keys::sc_d))
    {
        m_camera.SetPosition(position += glm::rotate(rotation, RIGHT) * m_speed * dt);
    }

    if (Input::IsKeyDown(Keys::sc_a))
    {
        m_camera.SetPosition(position += glm::rotate(rotation, -RIGHT) * m_speed * dt);
    }

    if (Input::IsKeyDown(Keys::sc_q))
    {
        m_camera.SetPosition(position += glm::rotate(rotation, -UP) * m_speed * dt);
    }

    if (Input::IsKeyDown(Keys::sc_e))
    {
        m_camera.SetPosition(position += glm::rotate(rotation, UP) * m_speed * dt);
    }

    if (Input::IsKeyPressed(Keys::sc_t))
    {
        Application::Singleton()->ToggleFullscreen();
    }

    static int2 mouse_pos_on_press = { 0,0 };
    static float3 rotation_on_press = { 0,0,0 };

    if(Input::IsMouseButtonDown(mb_RightButton))
    {
        Input::SetMouseVisible(false);

        if(Input::IsMouseButtonPressed(mb_RightButton))
        {
            mouse_pos_on_press = Input::GetMousePos();
            rotation_on_press = m_camera.GetRotation();
        }

    	auto mouse_pos = Input::GetMousePos();

    	float2 delta_pos = mouse_pos - mouse_pos_on_press;

        m_camera.SetRotation({ rotation_on_press.x + delta_pos.y * m_rotation_sensitivity, rotation_on_press.y + delta_pos.x * m_rotation_sensitivity, 0 });
    }
    else
    {
        mouse_pos_on_press = {};
        rotation_on_press = {};

        Input::SetMouseVisible(true);
    }

    m_camera.UpdateViewMatrix();

    LightAndShadowRenderer::SetCameraData(m_camera.GetViewMatrix(),m_camera.GetProjectionMatrix(), m_camera.GetPosition());


    if (Input::IsKeyPressed(Keys::sc_escape))
    {
        Application::Singleton()->Terminate();
    }
}

void D3DScene::OnUpdate(Skateboard::TimeManager* time)
{
	Scene::OnUpdate(time);

    LightAndShadowRenderer::UpdateLightsAndFrameData();
}

void D3DScene::OnRender()
{
    auto CommandList = Skateboard::D3D::gD3DContext->GetD3DDefaultFrameCommandList();

    //Rasterization
  //  {
  //      //we need to change the resource state of the render target to be able to write to it
		//ID3D12Resource* res[] = {PostProcessChain::GetInput<HandleType::Resource>()->GetResource()};

  //      Transition::BarrierToRTV(res, PostProcessChain::GetInput<HandleType::ResourceState>(), CommandList);

  //      //first we clear the render target
  //      float4 ClearColor{ 0.1,0.1,0.1,1.0f };

  //      CommandList->ClearRenderTargetView(PostProcessChain::GetInput<HandleType::RTV>().GetCPUHandle(), (float*)&ClearColor, 0, nullptr);

  //      //then we replace the current render target with our offscreen one
  //      CommandList->OMSetRenderTargets(1, &PostProcessChain::GetInput<HandleType::RTV>().GetCPUHandle(), false, Skateboard::D3D::gD3DContext->GetD3DDepthStencilHandle());

  //      //Triangle::SetStateAndDraw(CommandList);

        CommandList->SetGraphicsRootSignature(BindlessRootSignature::GetSignature());

        LightAndShadowRenderer::SetGraphicsFrameData(CommandList);

        auto DrawCalls = [](ID3D12GraphicsCommandList10* list, const LightAndShadowRenderer::Light_CPU* light, uint32_t InstanceCountMultiplier, uint32_t Execution_idx)
            {
                list->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

				//draw meshlets with old method for comparison
        	/*
                for(int i = 0; i < 25; i++)
                {
					auto name = "torus_meshlets";

                    auto primitive = Skateboard::AssetManager::GetModel(name)->GetPrimitive(i);

                    auto ContainingBuffer = static_cast<Skateboard::D3D::D3DBuffer*>(primitive->VertexBuffers[0].m_ParentResource.get());

                    auto primitiveVB = primitive->VertexBuffers[0];
                    auto primitiveIB = primitive->IndexBuffer;

                    //D3D doesnt support uint8 index buffers, PS5 does.
                    auto IBformat = (primitiveIB.m_Format == bit16) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
                    uint32_t IBstride = primitiveIB.m_Format;

                    //skateboard doesnt store a d3d12 specific view for the buffers, so we need to create them here

                    uint IBsize = ROUND_UP(primitiveIB.m_IndexCount * IBstride, 4);

                    D3D12_INDEX_BUFFER_VIEW IB{ ContainingBuffer->GetResourceGPUAddress() + primitiveIB.m_Offset, IBsize, IBformat };
                    list->IASetIndexBuffer(&IB);

                    D3D12_VERTEX_BUFFER_VIEW VB{ ContainingBuffer->GetResourceGPUAddress() + primitiveVB.m_Offset, primitiveVB.m_VertexStride * primitiveVB.m_VertexCount, primitiveVB.m_VertexStride };

                    //SKTBD_LOG_TRACE("Meshlets", "vertex buffer location {}, size inf bytes {}, Index buffer size in bytes {}", VB.BufferLocation, VB.SizeInBytes, IB.SizeInBytes);

                    list->IASetVertexBuffers(0, 1, &VB);

                    //use instance data 1; rotation
                    list->SetGraphicsRoot32BitConstant(1, 1, 0);

                    list->DrawIndexedInstanced(primitiveIB.m_IndexCount, InstanceCountMultiplier, 0, 0, 0);
                }
        	*/

                {
                    auto primitive = Skateboard::AssetManager::GetModel("plane")->GetPrimitive(0);

                    auto ContainingBuffer = static_cast<Skateboard::D3D::D3DBuffer*>(primitive->VertexBuffers[0].m_ParentResource.get());

                    auto primitiveVB = primitive->VertexBuffers[0];
                    auto primitiveIB = primitive->IndexBuffer;

                    //D3D doesnt support uint8 index buffers, PS5 does.
                    auto IBformat = (primitiveIB.m_Format == bit16) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
                    uint32_t IBstride = (primitiveIB.m_Format == bit16) ? sizeof(uint16_t) : sizeof(uint32_t);

                    //skateboard doesnt store a d3d12 specific view for the buffers, so we need to create them here

                    D3D12_INDEX_BUFFER_VIEW IB{ ContainingBuffer->GetResourceGPUAddress() + primitiveIB.m_Offset, primitiveIB.m_IndexCount * IBstride, IBformat };
                    list->IASetIndexBuffer(&IB);

                    D3D12_VERTEX_BUFFER_VIEW VB{ ContainingBuffer->GetResourceGPUAddress() + primitiveVB.m_Offset, primitiveVB.m_VertexStride * primitiveVB.m_VertexCount, primitiveVB.m_VertexStride };

                    list->IASetVertexBuffers(0, 1, &VB);

                    //use instance data 2; translation
                    list->SetGraphicsRoot32BitConstant(1, 2, 0);

                    list->DrawIndexedInstanced(primitiveIB.m_IndexCount, InstanceCountMultiplier, 0, 0, 0);
                } 
            };

  //      LightAndShadowRenderer::BeginDepthPass(CommandList);

  //      LightAndShadowRenderer::RecordCommandPerLight(CommandList, DrawCalls);

  //      LightAndShadowRenderer::PrepareShadowMapsForReading(CommandList);

  //      //we need to set the viewport and scissor rect to the size of our offscreen render target which comes from post process RT
  //      PostProcessChain::SetScissorAndViewport(CommandList);

  //      //ForwardPass::Begin(CommandList, &PostProcessChain::GetInput<HandleType::RTV>().GetCPUHandle(), Skateboard::D3D::gD3DContext->GetD3DDepthStencilHandle());

        DeferredPass::Begin(CommandList);

        DrawCalls(CommandList, nullptr, 1, 0);

        Meshlets::DrawMeshlets(CommandList, 0, 1);

  //      DeferredPass::End(CommandList);

  //      DeferredPass::ResolveGBuffer(CommandList, PostProcessChain::GetInput<HandleType::UAV>().GetIndex());
  //  }

    //Raytracing
    {
        
        CommandList->SetComputeRootSignature(BindlessRootSignature::GetSignature());

        LightAndShadowRenderer::SetComputeFrameData(CommandList);

		ID3D12Resource* res[] = { PostProcessChain::GetOutput<HandleType::Resource>()->GetResource() };

        Transition::BarrierToUAV(res, PostProcessChain::GetOutput<HandleType::ResourceState>(), CommandList);

        uint2 outputsize = PostProcessChain::GetDimensions();

        DXRTracing::SetInOutAndDispatch(CommandList, DeferredPass::SRVs, PostProcessChain::GetOutput<HandleType::UAV>(), { outputsize.x,outputsize.y,1 });

    }

	//Do Post Processing
    //PostProcessChain::PrepareForPostProcessing(CommandList);

    //ChromaAberration.Apply(CommandList, PostProcessChain::GetInput<>(), PostProcessChain::GetOutput<>());

    //PostProcessChain::Flip();

	//return back to the main back buffer, we are not using dsv for imgui so we dont need to set it
    CommandList->OMSetRenderTargets(1, Skateboard::D3D::gD3DContext->GetD3DCurrentBackBufferRTVHandle(), false, nullptr);
}

void D3DScene::OnEvent(Event& e)
{
	Scene::OnEvent(e);

    EventDispatcher dispatcher(e);

    SKTBD_MSG_TRACE("Begin Resize")

    dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT([&](const WindowResizeEvent& e)
        {
    	PostProcessChain::CreateOuputTextures(e.GetWidth(), e.GetHeight());
        DeferredPass::CreateTextures(e.GetWidth(), e.GetHeight());
		return false;
        }));


    SKTBD_MSG_TRACE("End Resize")
}

void D3DScene::OnImGuiRender()
{
    auto CommandList = Skateboard::D3D::gD3DContext->GetD3DDefaultFrameCommandList();

	//then we can render the texture to the screen using ImGui

	//ImGui::Begin("D3DScene", 0, ImGuiWindowFlags_::ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_::ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_::ImGuiWindowFlags_NoMove | ImGuiWindowFlags_::ImGuiWindowFlags_NoBringToFrontOnFocus);
	ImGui::Begin("D3D Scene");

	//ImGui::Image(DeferredPass::SRVs.GetGPUHandle().ptr, ImVec2(PostProcessChain::GetOutput<HandleType::Resource>()->GetResource()->GetDesc().Width-15, PostProcessChain::GetOutput<HandleType::Resource>()->GetResource()->GetDesc().Height-15));
	ImGui::Image(PostProcessChain::GetOutput<HandleType::SRV>().GetGPUHandle().ptr, ImVec2(PostProcessChain::GetOutput<HandleType::Resource>()->GetResource()->GetDesc().Width-15, PostProcessChain::GetOutput<HandleType::Resource>()->GetResource()->GetDesc().Height-15));

	ImGui::End();

    ImGui::Begin("D3DData");
    ImGui::Text("FPS: %f", Platform::GetTimeManager()->FPS());
    ImGui::Text("Frame: %f", Platform::GetTimeManager()->DeltaTime()*1000);

    ImGui::End();

   /* ImGui::Begin("Depth");
    ImGui::Image(LightAndShadowRenderer::GetLights()[0].GetLightSRV().GetGPUHandle().ptr, ImVec2(m_RenderTarget->GetResource()->GetDesc().Width, m_RenderTarget->GetResource()->GetDesc().Height));
    ImGui::End();*/

	//and change the resource state back to render target so we can render to it again next frame



	Scene::OnImGuiRender();
}
