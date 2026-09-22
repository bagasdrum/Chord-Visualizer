#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "public.sdk/source/common/pluginview.h"
#include "ChordVisualizerShared.h"

#include <windows.h>

namespace Steinberg {
namespace Vst {

class ChordVisualizerController;

class ChordVisualizerView : public CPluginView {
public:
    ChordVisualizerView(ChordVisualizerController* controller);
    ~ChordVisualizerView() override;

    tresult PLUGIN_API isPlatformTypeSupported(FIDString type) override;
    tresult PLUGIN_API attached(void* parent, FIDString type) override;
    tresult PLUGIN_API removed() override;

    tresult PLUGIN_API getSize(ViewRect* size) override;
    tresult PLUGIN_API onSize(ViewRect* newSize) override;
    tresult PLUGIN_API canResize() override { return kResultTrue; }
    tresult PLUGIN_API checkSizeConstraint(ViewRect* size) override;

    static LRESULT CALLBACK WndProc(
        HWND hwnd,
        UINT msg,
        WPARAM wParam,
        LPARAM lParam
    );

private:
    enum class Theme {
        Dark,
        Light
    };

    void render(HWND hwnd, HDC hdc);
    void handleTimer();
    void showThemeMenu(HWND hwnd, int x, int y);

    void updateFonts(int width, int height);
    void destroyFonts();

    HWND mHwnd{nullptr};
    HWND mParentHwnd{nullptr};

    ChordVisualizerController* mController{nullptr};

    HFONT mFontChord{nullptr};
    HFONT mFontQuality{nullptr};
    HFONT mFontNotes{nullptr};
    HFONT mFontInfo{nullptr};
    HFONT mFontFormula{nullptr};

    Theme mTheme{Theme::Dark};

    static constexpr UINT kRefreshTimer = 1001;

    static constexpr int kBaseWidth = 500;
    static constexpr int kBaseHeight = 280;

    static constexpr LONG kMinWidth = 320;
    static constexpr LONG kMinHeight = 180;

    static constexpr UINT kThemeDark = 2001;
    static constexpr UINT kThemeLight = 2002;
};

} // namespace Vst
} // namespace Steinberg
