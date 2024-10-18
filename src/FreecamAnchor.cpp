#include "FreecamAnchor.h"

#include <random>

#include "Events.h"
#include "Functions.h"
#include "Logging.h"

#include <Glacier/ZActor.h>
#include <Glacier/SGameUpdateEvent.h>
#include <Glacier/ZObject.h>
#include <Glacier/ZCameraEntity.h>
#include <Glacier/ZApplicationEngineWin32.h>
#include <Glacier/ZEngineAppCommon.h>
#include <Glacier/ZFreeCamera.h>
#include <Glacier/ZRender.h>
#include <Glacier/ZGameLoopManager.h>
#include <Glacier/ZHitman5.h>
#include <Glacier/ZHM5InputManager.h>
#include <Glacier/ZItem.h>
#include <Glacier/ZInputActionManager.h>

#include "IconsMaterialDesign.h"

FreecamAnchor::FreecamAnchor() :
	m_FreeCamActive(false),
	m_ShouldToggle(false),
	m_FreeCamFrozen(false),
	m_ControlsVisible(false),
	m_DebugMenuActive(false),
	m_ToggleFreeCamAction("ToggleFreeCamera"),
    m_FreezeFreeCamActionGc("ActivateGameControl0"),
    m_FreezeFreeCamActionKb("KBMInspectNode"),
	m_AnchoredObjectAction("ToggleFollowObject"),
	m_IncreaseXOffset("IncreaseXOffset"),
	m_DecreaseXOffset("DecreaseXOffset"),
	m_IncreaseYOffset("IncreaseYOffset"),
	m_DecreaseYOffset("DecreaseYOffset"),
	m_IncreaseZOffset("IncreaseZOffset"),
	m_DecreaseZOffset("DecreaseZOffset"),
	m_ResetOffset("ResetOffset"),
	m_IncreaseOffsetStep("IncreaseOffsetStep"),
	m_DecreaseOffsetStep("DecreaseOffsetStep"),
	m_OffsetStep(0.5),
	m_Unanchor("Unanchor")
{
    m_PcControls = {
        { "K", "Toggle freecam" },
        { "F3", "Lock camera and enable 47 input" },
        { "Ctrl + W/S", "Change FOV" },
        { "Ctrl + A/D", "Roll camera" },
	    { "Ctrl + X", "Reset roll" },
        { "Alt + W/S", "Change camera speed" },
        { "Space + Q/E", "Change camera height" },
        { "Space + W/S", "Move camera on axis" },
        { "Shift", "Increase camera speed" },
        { "Ctrl + F9", "Follow Object" },
    	{ "Shift + (8,9,0)", "Add to offset on (x,y,z) axis"},
    	{ "Ctrl + (8,9,0)", "Subtract to offset on (x,y,z) axis"},
		{ "l", "Unanchor from object (does not close freecam)" },
		{ ";", "Reset Offset"},
    	{ "[", "Decrease offset step"},
    	{ "]", "Increase offset step"},
    };
}

FreecamAnchor::~FreecamAnchor()
{
    const ZMemberDelegate<FreecamAnchor, void(const SGameUpdateEvent&)> s_Delegate(this, &FreecamAnchor::OnFrameUpdate);
    Globals::GameLoopManager->UnregisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdatePlayMode);

    // Reset the camera to default when unloading with freecam active.
    if (m_FreeCamActive)
    {
        TEntityRef<IRenderDestinationEntity> s_RenderDest;
        Functions::ZCameraManager_GetActiveRenderDestinationEntity->Call(Globals::CameraManager, &s_RenderDest);

        s_RenderDest.m_pInterfaceRef->SetSource(&m_OriginalCam);

        // Enable Hitman input.
        TEntityRef<ZHitman5> s_LocalHitman;
        Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &s_LocalHitman);

        if (s_LocalHitman)
        {
            auto* s_InputControl = Functions::ZHM5InputManager_GetInputControlForLocalPlayer->Call(Globals::InputManager);

            if (s_InputControl)
            {
                Logger::Debug("Got local hitman entity and input control! Enabling input. {} {}", fmt::ptr(s_InputControl), fmt::ptr(s_LocalHitman.m_pInterfaceRef));
                s_InputControl->m_bActive = true;
            }
        }
    }
}

