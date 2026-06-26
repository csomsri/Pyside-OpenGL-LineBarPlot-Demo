#define NOMINMAX
#include <windows.h>
#include <GL/gl.h>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

HDC gDeviceContext = nullptr;
HGLRC gRenderContext = nullptr;
GLuint gFontBase = 0;
HFONT gFont = nullptr;
int gWindowWidth = 1000;
int gWindowHeight = 700;
DWORD gLastTick = 0;
float gElapsedSeconds = 0.0f;

const float plotLeft = 120.0f;
const float plotRight = 860.0f;
const float plotBottom = 140.0f;
const float plotTop = 560.0f;

const size_t sampleCount = 32;
const float sampleSpacingSeconds = 0.18f;
std::vector<float> barValues(sampleCount, 0.0f);
std::vector<float> lineValues(sampleCount, 0.0f);
const float yMax = 100.0f;

void setColor(float r, float g, float b) {
    glColor3f(r, g, b);
}

void setColorAlpha(float r, float g, float b, float a) {
    glColor4f(r, g, b, a);
}

SIZE measureText(const std::string& text) {
    SIZE size = {};
    GetTextExtentPoint32A(gDeviceContext, text.c_str(), static_cast<int>(text.size()), &size);
    return size;
}

void drawText(float x, float y, const std::string& text) {
    if (text.empty()) {
        return;
    }

    glRasterPos2f(x, y);
    glPushAttrib(GL_LIST_BIT);
    glListBase(gFontBase);
    glCallLists(static_cast<GLsizei>(text.size()), GL_UNSIGNED_BYTE, text.c_str());
    glPopAttrib();
}

void drawCenteredText(float x, float y, const std::string& text) {
    const SIZE size = measureText(text);
    drawText(x - size.cx / 2.0f, y, text);
}

float mapX(size_t index) {
    const float usableWidth = plotRight - plotLeft;
    const float step = usableWidth / static_cast<float>(sampleCount);
    return plotLeft + step * (static_cast<float>(index) + 0.5f);
}

float mapY(float value) {
    const float usableHeight = plotTop - plotBottom;
    return plotBottom + (value / yMax) * usableHeight;
}

void drawLine(float x1, float y1, float x2, float y2) {
    glBegin(GL_LINES);
    glVertex2f(x1, y1);
    glVertex2f(x2, y2);
    glEnd();
}

void drawRectangle(float x, float y, float width, float height) {
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width, y + height);
    glVertex2f(x, y + height);
    glEnd();
}

void drawRectangleOutline(float x, float y, float width, float height) {
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width, y + height);
    glVertex2f(x, y + height);
    glEnd();
}

void drawCircle(float cx, float cy, float radius) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= 32; ++i) {
        const float angle = static_cast<float>(i) * 2.0f * 3.14159265f / 32.0f;
        glVertex2f(cx + std::cos(angle) * radius, cy + std::sin(angle) * radius);
    }
    glEnd();
}

void updateLiveData(float deltaSeconds) {
    gElapsedSeconds += deltaSeconds;

    for (size_t i = 0; i < sampleCount; ++i) {
        const float age = static_cast<float>(sampleCount - 1 - i) * sampleSpacingSeconds;
        const float t = gElapsedSeconds - age;

        const float pulse = std::sin(t * 2.3f) * 9.0f;
        const float drift = std::sin(t * 0.47f) * 18.0f;
        const float jitter = std::sin(t * 8.9f + 1.7f) * 4.0f;
        const float lineValue = 58.0f + drift + pulse + jitter;

        const float blockWave = std::fabs(std::sin(t * 1.15f)) * 30.0f;
        const float barValue = 28.0f + blockWave + std::sin(t * 4.1f) * 7.0f;

        lineValues[i] = std::max(6.0f, std::min(98.0f, lineValue));
        barValues[i] = std::max(4.0f, std::min(96.0f, barValue));
    }
}

void drawBackground() {
    setColor(0.02f, 0.015f, 0.055f);
    drawRectangle(0.0f, 0.0f, 1000.0f, 700.0f);

    glLineWidth(1.0f);
    setColorAlpha(0.13f, 0.96f, 1.0f, 0.08f);
    for (int x = 0; x <= 1000; x += 40) {
        drawLine(static_cast<float>(x), 0.0f, static_cast<float>(x), 700.0f);
    }
    for (int y = 20; y <= 700; y += 40) {
        drawLine(0.0f, static_cast<float>(y), 1000.0f, static_cast<float>(y));
    }
}

