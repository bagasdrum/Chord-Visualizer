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
    tresult PLUGIN_API checkSizeConstraint(ViewRect* rect) override;

    static LRESULT CALLBACK WndProc(
        HWND hwnd,
        UINT msg,
        WPARAM wParam,
        LPARAM lParam
    );

private:
    void render(HWND hwnd, HDC hdc);
    void handleTimer();
    void showThemeMenu(int x, int y);

    HWND mHwnd{nullptr};
    HWND mParentHwnd{nullptr};

    ChordVisualizerController* mController{nullptr};

    static constexpr UINT kRefreshTimer = 1001;

    // Base design size
    static constexpr LONG kDesignWidth = 500;
    static constexpr LONG kDesignHeight = 280;

    // Minimum window size.
    // Width and height are independent.
    static constexpr LONG kMinWidth = 320;
    static constexpr LONG kMinHeight = 180;
};

} // namespace Vst
} // namespace Steinberg