void FreecamAnchor::Init()
{
    Hooks::ZInputAction_Digital->AddDetour(this, &FreecamAnchor::ZInputAction_Digital);
    Hooks::ZEntitySceneContext_LoadScene->AddDetour(this, &FreecamAnchor::OnLoadScene);
    Hooks::ZEntitySceneContext_ClearScene->AddDetour(this, &FreecamAnchor::OnClearScene);
}

void FreecamAnchor::OnEngineInitialized()
{
	const ZMemberDelegate<FreecamAnchor, void(const SGameUpdateEvent&)> s_Delegate(this, &FreecamAnchor::OnFrameUpdate);
	Globals::GameLoopManager->RegisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdatePlayMode);

	const char* binds = "FreeCameraInput={"
		"ToggleFreeCamera=tap(kb,k);"
		"ToggleFollowObject=& | hold(kb,lctrl) hold(kb,rctrl) tap(kb,f9);"
		"IncreaseXOffset=& | hold(kb,lshift) hold(kb,rshift) tap(kb,8);"
		"DecreaseXOffset=& | hold(kb,lctrl) hold(kb,rctrl) tap(kb,8);"
		"IncreaseYOffset=& | hold(kb,lshift) hold(kb,rshift) tap(kb,9);"
		"DecreaseYOffset=& | hold(kb,lctrl) hold(kb,rctrl) tap(kb,9);"
		"IncreaseZOffset=& | hold(kb,lshift) hold(kb,rshift) tap(kb,0);"
		"DecreaseZOffset=& | hold(kb,lctrl) hold(kb,rctrl) tap(kb,0);"
		"ResetOffset=tap(kb,semicolon);"
		"IncreaseOffsetStep=tap(kb,rbracket);"
		"DecreaseOffsetStep=tap(kb,lbracket);"
		"Unanchor=tap(kb,l);"
		"};";

	if (ZInputActionManager::AddBindings(binds))
	{
		Logger::Debug("Successfully added bindings.");
	}
	else
	{
		Logger::Debug("Failed to add bindings.");
	}
}

SVector3 CrossProduct(SVector3 a, SVector3 b) {
	SVector3 crossProduct;
	crossProduct.x = (a.y * b.z) - (a.z * b.y);
	crossProduct.y = (a.x * b.z) - (a.z * b.x);
	crossProduct.z = (a.x * b.y) - (a.y * b.x);
	return crossProduct;
}

SVector3 Normalize(SVector3 vec) {
	SVector3 normalVec(vec.x / vec.Length(), vec.y / vec.Length(), vec.z / vec.Length());
	return normalVec;
}

