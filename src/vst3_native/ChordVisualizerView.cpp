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

enum : UINT
{
    kThemeDark = 40001,
    kThemeLight = 40002,
};

// -----------------------------------------------------------------------------
// Constructor / Destructor
// -----------------------------------------------------------------------------

ChordVisualizerView::ChordVisualizerView(
    ChordVisualizerController* controller)
    : CPluginView(nullptr)
    , mController(controller)
{
    rect = ViewRect(
        0,
        0,
        kDesignWidth,
        kDesignHeight
    );
}

ChordVisualizerView::~ChordVisualizerView() = default;

// -----------------------------------------------------------------------------
// Platform
// -----------------------------------------------------------------------------

tresult PLUGIN_API
ChordVisualizerView::isPlatformTypeSupported(FIDString type)
{
    return strcmp(type, kPlatformTypeHWND) == 0
        ? kResultTrue
        : kResultFalse;
}

// -----------------------------------------------------------------------------
// Size
// -----------------------------------------------------------------------------

tresult PLUGIN_API
ChordVisualizerView::getSize(ViewRect* size)
{
    if (!size)
        return kInvalidArgument;

    *size = rect;

    return kResultOk;
}

tresult PLUGIN_API
ChordVisualizerView::checkSizeConstraint(ViewRect* r)
{
    if (!r)
        return kInvalidArgument;

    const LONG width =
        std::max<LONG>(
            r->getWidth(),
            kMinWidth
        );

    const LONG height =
        std::max<LONG>(
            r->getHeight(),
            kMinHeight
        );

    *r = ViewRect(
        0,
        0,
        width,
        height
    );

    return kResultOk;
}

