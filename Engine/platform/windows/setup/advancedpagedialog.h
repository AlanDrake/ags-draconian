//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#pragma once

#include "platform/platform.h"

#if AGS_PLATFORM_OS_WINDOWS

#include "platform/windows/setup/basicpagedialog.h"

namespace AGS
{
namespace Engine
{


//=============================================================================
//
// AdvancedPageDialog
//
//=============================================================================
class AdvancedPageDialog : public WinSetupPageDialog
{
public:
    AdvancedPageDialog(WinConfig &win_cfg, const Common::ConfigTree &cfg_in) : WinSetupPageDialog(win_cfg, cfg_in) {}

    Common::String GetTitle() const override { return "Advanced"; }

    void ResetSetup(const Common::ConfigTree &cfg_from) override;
    void SaveSetup() override;

protected:
    UINT GetTemplateID() const override { return IDD_PAGE_ADVANCED; }

    // Event handlers
    INT_PTR OnInitDialog() override;
    INT_PTR OnDialogEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

private:
    static const int MouseSpeedMin = 1;
    static const int MouseSpeedMax = 100;

    // Operations
    void FillAudioDriverList();
    void UpdateMouseSpeedText();

    // Dialog properties
    // Audio driver descriptions
    std::vector<std::pair<Common::String, Common::String>> _drvAudioList;

    // Dialog controls
    HWND _hSpriteCacheList = nullptr;
    HWND _hTextureCacheList = nullptr;
    HWND _hAudioDriverList = nullptr;
    HWND _hSoundCacheList = nullptr;
    HWND _hVSync = nullptr;
    HWND _hRenderAtScreenRes = nullptr;
    HWND _hAntialiasSprites = nullptr;
    HWND _hUseVoicePack = nullptr;
    HWND _hMouseLock = nullptr;
    HWND _hMouseSpeed = nullptr;
    HWND _hMouseSpeedText = nullptr;
};

//=============================================================================
//
// GameFilesPageDialog
//
//=============================================================================
class GameFilesPageDialog : public WinSetupPageDialog
{
public:
    GameFilesPageDialog(WinConfig &win_cfg, const Common::ConfigTree &cfg_in) : WinSetupPageDialog(win_cfg, cfg_in) {}

    Common::String GetTitle() const override { return "Game Files"; }

    void ResetSetup(const Common::ConfigTree &cfg_from) override;
    void SaveSetup() override;

protected:
    UINT GetTemplateID() const override { return IDD_PAGE_PATHS; }

    // Event handlers
    INT_PTR OnInitDialog() override;
    INT_PTR OnCommand(WORD id) override;

private:
    // Event handlers
    void OnCustomSaveDirBtn();
    void OnCustomSaveDirCheck();
    void OnCustomAppDataDirBtn();
    void OnCustomAppDataDirCheck();

    // Dialog controls
    HWND _hCustomSaveDir = nullptr;
    HWND _hCustomSaveDirBtn = nullptr;
    HWND _hCustomSaveDirCheck = nullptr;
    HWND _hCustomAppDataDir = nullptr;
    HWND _hCustomAppDataDirBtn = nullptr;
    HWND _hCustomAppDataDirCheck = nullptr;
    HWND _hCompressSavesCheck = nullptr;
};

//=============================================================================
//
// AccessibilityPageDialog
//
//=============================================================================
class AccessibilityPageDialog : public WinSetupPageDialog
{
public:
    AccessibilityPageDialog(WinConfig &win_cfg, const Common::ConfigTree &cfg_in, Common::ConfigTree &cfg_out)
        : WinSetupPageDialog(win_cfg, cfg_in),
          _cfgOut(cfg_out)
    {
    }

    static bool ShouldDisplayPage(const Common::ConfigTree &cfg_in);

    Common::String GetTitle() const override { return "Accessibility"; }

    void ResetSetup(const Common::ConfigTree &cfg_from) override;
    void SaveSetup() override;

protected:
    UINT GetTemplateID() const override { return IDD_PAGE_ACCESS; }

    // Event handlers
    INT_PTR OnInitDialog() override;
    INT_PTR OnDialogEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
    INT_PTR OnCommand(WORD id) override;

private:
    static const int TextReadSpeedMin = 0;  // 0 means "use game defaults"
    static const int TextReadSpeedMax = 30; // double of the default AGS "15"

    // Event handlers
    void OnEnableAccessCheck();
    void UpdateTextReadSpeed();

    // Dialog controls
    HWND _hEnableAccess = nullptr;
    HWND _hSpeechSkipStyle = nullptr;
    HWND _hTextSkipStyle = nullptr;
    HWND _hTextReadSpeed = nullptr;
    HWND _hTextReadSpeedText = nullptr;
    HWND _hSpeechMode = nullptr;
    HWND _hAlwaysWaitForText = nullptr;

    Common::ConfigTree &_cfgOut;
    bool _disabledSkipStyle = false;
};

} // namespace Engine
} // namespace AGS

#endif // AGS_PLATFORM_OS_WINDOWS