void FreecamAnchor::OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent)
{
    if (!*Globals::ApplicationEngineWin32)
        return;

    if (!(*Globals::ApplicationEngineWin32)->m_pEngineAppCommon.m_pFreeCamera01.m_pInterfaceRef)
    {
        Logger::Debug("Creating free camera.");
        Functions::ZEngineAppCommon_CreateFreeCamera->Call(&(*Globals::ApplicationEngineWin32)->m_pEngineAppCommon);

        // If freecam was active we need to toggle.
        // This can happen after level restarts / changes.
        if (m_FreeCamActive)
            m_ShouldToggle = true;
    }

	(*Globals::ApplicationEngineWin32)->m_pEngineAppCommon.m_pFreeCameraControl01.m_pInterfaceRef->SetActive(m_FreeCamActive);

    if (Functions::ZInputAction_Digital->Call(&m_ToggleFreeCamAction, -1))
    {
        ToggleFreecam();
    }

    if (m_ShouldToggle)
    {
        m_ShouldToggle = false;

        if (m_FreeCamActive)
            EnableFreecam();
        else
            DisableFreecam();
    }

    // While freecam is active, only enable hitman input when the "freeze camera" button is pressed.
    if (m_FreeCamActive)
    {
        if (Functions::ZInputAction_Digital->Call(&m_FreezeFreeCamActionKb, -1))
            m_FreeCamFrozen = !m_FreeCamFrozen;

        const bool s_FreezeFreeCam = Functions::ZInputAction_Digital->Call(&m_FreezeFreeCamActionGc, -1) || m_FreeCamFrozen;

    	(*Globals::ApplicationEngineWin32)->m_pEngineAppCommon.m_pFreeCameraControl01.m_pInterfaceRef->m_bFreezeCamera = s_FreezeFreeCam;

        TEntityRef<ZHitman5> s_LocalHitman;
        Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &s_LocalHitman);

        if (s_LocalHitman)
        {
            auto* s_InputControl = Functions::ZHM5InputManager_GetInputControlForLocalPlayer->Call(Globals::InputManager);

            if (s_InputControl)
                s_InputControl->m_bActive = s_FreezeFreeCam;
        }

    	if(m_NeedsToMove == true) {
    		auto s_Camera = (*Globals::ApplicationEngineWin32)->m_pEngineAppCommon.m_pFreeCamera01;
    		SMatrix43 updatedCamMatrix = s_Camera.m_pInterfaceRef->m_mTransform;
    		updatedCamMatrix.Trans = m_AnchoredItemSpatial->m_mTransform.Trans;

    		updatedCamMatrix.Trans.x += m_AnchorOffset.x;
    		updatedCamMatrix.Trans.y += m_AnchorOffset.y;
    		updatedCamMatrix.Trans.z += m_AnchorOffset.z;

    		s_Camera.m_pInterfaceRef->SetWorldMatrix(updatedCamMatrix);
    	}

    	if (Functions::ZInputAction_Digital->Call(&m_Unanchor, -1)) {
    		m_NeedsToMove = false;
    	}

    	if (Functions::ZInputAction_Digital->Call(&m_AnchoredObjectAction, -1)) {
    		Logger::Debug("Following Object :D");
    		AnchorToObject();
    	}

		if(Functions::ZInputAction_Digital->Call(&m_ResetOffset, -1)) {
			Logger::Debug("OFFSET RESET");
			m_AnchorOffset.x = 0;
			m_AnchorOffset.y = 0;
			m_AnchorOffset.z = 0;
		}

    	if(Functions::ZInputAction_Digital->Call(&m_DecreaseXOffset, -1)) {
    		Logger::Debug("X OFFSET DECREASED");
    		m_AnchorOffset.x -= m_OffsetStep;
    	}
    	if(Functions::ZInputAction_Digital->Call(&m_IncreaseXOffset, -1)) {
    		Logger::Debug("X OFFSET INCREASED");
    		m_AnchorOffset.x += m_OffsetStep;
    	}
    	if(Functions::ZInputAction_Digital->Call(&m_DecreaseYOffset, -1)) {
    		Logger::Debug("Y OFFSET DECREASED");
    		m_AnchorOffset.y -= m_OffsetStep;
    	}
    	if(Functions::ZInputAction_Digital->Call(&m_IncreaseYOffset, -1)) {
    		Logger::Debug("Y OFFSET INCREASED");
    		m_AnchorOffset.y += m_OffsetStep;
    	}
    	if(Functions::ZInputAction_Digital->Call(&m_DecreaseZOffset, -1)) {
    		Logger::Debug("Z OFFSET DECREASED");
    		m_AnchorOffset.z -= m_OffsetStep;
    	}
    	if(Functions::ZInputAction_Digital->Call(&m_IncreaseZOffset, -1)) {
    		Logger::Debug("Z OFFSET INCREASED");
    		m_AnchorOffset.z += m_OffsetStep;
    	}

    	if(Functions::ZInputAction_Digital->Call(&m_IncreaseOffsetStep, -1)) {
    		Logger::Debug("OFFSET STEP INCREASED");
    		m_OffsetStep += .05f;
    		if(m_OffsetStep > 2) {
    			m_OffsetStep = 2;
    		}
    	}
    	if(Functions::ZInputAction_Digital->Call(&m_DecreaseOffsetStep, -1)) {
    		Logger::Debug("OFFSET STEP DECREASED");
    		m_OffsetStep -= .05f;
    		if(m_OffsetStep < 0.05f) {
    			m_OffsetStep = 0.05f;
    		}
    	}
    }
}