void drawAxesAndGrid() {
    setColorAlpha(0.12f, 0.95f, 1.0f, 0.18f);
    glLineWidth(1.0f);

    for (int i = 0; i <= 4; ++i) {
        const float value = yMax * i / 4.0f;
        const float y = mapY(value);
        drawLine(plotLeft, y, plotRight, y);
    }

    for (size_t i = 0; i < sampleCount; ++i) {
        const float x = mapX(i);
        drawLine(x, plotBottom, x, plotTop);
    }

    setColorAlpha(0.0f, 0.95f, 1.0f, 0.38f);
    glLineWidth(7.0f);
    drawLine(plotLeft, plotBottom, plotRight, plotBottom);
    drawLine(plotLeft, plotBottom, plotLeft, plotTop);

    setColor(0.24f, 1.0f, 1.0f);
    glLineWidth(2.0f);
    drawLine(plotLeft, plotBottom, plotRight, plotBottom);
    drawLine(plotLeft, plotBottom, plotLeft, plotTop);

    setColor(0.82f, 0.97f, 1.0f);
    for (int i = 0; i <= 4; ++i) {
        const float value = yMax * i / 4.0f;
        const float y = mapY(value);
        drawLine(plotLeft - 6.0f, y, plotLeft, y);

        std::ostringstream tickLabel;
        tickLabel << static_cast<int>(value);
        drawText(plotLeft - 48.0f, y - 5.0f, tickLabel.str());
    }

    for (size_t i = 0; i < sampleCount; ++i) {
        const float x = mapX(i);
        drawLine(x, plotBottom, x, plotBottom - 6.0f);
        if (i % 6 == 0 || i + 1 == sampleCount) {
            std::ostringstream label;
            if (i + 1 == sampleCount) {
                label << "now";
            } else {
                label << "-" << std::fixed << std::setprecision(1)
                      << static_cast<float>(sampleCount - 1 - i) * sampleSpacingSeconds << "s";
            }
            drawCenteredText(x, plotBottom - 30.0f, label.str());
        }
    }
}

void drawBars() {
    const float step = (plotRight - plotLeft) / static_cast<float>(sampleCount);
    const float barWidth = step * 0.45f;

    for (size_t i = 0; i < barValues.size(); ++i) {
        const float x = mapX(i) - barWidth / 2.0f;
        const float y = mapY(barValues[i]);

        setColorAlpha(0.0f, 0.92f, 1.0f, 0.18f);
        drawRectangle(x - 4.0f, plotBottom, barWidth + 8.0f, y - plotBottom);

        setColor(0.0f, 0.84f, 1.0f);
        drawRectangle(x, plotBottom, barWidth, y - plotBottom);

        setColor(0.97f, 0.08f, 0.95f);
        glLineWidth(1.0f);
        drawRectangleOutline(x, plotBottom, barWidth, y - plotBottom);
    }
}

void drawLineGraph() {
    setColorAlpha(1.0f, 0.0f, 0.78f, 0.22f);
    glLineWidth(10.0f);
    glBegin(GL_LINE_STRIP);
    for (size_t i = 0; i < lineValues.size(); ++i) {
        glVertex2f(mapX(i), mapY(lineValues[i]));
    }
    glEnd();

    setColor(1.0f, 0.08f, 0.82f);
    glLineWidth(3.0f);
    glBegin(GL_LINE_STRIP);
    for (size_t i = 0; i < lineValues.size(); ++i) {
        glVertex2f(mapX(i), mapY(lineValues[i]));
    }
    glEnd();

    for (size_t i = 0; i < lineValues.size(); ++i) {
        setColorAlpha(1.0f, 0.0f, 0.78f, 0.25f);
        drawCircle(mapX(i), mapY(lineValues[i]), 11.0f);
        setColor(1.0f, 0.92f, 0.22f);
        drawCircle(mapX(i), mapY(lineValues[i]), 6.0f);
    }
}

void drawLegend() {
    const float x = 690.0f;
    const float y = 575.0f;

    setColorAlpha(0.04f, 0.02f, 0.12f, 0.86f);
    drawRectangle(x - 18.0f, y - 52.0f, 190.0f, 70.0f);

    setColor(0.0f, 0.95f, 1.0f);
    glLineWidth(1.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x - 18.0f, y - 52.0f);
    glVertex2f(x + 172.0f, y - 52.0f);
    glVertex2f(x + 172.0f, y + 18.0f);
    glVertex2f(x - 18.0f, y + 18.0f);
    glEnd();

    setColor(0.0f, 0.84f, 1.0f);
    drawRectangle(x, y - 7.0f, 28.0f, 14.0f);
    setColor(0.82f, 0.97f, 1.0f);
    drawText(x + 40.0f, y - 6.0f, "Data Packets");

    setColor(1.0f, 0.08f, 0.82f);
    glLineWidth(3.0f);
    drawLine(x, y - 34.0f, x + 28.0f, y - 34.0f);
    setColor(1.0f, 0.92f, 0.22f);
    drawCircle(x + 14.0f, y - 34.0f, 4.0f);
    setColor(0.82f, 0.97f, 1.0f);
    drawText(x + 40.0f, y - 40.0f, "Neural Load");
}

