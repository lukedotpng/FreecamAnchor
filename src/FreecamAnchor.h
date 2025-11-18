#pragma once

#include <unordered_map>

#include "IPluginInterface.h"

#include <Glacier/ZEntity.h>
#include <Glacier/ZInput.h>
#include <Glacier/ZCollision.h>
#include <Glacier/ZItem.h>

class FreecamAnchor : public IPluginInterface
{
public:
    FreecamAnchor();
    ~FreecamAnchor() override;

    void Init() override;
    void OnEngineInitialized() override;
    void OnDrawMenu() override;
    void OnDrawUI(bool p_HasFocus) override;

private:
    void OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent);
    void ToggleFreecam();
    void EnableFreecam();
    void DisableFreecam();
	void AnchorToObject();
	bool GetFreeCameraRayCastClosestHitQueryOutput(ZRayQueryOutput& p_RayOutput);

private:
    DECLARE_PLUGIN_DETOUR(FreecamAnchor, bool, ZInputAction_Digital, ZInputAction* th, int a2);
    DECLARE_PLUGIN_DETOUR(FreecamAnchor, void, OnLoadScene, ZEntitySceneContext*, SSceneInitParameters&);
    DECLARE_PLUGIN_DETOUR(FreecamAnchor, void, OnClearScene, ZEntitySceneContext*, bool);

private:
    volatile bool m_FreeCamActive;
    volatile bool m_ShouldToggle;
    volatile bool m_FreeCamFrozen;
    ZEntityRef m_OriginalCam;
    ZSpatialEntity* m_AnchoredItemSpatial;
    SVector3 m_AnchorOffset;
    ZInputAction m_ToggleFreeCamAction;
    ZInputAction m_FreezeFreeCamActionGc;
    ZInputAction m_FreezeFreeCamActionKb;
    ZInputAction m_AnchoredObjectAction;
    ZInputAction m_IncreaseXOffset;
    ZInputAction m_DecreaseXOffset;
    ZInputAction m_IncreaseYOffset;
    ZInputAction m_DecreaseYOffset;
    ZInputAction m_IncreaseZOffset;
    ZInputAction m_DecreaseZOffset;
    ZInputAction m_ResetOffset;
    ZInputAction m_IncreaseOffsetStep;
    ZInputAction m_DecreaseOffsetStep;
    float m_OffsetStep;
    ZInputAction m_Unanchor;
    bool m_ControlsVisible;
    bool m_IsAnchored;
    bool m_NeedsToMove;
    bool m_DebugMenuActive;
    bool m_HasToggledFreecamBefore;
    bool m_isTaser;
    std::unordered_map<std::string, std::string> m_PcControls;
    std::unordered_map<std::string, std::string> m_ControllerControls;
};

DECLARE_ZHM_PLUGIN(FreecamAnchor)