void FreecamAnchor::OnDrawMenu()
{
    bool s_FreeCamActive = m_FreeCamActive;
    if (ImGui::Checkbox(ICON_MD_PHOTO_CAMERA " FREECAM FOLLOW", &s_FreeCamActive))
    {
        ToggleFreecam();
    }

    if (ImGui::Button(ICON_MD_SPORTS_ESPORTS " FREECAM FOLLOW CONTROLS")) {
	    m_ControlsVisible = !m_ControlsVisible;
    }

	if(ImGui::BeginMenu("Freecam Settings")) {
		ImGui::SliderFloat("Offset Step", &m_OffsetStep, .05f, 2.0f, "%.2f", ImGuiSliderFlags_None);

		ImGui::EndMenu();
	}
}

void FreecamAnchor::ToggleFreecam()
{
    m_FreeCamActive = !m_FreeCamActive;
    m_ShouldToggle = true;
    m_HasToggledFreecamBefore = true;
}

void FreecamAnchor::EnableFreecam()
{
    auto s_Camera = (*Globals::ApplicationEngineWin32)->m_pEngineAppCommon.m_pFreeCamera01;

    TEntityRef<IRenderDestinationEntity> s_RenderDest;
    Functions::ZCameraManager_GetActiveRenderDestinationEntity->Call(Globals::CameraManager, &s_RenderDest);

    m_OriginalCam = *s_RenderDest.m_pInterfaceRef->GetSource();

    const auto s_CurrentCamera = Functions::GetCurrentCamera->Call();
    s_Camera.m_pInterfaceRef->SetWorldMatrix(s_CurrentCamera->GetWorldMatrix());

    Logger::Debug("Camera trans: {}", fmt::ptr(&s_Camera.m_pInterfaceRef->m_mTransform.Trans));

    s_RenderDest.m_pInterfaceRef->SetSource(&s_Camera.m_ref);
}

void FreecamAnchor::DisableFreecam()
{
    TEntityRef<IRenderDestinationEntity> s_RenderDest;
    Functions::ZCameraManager_GetActiveRenderDestinationEntity->Call(Globals::CameraManager, &s_RenderDest);

    s_RenderDest.m_pInterfaceRef->SetSource(&m_OriginalCam);

    // Enable Hitman input.
    TEntityRef<ZHitman5> s_LocalHitman;
    Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &s_LocalHitman);

    if (s_LocalHitman)
    {
        auto* s_InputControl = Functions::ZHM5InputManager_GetInputControlForLocalPlayer->Call(Globals::InputManager);

        if (s_InputControl)
        {
            Logger::Debug("Got local hitman entity and input control! Enabling input. {} {}", fmt::ptr(s_InputControl), fmt::ptr(s_LocalHitman.m_pInterfaceRef));
            s_InputControl->m_bActive = true;
        	m_NeedsToMove = false;
        	m_AnchorOffset = { 0, 0, 0};

        }
    }
}

void FreecamAnchor::AnchorToObject()
{
	ZRayQueryOutput s_RayOutput{};

	if (GetFreeCameraRayCastClosestHitQueryOutput(s_RayOutput) && s_RayOutput.m_BlockingEntity)
	{
		TEntityRef<ZHitman5> s_LocalHitman;

		Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &s_LocalHitman);

		auto* s_BlockingEntitySpatial = s_RayOutput.m_BlockingEntity.QueryInterface<ZSpatialEntity>();

		if (s_BlockingEntitySpatial) {
			m_AnchoredItemSpatial = s_BlockingEntitySpatial;
			m_NeedsToMove = true;
			Logger::Debug("x:{} y:{} z:{}", s_BlockingEntitySpatial->m_mTransform.Trans.x, s_BlockingEntitySpatial->m_mTransform.Trans.y, s_BlockingEntitySpatial->m_mTransform.Trans.z);
		}
	}
}

