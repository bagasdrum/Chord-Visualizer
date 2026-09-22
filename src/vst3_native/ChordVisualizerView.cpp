#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "ChordVisualizerView.h"
#include "ChordVisualizerController.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cctype>

namespace Steinberg {
namespace Vst {

static const char* kWndClassName = "ChordScopeWin32Editor";
static bool g_classRegistered = false;

enum : UINT {
    kThemeDark = 40001,
    kThemeLight = 40002,
};

// -----------------------------------------------------------------------------
// Constructor / Destructor
// -----------------------------------------------------------------------------

ChordVisualizerView::ChordVisualizerView(ChordVisualizerController* controller)
    : CPluginView(nullptr)
    , mController(controller)
{
    rect = ViewRect(0, 0, kDesignWidth, kDesignHeight);
}

ChordVisualizerView::~ChordVisualizerView() = default;

// -----------------------------------------------------------------------------
// Platform
// -----------------------------------------------------------------------------

tresult PLUGIN_API ChordVisualizerView::isPlatformTypeSupported(FIDString type)
{
    return strcmp(type, kPlatformTypeHWND) == 0
        ? kResultTrue
        : kResultFalse;
}

// -----------------------------------------------------------------------------
// Size
// -----------------------------------------------------------------------------

tresult PLUGIN_API ChordVisualizerView::getSize(ViewRect* size)
{
    if (!size)
        return kInvalidArgument;

    *size = rect;
    return kResultOk;
}

tresult PLUGIN_API ChordVisualizerView::checkSizeConstraint(ViewRect* r)
{
    if (!r)
        return kInvalidArgument;

    // FREE RESIZE:
    // No aspect-ratio locking.
    // Only enforce minimum size.

    const LONG width = std::max<LONG>(
        r->getWidth(),
        kMinWidth
    );

    const LONG height = std::max<LONG>(
        r->getHeight(),
        kMinHeight
    );

    *r = ViewRect(0, 0, width, height);

    return kResultOk;
}

tresult PLUGIN_API ChordVisualizerView::onSize(ViewRect* newSize)
{
    if (!newSize)
        return kInvalidArgument;

    rect = *newSize;

    if (mHwnd)
    {
        SetWindowPos(
            mHwnd,
            nullptr,
            0,
            0,
            rect.getWidth(),
            rect.getHeight(),
            SWP_NOZORDER | SWP_NOMOVE
        );

        InvalidateRect(
            mHwnd,
            nullptr,
            FALSE
        );
    }

    return kResultOk;
}

// -----------------------------------------------------------------------------
// Timer
// -----------------------------------------------------------------------------

void ChordVisualizerView::handleTimer()
{
    if (mHwnd)
        InvalidateRect(mHwnd, nullptr, FALSE);
}

// -----------------------------------------------------------------------------
// Theme menu
// -----------------------------------------------------------------------------

void ChordVisualizerView::showThemeMenu(int x, int y)
{
    if (!mHwnd)
        return;

    HMENU menu = CreatePopupMenu();

    if (!menu)
        return;

    const bool light =
        g_sharedState.isLightTheme.load();

    AppendMenuA(
        menu,
        MF_STRING | (!light ? MF_CHECKED : 0),
        kThemeDark,
        "Dark"
    );

    AppendMenuA(
        menu,
        MF_STRING | (light ? MF_CHECKED : 0),
        kThemeLight,
        "Light"
    );

    POINT pt{x, y};

    ClientToScreen(
        mHwnd,
        &pt
    );

    SetForegroundWindow(mHwnd);

    const UINT command =
        TrackPopupMenu(
            menu,
            TPM_RIGHTBUTTON | TPM_RETURNCMD,
            pt.x,
            pt.y,
            0,
            mHwnd,
            nullptr
        );

    if (command == kThemeDark)
    {
        g_sharedState.isLightTheme.store(false);

        InvalidateRect(
            mHwnd,
            nullptr,
            FALSE
        );
    }
    else if (command == kThemeLight)
    {
        g_sharedState.isLightTheme.store(true);

        InvalidateRect(
            mHwnd,
            nullptr,
            FALSE
        );
    }

    DestroyMenu(menu);
}

// -----------------------------------------------------------------------------
// Window procedure
// -----------------------------------------------------------------------------

LRESULT CALLBACK ChordVisualizerView::WndProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    auto* view =
        reinterpret_cast<ChordVisualizerView*>(
            GetWindowLongPtr(
                hwnd,
                GWLP_USERDATA
            )
        );