tresult PLUGIN_API
ChordVisualizerView::onSize(ViewRect* newSize)
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
        InvalidateRect(
            mHwnd,
            nullptr,
            FALSE
        );
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
        MF_STRING |
        (!light ? MF_CHECKED : 0),
        kThemeDark,
        "Dark"
    );

    AppendMenuA(
        menu,
        MF_STRING |
        (light ? MF_CHECKED : 0),
        kThemeLight,
        "Light"
    );

    POINT pt{};
    pt.x = x;
    pt.y = y;

    ClientToScreen(
        mHwnd,
        &pt
    );

    SetForegroundWindow(mHwnd);

    UINT command =
        TrackPopupMenu(
            menu,
            TPM_RIGHTBUTTON |
            TPM_RETURNCMD,
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

LRESULT CALLBACK
ChordVisualizerView::WndProc(
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
            if (view &&
                wParam == kRefreshTimer)
            {
                view->handleTimer();
            }

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
            PAINTSTRUCT ps{};

            HDC hdc =
                BeginPaint(
                    hwnd,
                    &ps
                );

            if (view)
            {
                view->render(
                    hwnd,
                    hdc
                );
            }

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

tresult PLUGIN_API
ChordVisualizerView::attached(
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

        if (!RegisterClassA(&wc) &&
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

tresult PLUGIN_API
ChordVisualizerView::removed()
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
        CreateCompatibleDC(hdc);

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
    // Responsive scale
    // -------------------------------------------------------------------------

    const double widthRatio =
        static_cast<double>(width) /
        static_cast<double>(kDesignWidth);

    const double heightRatio =
        static_cast<double>(height) /
        static_cast<double>(kDesignHeight);

    const double scale =
        std::clamp(
            std::min(
                widthRatio,
                heightRatio
            ),
            0.70,
            2.00
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
    // Font helpers
    // -------------------------------------------------------------------------

    auto fontSize =
        [&](double base) -> int
    {
        return std::max(
            8,
            static_cast<int>(
                std::lround(
                    base * scale
                )
            )
        );
    };

    auto makeFont =
        [&](double size,
            int weight) -> HFONT
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
            "Segoe UI"
        );
    };

    HFONT fontChord =
        makeFont(
            58.0,
            FW_BOLD
        );

    HFONT fontQuality =
        makeFont(
            11.0,
            FW_SEMIBOLD
        );

    HFONT fontNotes =
        makeFont(
            17.0,
            FW_NORMAL
        );

    HFONT fontMeta =
        makeFont(
            10.0,
            FW_NORMAL
        );

    HFONT fontFormula =
        makeFont(
            11.0,
            FW_NORMAL
        );

    // -------------------------------------------------------------------------
    // Text area
    // -------------------------------------------------------------------------

    const int sideMargin =
        std::max(
            24,
            static_cast<int>(
                std::lround(
                    30.0 * scale
                )
            )
        );

    auto makeRect =
        [&](double top,
            double bottom) -> RECT
    {
        RECT r{};

        r.left =
            sideMargin;

        r.right =
            width - sideMargin;

        r.top =
            static_cast<int>(
                std::lround(
                    top *
                    height /
                    static_cast<double>(
                        kDesignHeight
                    )
                )
            );

        r.bottom =
            static_cast<int>(
                std::lround(
                    bottom *
                    height /
                    static_cast<double>(
                        kDesignHeight
                    )
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
            int characterExtra)
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
            makeRect(
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
    // Prepare quality
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
    //
    // IMPORTANT:
    // ASCII only. No Unicode bullet / flat / sharp characters.
    //
    // Existing separators are normalized to " . ".
    // -------------------------------------------------------------------------

    char notesClean[128]{};

    if (notes[0])
    {
        std::strncpy(
            notesClean,
            notes,
            sizeof(notesClean) - 1
        );

        char temp[128]{};
        int out = 0;

        for (int i = 0;
             notesClean[i] &&
             out < static_cast<int>(
                 sizeof(temp) - 1
             );
             ++i)
        {
            const char c =
                notesClean[i];

            if (c == '-')
            {
                temp[out++] = ' ';
                temp[out++] = '.';
                temp[out++] = ' ';
            }
            else
            {
                temp[out++] = c;
            }

            if (out >=
                static_cast<int>(
                    sizeof(temp) - 1
                ))
            {
                break;
            }
        }

        temp[out] = '\0';

        std::strncpy(
            notesClean,
            temp,
            sizeof(notesClean) - 1
        );
    }

    // -------------------------------------------------------------------------
    // Prepare metadata
    // -------------------------------------------------------------------------

    char meta[256]{};

    if (active)
    {
        if (inversion[0])
        {
            std::snprintf(
                meta,
                sizeof(meta),
                "Root %s    Bass %s    %s",
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
                "Root %s    Bass %s",
                root,
                bass
            );
        }
    }

    // -------------------------------------------------------------------------
    // Prepare formula
    //
    // ASCII-safe:
    //   1 - b3 - 5
    // becomes:
    //   1 . b3 . 5
    // -------------------------------------------------------------------------

    char formulaClean[128]{};

    if (formula[0])
    {
        int out = 0;

        for (int i = 0;
             formula[i] &&
             out < static_cast<int>(
                 sizeof(formulaClean) - 1
             );
             ++i)
        {
            const char c =
                formula[i];

            if (c == '-')
            {
                // Avoid duplicate spaces around separator.
                while (out > 0 &&
                       formulaClean[out - 1] == ' ')
                {
                    --out;
                }

                if (out > 0)
                    formulaClean[out++] = ' ';

                formulaClean[out++] = '.';
                formulaClean[out++] = ' ';
            }
            else
            {
                formulaClean[out++] = c;
            }
        }

        while (out > 0 &&
               formulaClean[out - 1] == ' ')
        {
            --out;
        }

        formulaClean[out] = '\0';
    }

    // -------------------------------------------------------------------------
    // Layout
    //
    // 500 x 280 reference:
    //
    //             Am
    //
    //         MINOR TRIAD
    //
    //         A3 . C4 . E4
    //
    //      Root A    Bass A
    //       Root Position
    //
    //          1 . b3 . 5
    //
    // -------------------------------------------------------------------------

    // Main chord
    drawCentered(
        fontChord,
        mainColor,
        chord,
        31,
        87,
        0
    );

    // Quality
    drawCentered(
        fontQuality,
        subColor,
        qualityUpper,
        91,
        114,
        2
    );

    // Notes
    drawCentered(
        fontNotes,
        mainColor,
        notesClean,
        120,
        148,
        0
    );

    // Analysis
    drawCentered(
        fontMeta,
        subColor,
        meta,
        169,
        190,
        0
    );

    // Formula
    drawCentered(
        fontFormula,
        subColor,
        formulaClean,
        197,
        220,
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