bool FreecamAnchor::GetFreeCameraRayCastClosestHitQueryOutput(ZRayQueryOutput& p_RayOutput)
{
	auto s_Camera = (*Globals::ApplicationEngineWin32)->m_pEngineAppCommon.m_pFreeCamera01;
	SMatrix s_WorldMatrix = s_Camera.m_pInterfaceRef->GetWorldMatrix();
	float4 s_InvertedDirection = float4(-s_WorldMatrix.ZAxis.x, -s_WorldMatrix.ZAxis.y, -s_WorldMatrix.ZAxis.z, -s_WorldMatrix.ZAxis.w);
	float4 s_From = s_WorldMatrix.Trans;
	float4 s_To = s_WorldMatrix.Trans + s_InvertedDirection * 500.f;

	if (!*Globals::CollisionManager)
	{
		Logger::Error("Collision manager not found.");

		return false;
	}

	ZRayQueryInput s_RayInput{
		.m_vFrom = s_From,
		.m_vTo = s_To,
	};

	if (!(*Globals::CollisionManager)->RayCastClosestHit(s_RayInput, &p_RayOutput))
	{
		Logger::Error("Raycast failed.");

		return false;
	}

	return true;
}

void FreecamAnchor::OnDrawUI(bool p_HasFocus)
{
    if (m_ControlsVisible)
    {
        ImGui::PushFont(SDK()->GetImGuiBlackFont());
        const auto s_ControlsExpanded = ImGui::Begin(ICON_MD_PHOTO_CAMERA " FreecamFollow Controls", &m_ControlsVisible);
        ImGui::PushFont(SDK()->GetImGuiRegularFont());

        if (s_ControlsExpanded)
        {
            ImGui::TextUnformatted("PC Controls");

            ImGui::BeginTable("FreeCamControlsPc", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit);

			for (auto& [s_Key, s_Description]: m_PcControls)
			{
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(s_Key.c_str());
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(s_Description.c_str());
			}


            ImGui::EndTable();

			ImGui::TextUnformatted("Controller Controls");

			ImGui::BeginTable("FreeCamControlsController", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit);

			for (auto& [s_Key, s_Description]: m_ControllerControls) {
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(s_Key.c_str());
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(s_Description.c_str());
			}

			ImGui::EndTable();
		}

        ImGui::PopFont();
        ImGui::End();
        ImGui::PopFont();
    }
}

DEFINE_PLUGIN_DETOUR(FreecamAnchor, bool, ZInputAction_Digital, ZInputAction* th, int a2)
{
    if (!m_FreeCamActive)
        return HookResult<bool>(HookAction::Continue());

    if (strcmp(th->m_szName, "ActivateGameControl0") == 0 && m_FreeCamFrozen)
        return HookResult(HookAction::Return(), true);

    return HookResult<bool>(HookAction::Continue());
}

DEFINE_PLUGIN_DETOUR(FreecamAnchor, void, OnLoadScene, ZEntitySceneContext* th, ZSceneData&)
{
    if (m_FreeCamActive)
        DisableFreecam();

    m_FreeCamActive = false;
    m_ShouldToggle = false;

    return HookResult<void>(HookAction::Continue());
}

DEFINE_PLUGIN_DETOUR(FreecamAnchor, void, OnClearScene, ZEntitySceneContext* th, bool)
{
    if (m_FreeCamActive)
        DisableFreecam();

    m_FreeCamActive = false;
    m_ShouldToggle = false;

    return HookResult<void>(HookAction::Continue());
}

DEFINE_ZHM_PLUGIN(FreecamAnchor);