    switch (msg)
    {
        case WM_CREATE:
        {
            auto* cs =
                reinterpret_cast<CREATESTRUCT*>(
                    lParam
                );

            view =
                reinterpret_cast<ChordVisualizerView*>(
                    cs->lpCreateParams
                );

            SetWindowLongPtr(
                hwnd,
                GWLP_USERDATA,
                reinterpret_cast<LONG_PTR>(view)
            );

            SetTimer(
                hwnd,
                kRefreshTimer,
                50,
                nullptr
            );

            return 0;
        }

        case WM_TIMER:
        {
            if (view && wParam == kRefreshTimer)
                view->handleTimer();

            return 0;
        }

        case WM_RBUTTONUP:
        {
            if (view)
            {
                const int x =
                    static_cast<int>(
                        static_cast<short>(
                            LOWORD(lParam)
                        )
                    );

                const int y =
                    static_cast<int>(
                        static_cast<short>(
                            HIWORD(lParam)
                        )
                    );

                view->showThemeMenu(
                    x,
                    y
                );
            }

            return 0;
        }

        case WM_PAINT:
        {
            PAINTSTRUCT ps;

            HDC hdc =
                BeginPaint(
                    hwnd,
                    &ps
                );

            if (view)
                view->render(
                    hwnd,
                    hdc
                );

            EndPaint(
                hwnd,
                &ps
            );

            return 0;
        }

        case WM_ERASEBKGND:
            return 1;

        case WM_DESTROY:
        {
            KillTimer(
                hwnd,
                kRefreshTimer
            );

            return 0;
        }

        default:
            return DefWindowProc(
                hwnd,
                msg,
                wParam,
                lParam
            );
    }
}

// -----------------------------------------------------------------------------
// Attach
// -----------------------------------------------------------------------------

tresult PLUGIN_API ChordVisualizerView::attached(
    void* parent,
    FIDString type)
{
    if (strcmp(type, kPlatformTypeHWND) != 0)
        return kResultFalse;

    mParentHwnd =
        static_cast<HWND>(parent);

    HINSTANCE hInstance =
        GetModuleHandle(nullptr);

    if (!g_classRegistered)
    {
        WNDCLASSA wc{};

        wc.lpfnWndProc =
            WndProc;

        wc.hInstance =
            hInstance;

        wc.lpszClassName =
            kWndClassName;

        wc.hCursor =
            LoadCursor(
                nullptr,
                IDC_ARROW
            );

        wc.hbrBackground =
            nullptr;

        if (!RegisterClassA(
                &wc) &&
            GetLastError() !=
                ERROR_CLASS_ALREADY_EXISTS)
        {
            return kResultFalse;
        }

        g_classRegistered = true;
    }

    mHwnd =
        CreateWindowExA(
            0,
            kWndClassName,
            "ChordScope",
            WS_CHILD |
            WS_VISIBLE |
            WS_CLIPCHILDREN,
            0,
            0,
            kDesignWidth,
            kDesignHeight,
            mParentHwnd,
            nullptr,
            hInstance,
            this
        );

    if (!mHwnd)
        return kResultFalse;

    return kResultOk;
}

// -----------------------------------------------------------------------------
// Remove
// -----------------------------------------------------------------------------

tresult PLUGIN_API ChordVisualizerView::removed()
{
    if (mHwnd)
    {
        KillTimer(
            mHwnd,
            kRefreshTimer
        );

        DestroyWindow(
            mHwnd
        );

        mHwnd = nullptr;
    }

    mParentHwnd = nullptr;

    return CPluginView::removed();
}

// -----------------------------------------------------------------------------
// Render
// -----------------------------------------------------------------------------

void ChordVisualizerView::render(
    HWND hwnd,
    HDC hdc)
{
    RECT rc{};

    GetClientRect(
        hwnd,
        &rc
    );

    const int width =
        std::max<LONG>(
            1,
            rc.right
        );

    const int height =
        std::max<LONG>(
            1,
            rc.bottom
        );

    // -------------------------------------------------------------------------
    // Double buffering
    // -------------------------------------------------------------------------

    HDC mem =
        CreateCompatibleDC(
            hdc
        );

    if (!mem)
        return;

    HBITMAP bmp =
        CreateCompatibleBitmap(
            hdc,
            width,
            height
        );

    if (!bmp)
    {
        DeleteDC(mem);
        return;
    }

    HBITMAP oldBmp =
        static_cast<HBITMAP>(
            SelectObject(
                mem,
                bmp
            )
        );

    // -------------------------------------------------------------------------
    // Theme
    // -------------------------------------------------------------------------

    const bool light =
        g_sharedState.isLightTheme.load();

    // Dark:
    // Soft near-black.
    //
    // Light:
    // Soft gray instead of harsh white.
    const COLORREF bgColor =
        light
        ? RGB(224, 225, 228)
        : RGB(18, 18, 18);

    const COLORREF mainColor =
        light
        ? RGB(28, 29, 32)
        : RGB(244, 244, 246);

    const COLORREF subColor =
        light
        ? RGB(92, 94, 100)
        : RGB(142, 142, 147);

    HBRUSH bg =
        CreateSolidBrush(
            bgColor
        );

    FillRect(
        mem,
        &rc,
        bg
    );

    DeleteObject(bg);

    SetBkMode(
        mem,
        TRANSPARENT
    );

    // -------------------------------------------------------------------------
    // Responsive layout
    // -------------------------------------------------------------------------
    //
    // The design reference is 500 x 280.
    //
    // Unlike the previous implementation, this does NOT letterbox the design
    // into a fixed aspect ratio.
    //
    // The text remains centered in the actual plugin window.
    //

    const double widthRatio =
        static_cast<double>(width) /
        static_cast<double>(kDesignWidth);

    const double heightRatio =
        static_cast<double>(height) /
        static_cast<double>(kDesignHeight);

    // Typography responds mainly to the smaller dimension so that an extremely
    // wide but short window does not create enormous text.
    const double scale =
        std::clamp(
            std::min(
                widthRatio,
                heightRatio
            ),
            0.70,
            2.50
        );

    // -------------------------------------------------------------------------
    // Current chord information
    // -------------------------------------------------------------------------

    const bool active =
        g_sharedState.activeNotesCount.load() > 0;

    const char* chord =
        active &&
        g_sharedState.chordName[0]
        ? g_sharedState.chordName
        : "-";

    const char* quality =
        active &&
        g_sharedState.chordQuality[0]
        ? g_sharedState.chordQuality
        : "";

    const char* notes =
        active &&
        g_sharedState.notesString[0]
        ? g_sharedState.notesString
        : "";

    const char* root =
        active &&
        g_sharedState.rootNote[0]
        ? g_sharedState.rootNote
        : "";

    const char* bass =
        active &&
        g_sharedState.bassNote[0]
        ? g_sharedState.bassNote
        : "";

    const char* inversion =
        active &&
        g_sharedState.inversionString[0]
        ? g_sharedState.inversionString
        : "";

    const char* formula =
        active &&
        g_sharedState.intervalsString[0]
        ? g_sharedState.intervalsString
        : "";

    // -------------------------------------------------------------------------
    // Font helper
    // -------------------------------------------------------------------------

    auto fontSize =
        [&](double baseSize) -> int
    {
        return std::max(
            8,
            static_cast<int>(
                std::lround(
                    baseSize * scale
                )
            )
        );
    };

    auto makeFont =
        [&](double size,
            int weight,
            const char* family) -> HFONT
    {
        return CreateFontA(
            -fontSize(size),
            0,
            0,
            0,
            weight,
            FALSE,
            FALSE,
            FALSE,
            DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE,
            family
        );
    };

    // -------------------------------------------------------------------------
    // Typography
    // -------------------------------------------------------------------------
    //
    // Main chord:
    //   large, bold, clean
    //
    // Quality:
    //   small uppercase
    //
    // Notes:
    //   medium, light
    //
    // Analysis:
    //   small muted text
    //

    HFONT fontChord =
        makeFont(
            58.0,
            FW_BOLD,
            "Segoe UI"
        );

    HFONT fontQuality =
        makeFont(
            11.0,
            FW_SEMIBOLD,
            "Segoe UI"
        );

    HFONT fontNotes =
        makeFont(
            17.0,
            FW_NORMAL,
            "Segoe UI"
        );

    HFONT fontMeta =
        makeFont(
            11.0,
            FW_NORMAL,
            "Segoe UI"
        );

    HFONT fontFormula =
        makeFont(
            11.0,
            FW_NORMAL,
            "Segoe UI"
        );

    // -------------------------------------------------------------------------
    // Helpers
    // -------------------------------------------------------------------------

    auto centerRect =
        [&](double top,
            double bottom) -> RECT
    {
        const int margin =
            std::max(
                20,
                static_cast<int>(
                    std::lround(
                        30.0 * scale
                    )
                )
            );

        RECT r{};

        r.left =
            margin;

        r.right =
            width - margin;

        r.top =
            static_cast<int>(
                std::lround(
                    top * height / kDesignHeight
                )
            );

        r.bottom =
            static_cast<int>(
                std::lround(
                    bottom * height / kDesignHeight
                )
            );

        return r;
    };

    auto drawCentered =
        [&](HFONT font,
            COLORREF color,
            const char* text,
            double top,
            double bottom,
            int characterExtra = 0)
    {
        if (!text || !text[0])
            return;

        SelectObject(
            mem,
            font
        );

        SetTextColor(
            mem,
            color
        );

        SetTextCharacterExtra(
            mem,
            static_cast<int>(
                std::lround(
                    characterExtra * scale
                )
            )
        );

        RECT r =
            centerRect(
                top,
                bottom
            );

        DrawTextA(
            mem,
            text,
            -1,
            &r,
            DT_CENTER |
            DT_VCENTER |
            DT_SINGLELINE |
            DT_NOPREFIX
        );

        SetTextCharacterExtra(
            mem,
            0
        );
    };

    // -------------------------------------------------------------------------
    // Prepare quality text
    // -------------------------------------------------------------------------

    char qualityUpper[128]{};

    if (quality[0])
    {
        std::strncpy(
            qualityUpper,
            quality,
            sizeof(qualityUpper) - 1
        );

        for (char* p = qualityUpper;
             *p;
             ++p)
        {
            *p =
                static_cast<char>(
                    std::toupper(
                        static_cast<unsigned char>(
                            *p
                        )
                    )
                );
        }
    }

    // -------------------------------------------------------------------------
    // Prepare notes
    // -------------------------------------------------------------------------

    char notesClean[128]{};

    if (notes[0])
    {
        std::strncpy(
            notesClean,
            notes,
            sizeof(notesClean) - 1
        );

        // Replace hyphen separators with centered dot.
        //
        // This is intentionally conservative:
        // if the source already contains "·", it is preserved.
        //
        for (char* p = notesClean;
             *p;
             ++p)
        {
            if (*p == '-')
                *p = ' ';
        }
    }

    // -------------------------------------------------------------------------
    // Prepare analysis line
    // -------------------------------------------------------------------------

    char meta[256]{};

    if (active)
    {
        if (inversion[0])
        {
            std::snprintf(
                meta,
                sizeof(meta),
                "Root %s  ·  Bass %s  ·  %s",
                root,
                bass,
                inversion
            );
        }
        else
        {
            std::snprintf(
                meta,
                sizeof(meta),
                "Root %s  ·  Bass %s",
                root,
                bass
            );
        }
    }

    // -------------------------------------------------------------------------
    // Layout
    // -------------------------------------------------------------------------
    //
    // The vertical positions are based on the 500 x 280 design.
    //
    // We deliberately keep the groups closer together than the old version.
    //

    // 1. Main chord
    drawCentered(
        fontChord,
        mainColor,
        chord,
        30,
        98
    );

    // 2. Chord quality
    drawCentered(
        fontQuality,
        subColor,
        qualityUpper,
        102,
        128,
        2
    );

    // 3. Notes
    drawCentered(
        fontNotes,
        mainColor,
        notesClean,
        139,
        177,
        1
    );

    // 4. Root / Bass / Position
    drawCentered(
        fontMeta,
        subColor,
        meta,
        205,
        230,
        0
    );

    // 5. Formula
    drawCentered(
        fontFormula,
        subColor,
        formula,
        232,
        258,
        1
    );

    // -------------------------------------------------------------------------
    // Cleanup
    // -------------------------------------------------------------------------

    DeleteObject(fontChord);
    DeleteObject(fontQuality);
    DeleteObject(fontNotes);
    DeleteObject(fontMeta);
    DeleteObject(fontFormula);

    BitBlt(
        hdc,
        0,
        0,
        width,
        height,
        mem,
        0,
        0,
        SRCCOPY
    );

    SelectObject(
        mem,
        oldBmp
    );

    DeleteObject(bmp);
    DeleteDC(mem);
}

} // namespace Vst
} // namespace Steinberg