void renderScene() {
    glClearColor(0.02f, 0.015f, 0.055f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    drawBackground();
    drawAxesAndGrid();
    drawBars();
    drawLineGraph();
    drawLegend();

    setColor(1.0f, 0.92f, 0.22f);
    drawCenteredText((plotLeft + plotRight) / 2.0f, 620.0f, "NEON GRID // LIVE TELEMETRY");
    setColor(0.82f, 0.97f, 1.0f);
    drawCenteredText((plotLeft + plotRight) / 2.0f, 80.0f, "Streaming Time Window");
    drawText(42.0f, plotTop + 16.0f, "Signal");

    std::ostringstream status;
    status << "t+" << std::fixed << std::setprecision(1) << gElapsedSeconds << "s";
    drawText(plotLeft, 595.0f, status.str());

    SwapBuffers(gDeviceContext);
}

void resizeOpenGL(int width, int height) {
    gWindowWidth = width;
    gWindowHeight = height;

    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, 1000.0, 0.0, 700.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

bool setupOpenGL(HWND window) {
    gDeviceContext = GetDC(window);
    if (!gDeviceContext) {
        return false;
    }

    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.iLayerType = PFD_MAIN_PLANE;

    const int pixelFormat = ChoosePixelFormat(gDeviceContext, &pfd);
    if (pixelFormat == 0 || !SetPixelFormat(gDeviceContext, pixelFormat, &pfd)) {
        return false;
    }

    gRenderContext = wglCreateContext(gDeviceContext);
    if (!gRenderContext || !wglMakeCurrent(gDeviceContext, gRenderContext)) {
        return false;
    }

    gFont = CreateFontA(
        20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
        OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
        FF_DONTCARE | DEFAULT_PITCH, "Arial");
    SelectObject(gDeviceContext, gFont);

    gFontBase = glGenLists(256);
    wglUseFontBitmapsA(gDeviceContext, 0, 256, gFontBase);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    resizeOpenGL(gWindowWidth, gWindowHeight);
    return true;
}

void cleanupOpenGL(HWND window) {
    if (gFontBase != 0) {
        glDeleteLists(gFontBase, 256);
    }
    if (gFont) {
        DeleteObject(gFont);
    }
    if (gRenderContext) {
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(gRenderContext);
    }
    if (gDeviceContext) {
        ReleaseDC(window, gDeviceContext);
    }
}

LRESULT CALLBACK windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        if (!setupOpenGL(window)) {
            MessageBoxA(window, "Could not initialize OpenGL.", "OpenGL Error", MB_ICONERROR);
            return -1;
        }
        gLastTick = GetTickCount();
        updateLiveData(0.0f);
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_SIZE:
        resizeOpenGL(LOWORD(lParam), HIWORD(lParam));
        return 0;

    case WM_TIMER:
        {
            const DWORD now = GetTickCount();
            const float deltaSeconds = std::min((now - gLastTick) / 1000.0f, 0.05f);
            gLastTick = now;
            updateLiveData(deltaSeconds);
        }
        InvalidateRect(window, nullptr, FALSE);
        UpdateWindow(window);
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT paint = {};
        BeginPaint(window, &paint);
        renderScene();
        EndPaint(window, &paint);
        return 0;
    }

    case WM_CLOSE:
        DestroyWindow(window);
        return 0;

    case WM_DESTROY:
        KillTimer(window, 1);
        cleanupOpenGL(window);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcA(window, message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int showCommand) {
    const char className[] = "GraphOpenGlWindow";

    WNDCLASSA windowClass = {};
    windowClass.style = CS_OWNDC;
    windowClass.lpfnWndProc = windowProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.lpszClassName = className;

    if (!RegisterClassA(&windowClass)) {
        MessageBoxA(nullptr, "Could not register window class.", "Window Error", MB_ICONERROR);
        return 1;
    }

    HWND window = CreateWindowExA(
        0,
        className,
        "Cyberpunk OpenGL Live Graph",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        gWindowWidth,
        gWindowHeight,
        nullptr,
        nullptr,
        instance,
        nullptr);

    if (!window) {
        MessageBoxA(nullptr, "Could not create window.", "Window Error", MB_ICONERROR);
        return 1;
    }

    ShowWindow(window, showCommand);
    UpdateWindow(window);
    SetTimer(window, 1, 16, nullptr);

    MSG message = {};
    while (GetMessageA(&message, nullptr, 0, 0)) {
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }

    return static_cast<int>(message.wParam);
}
