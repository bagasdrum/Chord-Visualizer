#pragma once

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
    tresult PLUGIN_API checkSizeConstraint(ViewRect* rect) override;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    enum class Theme { Dark = 0, Light = 1, Transparent = 2 };

    void render(HWND hwnd, HDC hdc);
    void handleTimer();
    void showThemeMenu(int x, int y);
    void cycleTheme();
    void updateFonts(int width, int height);
    float currentScale() const;

    HWND mHwnd{nullptr};
    HWND mParentHwnd{nullptr};
    ChordVisualizerController* mController{nullptr};
    HFONT mFontChord{nullptr};
    HFONT mFontQuality{nullptr};
    HFONT mFontNotes{nullptr};
    HFONT mFontAnalysis{nullptr};
    Theme mTheme{Theme::Dark};

    static constexpr UINT kRefreshTimer = 1001;
    static constexpr int kBaseWidth = 500;
    static constexpr int kBaseHeight = 260;
    static constexpr int kMaxScale = 3;
};

} // namespace Vst
} // namespace Steinberg
