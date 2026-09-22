#include "ChordVisualizerView.h"
#include "ChordVisualizerController.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace Steinberg {
namespace Vst {

static const char* kWndClassName = "ChordScopeWin32Editor";
static bool g_classRegistered = false;

static COLORREF rgb(BYTE r, BYTE g, BYTE b) { return RGB(r, g, b); }

ChordVisualizerView::ChordVisualizerView(ChordVisualizerController* controller)
    : CPluginView(nullptr), mController(controller) {
    rect = ViewRect(0, 0, kBaseWidth, kBaseHeight);
}

ChordVisualizerView::~ChordVisualizerView() {
    if (mFontChord) DeleteObject(mFontChord);
    if (mFontQuality) DeleteObject(mFontQuality);
    if (mFontNotes) DeleteObject(mFontNotes);
    if (mFontAnalysis) DeleteObject(mFontAnalysis);
}

tresult PLUGIN_API ChordVisualizerView::isPlatformTypeSupported(FIDString type) {
    return strcmp(type, kPlatformTypeHWND) == 0 ? kResultTrue : kResultFalse;
}

tresult PLUGIN_API ChordVisualizerView::getSize(ViewRect* size) {
    if (!size) return kInvalidArgument;
    *size = rect;
    return kResultOk;
}

tresult PLUGIN_API ChordVisualizerView::checkSizeConstraint(ViewRect* r) {
    if (!r) return kInvalidArgument;

    // Keep the original 500x260 aspect ratio and snap to 1x / 2x / 3x.
    const int requestedW = std::max(kBaseWidth, r->getWidth());
    int scale = static_cast<int>(std::lround(static_cast<double>(requestedW) / kBaseWidth));
    scale = std::clamp(scale, 1, kMaxScale);

    const int w = kBaseWidth * scale;
    const int h = kBaseHeight * scale;
    *r = ViewRect(0, 0, w, h);
    return kResultOk;
}

tresult PLUGIN_API ChordVisualizerView::onSize(ViewRect* newSize) {
    if (!newSize) return kInvalidArgument;
    rect = *newSize;

    if (mHwnd) {
        SetWindowPos(mHwnd, nullptr, 0, 0, rect.getWidth(), rect.getHeight(),
                     SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
        updateFonts(rect.getWidth(), rect.getHeight());
        InvalidateRect(mHwnd, nullptr, FALSE);
    }
    return kResultOk;
}

float ChordVisualizerView::currentScale() const {
    return std::clamp(static_cast<float>(rect.getWidth()) / static_cast<float>(kBaseWidth),
                      1.0f, static_cast<float>(kMaxScale));
}

void ChordVisualizerView::updateFonts(int width, int height) {
    (void)height;
    const int s = std::max(1, static_cast<int>(std::lround(currentScale())));

    if (mFontChord) DeleteObject(mFontChord);
    if (mFontQuality) DeleteObject(mFontQuality);
    if (mFontNotes) DeleteObject(mFontNotes);
    if (mFontAnalysis) DeleteObject(mFontAnalysis);

    const int chordSize = 36 * s;
    const int qualitySize = 17 * s;
    const int notesSize = 13 * s;
    const int analysisSize = 11 * s;

    mFontChord = CreateFontA(-chordSize, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             CLEARTYPE_QUALITY, DEFAULT_PITCH, "Georgia");
    mFontQuality = CreateFontA(-qualitySize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               CLEARTYPE_QUALITY, DEFAULT_PITCH, "Georgia");
    mFontNotes = CreateFontA(-notesSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             CLEARTYPE_QUALITY, DEFAULT_PITCH, "Georgia");
    mFontAnalysis = CreateFontA(-analysisSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");
    (void)width;
}

void ChordVisualizerView::showThemeMenu(int x, int y) {
    HMENU menu = CreatePopupMenu();
    if (!menu) return;

    AppendMenuA(menu, MF_STRING | (mTheme == Theme::Dark ? MF_CHECKED : 0), 2001, "Dark");
    AppendMenuA(menu, MF_STRING | (mTheme == Theme::Light ? MF_CHECKED : 0), 2002, "Light");
    AppendMenuA(menu, MF_STRING | (mTheme == Theme::Transparent ? MF_CHECKED : 0), 2003, "Transparent");

    POINT pt{x, y};
    ClientToScreen(mHwnd, &pt);
    const UINT command = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, 0, mHwnd, nullptr);
    DestroyMenu(menu);

    switch (command) {
        case 2001: mTheme = Theme::Dark; break;
        case 2002: mTheme = Theme::Light; break;
        case 2003: mTheme = Theme::Transparent; break;
        default: return;
    }
    InvalidateRect(mHwnd, nullptr, FALSE);
}


void ChordVisualizerView::handleTimer() {
    if (mHwnd) InvalidateRect(mHwnd, nullptr, FALSE);
}

LRESULT CALLBACK ChordVisualizerView::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto* view = reinterpret_cast<ChordVisualizerView*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (msg) {
        case WM_CREATE: {
            auto* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
            view = reinterpret_cast<ChordVisualizerView*>(cs->lpCreateParams);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(view));
            SetTimer(hwnd, kRefreshTimer, 50, nullptr);
            return 0;
        }
        case WM_TIMER:
            if (view && wParam == kRefreshTimer) view->handleTimer();
            return 0;
        case WM_RBUTTONUP:
            if (view) view->showThemeMenu(static_cast<int>(static_cast<short>(LOWORD(lParam))),
                                          static_cast<int>(static_cast<short>(HIWORD(lParam))));
            return 0;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            if (view) view->render(hwnd, hdc);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_ERASEBKGND:
            return 1;
        case WM_DESTROY:
            KillTimer(hwnd, kRefreshTimer);
            return 0;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

tresult PLUGIN_API ChordVisualizerView::attached(void* parent, FIDString type) {
    if (strcmp(type, kPlatformTypeHWND) != 0) return kResultFalse;

    mParentHwnd = static_cast<HWND>(parent);
    HINSTANCE hInstance = GetModuleHandle(nullptr);

    if (!g_classRegistered) {
        WNDCLASSA wc{};
        wc.lpfnWndProc = WndProc;
        wc.hInstance = hInstance;
        wc.lpszClassName = kWndClassName;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = nullptr;
        if (!RegisterClassA(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return kResultFalse;
        g_classRegistered = true;
    }

    mHwnd = CreateWindowExA(
        WS_EX_TRANSPARENT, kWndClassName, "ChordScope",
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
        0, 0, rect.getWidth(), rect.getHeight(),
        mParentHwnd, nullptr, hInstance, this);

    if (!mHwnd) return kResultFalse;

    updateFonts(rect.getWidth(), rect.getHeight());
    return kResultOk;
}

tresult PLUGIN_API ChordVisualizerView::removed() {
    if (mHwnd) {
        KillTimer(mHwnd, kRefreshTimer);
        DestroyWindow(mHwnd);
        mHwnd = nullptr;
    }
    mParentHwnd = nullptr;
    return CPluginView::removed();
}

void ChordVisualizerView::render(HWND hwnd, HDC hdc) {
    RECT rc{};
    GetClientRect(hwnd, &rc);
    const int w = rc.right - rc.left;
    const int h = rc.bottom - rc.top;
    const float s = currentScale();
    const int pad = std::max(18, static_cast<int>(20.0f * s));

    HDC mem = CreateCompatibleDC(hdc);
    HBITMAP bmp = CreateCompatibleBitmap(hdc, w, h);
    HBITMAP oldBmp = static_cast<HBITMAP>(SelectObject(mem, bmp));

    const bool transparent = mTheme == Theme::Transparent;
    if (!transparent) {
        const COLORREF bg = (mTheme == Theme::Dark) ? rgb(24, 24, 27) : rgb(246, 244, 239);
        HBRUSH brush = CreateSolidBrush(bg);
        FillRect(mem, &rc, brush);
        DeleteObject(brush);
    } else {
        // Transparent mode: copy the host parent's background behind the view,
        // then draw only ChordScope's text over it. This avoids Unicode/alpha
        // rendering tricks and behaves like a transparent overlay in hosts
        // that paint their parent window normally.
        if (mParentHwnd) {
            POINT childPos{0, 0};
            ClientToScreen(hwnd, &childPos);
            ScreenToClient(mParentHwnd, &childPos);
            HDC parentDc = GetDC(mParentHwnd);
            if (parentDc) {
                BitBlt(mem, 0, 0, w, h, parentDc, childPos.x, childPos.y, SRCCOPY);
                ReleaseDC(mParentHwnd, parentDc);
            }
        }
    }

    const COLORREF primary = transparent
        ? rgb(245, 245, 245)
        : (mTheme == Theme::Dark ? rgb(245, 245, 247) : rgb(35, 34, 32));
    const COLORREF secondary = transparent
        ? rgb(215, 215, 220)
        : (mTheme == Theme::Dark ? rgb(165, 165, 175) : rgb(92, 88, 82));
    const COLORREF body = transparent
        ? rgb(232, 232, 235)
        : (mTheme == Theme::Dark ? rgb(215, 215, 220) : rgb(60, 57, 53));

    SetBkMode(mem, TRANSPARENT);
    SetTextColor(mem, primary);

    const bool active = g_sharedState.activeNotesCount.load() > 0;
    const char* chord = g_sharedState.chordName[0] ? g_sharedState.chordName : "-";
    const char* quality = active && g_sharedState.chordQuality[0] ? g_sharedState.chordQuality : "";
    const char* notes = active && g_sharedState.notesString[0] ? g_sharedState.notesString : "";

    // Book-like hierarchy: title, subtitle, body, then compact analysis.
    RECT chordRect{pad, static_cast<int>(32 * s), w - pad, static_cast<int>(82 * s)};
    SelectObject(mem, mFontChord);
    SetTextColor(mem, primary);
    DrawTextA(mem, chord, -1, &chordRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    RECT qualityRect{pad, static_cast<int>(88 * s), w - pad, static_cast<int>(116 * s)};
    SelectObject(mem, mFontQuality);
    SetTextColor(mem, secondary);
    DrawTextA(mem, quality, -1, &qualityRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    RECT notesRect{pad, static_cast<int>(132 * s), w - pad, static_cast<int>(158 * s)};
    SelectObject(mem, mFontNotes);
    SetTextColor(mem, body);
    DrawTextA(mem, notes, -1, &notesRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    char info[256]{};
    if (active) {
        std::snprintf(info, sizeof(info), "Root %s    Bass %s    %s    %s",
                      g_sharedState.rootNote[0] ? g_sharedState.rootNote : "-",
                      g_sharedState.bassNote[0] ? g_sharedState.bassNote : "-",
                      g_sharedState.inversionString[0] ? g_sharedState.inversionString : "-",
                      g_sharedState.intervalsString[0] ? g_sharedState.intervalsString : "-");
    }

    RECT infoRect{pad, static_cast<int>(184 * s), w - pad, static_cast<int>(212 * s)};
    SelectObject(mem, mFontAnalysis);
    SetTextColor(mem, secondary);
    DrawTextA(mem, info, -1, &infoRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);



    BitBlt(hdc, 0, 0, w, h, mem, 0, 0, SRCCOPY);
    SelectObject(mem, oldBmp);
    DeleteObject(bmp);
    DeleteDC(mem);
}

} // namespace Vst
} // namespace Steinberg
