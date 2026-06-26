#define PY_SSIZE_T_CLEAN
#if defined(_MSC_VER) && defined(_DEBUG)
#define NATIVE_GRAPH_RESTORE_DEBUG
#undef _DEBUG
#endif
#include <Python.h>
#if defined(NATIVE_GRAPH_RESTORE_DEBUG)
#define _DEBUG
#undef NATIVE_GRAPH_RESTORE_DEBUG
#endif

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

#include <GL/gl.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace {

constexpr float kDesignWidth = 1000.0f;
constexpr float kDesignHeight = 700.0f;
constexpr float kPlotLeft = 120.0f;
constexpr float kPlotRight = 860.0f;
constexpr float kPlotBottom = 140.0f;
constexpr float kPlotTop = 560.0f;
constexpr size_t kSampleCount = 32;
constexpr float kSampleSpacingSeconds = 0.18f;
constexpr float kYMax = 100.0f;

float gElapsedSeconds = 0.0f;
std::vector<float> gBarValues(kSampleCount, 0.0f);
std::vector<float> gLineValues(kSampleCount, 0.0f);

using Glyph = std::vector<std::string>;

const std::map<char, Glyph> kFont = {
    {'A', {"01110", "10001", "10001", "11111", "10001", "10001", "10001"}},
    {'B', {"11110", "10001", "10001", "11110", "10001", "10001", "11110"}},
    {'C', {"01111", "10000", "10000", "10000", "10000", "10000", "01111"}},
    {'D', {"11110", "10001", "10001", "10001", "10001", "10001", "11110"}},
    {'E', {"11111", "10000", "10000", "11110", "10000", "10000", "11111"}},
    {'F', {"11111", "10000", "10000", "11110", "10000", "10000", "10000"}},
    {'G', {"01111", "10000", "10000", "10111", "10001", "10001", "01111"}},
    {'H', {"10001", "10001", "10001", "11111", "10001", "10001", "10001"}},
    {'I', {"11111", "00100", "00100", "00100", "00100", "00100", "11111"}},
    {'J', {"00111", "00010", "00010", "00010", "00010", "10010", "01100"}},
    {'K', {"10001", "10010", "10100", "11000", "10100", "10010", "10001"}},
    {'L', {"10000", "10000", "10000", "10000", "10000", "10000", "11111"}},
    {'M', {"10001", "11011", "10101", "10101", "10001", "10001", "10001"}},
    {'N', {"10001", "11001", "10101", "10011", "10001", "10001", "10001"}},
    {'O', {"01110", "10001", "10001", "10001", "10001", "10001", "01110"}},
    {'P', {"11110", "10001", "10001", "11110", "10000", "10000", "10000"}},
    {'Q', {"01110", "10001", "10001", "10001", "10101", "10010", "01101"}},
    {'R', {"11110", "10001", "10001", "11110", "10100", "10010", "10001"}},
    {'S', {"01111", "10000", "10000", "01110", "00001", "00001", "11110"}},
    {'T', {"11111", "00100", "00100", "00100", "00100", "00100", "00100"}},
    {'U', {"10001", "10001", "10001", "10001", "10001", "10001", "01110"}},
    {'V', {"10001", "10001", "10001", "10001", "10001", "01010", "00100"}},
    {'W', {"10001", "10001", "10001", "10101", "10101", "10101", "01010"}},
    {'X', {"10001", "10001", "01010", "00100", "01010", "10001", "10001"}},
    {'Y', {"10001", "10001", "01010", "00100", "00100", "00100", "00100"}},
    {'Z', {"11111", "00001", "00010", "00100", "01000", "10000", "11111"}},
    {'0', {"01110", "10001", "10011", "10101", "11001", "10001", "01110"}},
    {'1', {"00100", "01100", "00100", "00100", "00100", "00100", "01110"}},
    {'2', {"01110", "10001", "00001", "00010", "00100", "01000", "11111"}},
    {'3', {"11110", "00001", "00001", "01110", "00001", "00001", "11110"}},
    {'4', {"10010", "10010", "10010", "11111", "00010", "00010", "00010"}},
    {'5', {"11111", "10000", "10000", "11110", "00001", "00001", "11110"}},
    {'6', {"01110", "10000", "10000", "11110", "10001", "10001", "01110"}},
    {'7', {"11111", "00001", "00010", "00100", "01000", "01000", "01000"}},
    {'8', {"01110", "10001", "10001", "01110", "10001", "10001", "01110"}},
    {'9', {"01110", "10001", "10001", "01111", "00001", "00001", "01110"}},
    {'-', {"00000", "00000", "00000", "11111", "00000", "00000", "00000"}},
    {'+', {"00000", "00100", "00100", "11111", "00100", "00100", "00000"}},
    {'.', {"00000", "00000", "00000", "00000", "00000", "01100", "01100"}},
    {'/', {"00001", "00010", "00010", "00100", "01000", "01000", "10000"}},
    {':', {"00000", "01100", "01100", "00000", "01100", "01100", "00000"}},
};

void setColor(float r, float g, float b) {
    glColor3f(r, g, b);
}

void setColorAlpha(float r, float g, float b, float a) {
    glColor4f(r, g, b, a);
}

float mapX(size_t index) {
    const float step = (kPlotRight - kPlotLeft) / static_cast<float>(kSampleCount);
    return kPlotLeft + step * (static_cast<float>(index) + 0.5f);
}

float mapY(float value) {
    return kPlotBottom + (value / kYMax) * (kPlotTop - kPlotBottom);
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

float textWidth(const std::string& text, float scale) {
    return static_cast<float>(text.size()) * 6.0f * scale;
}

void drawText(float x, float y, std::string text, float scale = 3.0f) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });

    for (char c : text) {
        if (c == ' ') {
            x += 6.0f * scale;
            continue;
        }

        const auto glyphIt = kFont.find(c);
        const Glyph& glyph = glyphIt == kFont.end() ? kFont.at('-') : glyphIt->second;
        for (size_t row = 0; row < glyph.size(); ++row) {
            for (size_t col = 0; col < glyph[row].size(); ++col) {
                if (glyph[row][col] == '1') {
                    drawRectangle(x + static_cast<float>(col) * scale,
                                  y + static_cast<float>(6 - row) * scale,
                                  scale * 0.82f,
                                  scale * 0.82f);
                }
            }
        }
        x += 6.0f * scale;
    }
}

void drawCenteredText(float x, float y, const std::string& text, float scale = 3.0f) {
    drawText(x - textWidth(text, scale) / 2.0f, y, text, scale);
}

void updateLiveData(float deltaSeconds) {
    gElapsedSeconds += deltaSeconds;

    for (size_t i = 0; i < kSampleCount; ++i) {
        const float age = static_cast<float>(kSampleCount - 1 - i) * kSampleSpacingSeconds;
        const float t = gElapsedSeconds - age;

        const float pulse = std::sin(t * 2.3f) * 9.0f;
        const float drift = std::sin(t * 0.47f) * 18.0f;
        const float jitter = std::sin(t * 8.9f + 1.7f) * 4.0f;
        const float lineValue = 58.0f + drift + pulse + jitter;

        const float blockWave = std::fabs(std::sin(t * 1.15f)) * 30.0f;
        const float barValue = 28.0f + blockWave + std::sin(t * 4.1f) * 7.0f;

        gLineValues[i] = std::clamp(lineValue, 6.0f, 98.0f);
        gBarValues[i] = std::clamp(barValue, 4.0f, 96.0f);
    }
}

void drawBackground() {
    setColor(0.02f, 0.015f, 0.055f);
    drawRectangle(0.0f, 0.0f, kDesignWidth, kDesignHeight);

    glLineWidth(1.0f);
    setColorAlpha(0.13f, 0.96f, 1.0f, 0.08f);
    for (int x = 0; x <= 1000; x += 40) {
        drawLine(static_cast<float>(x), 0.0f, static_cast<float>(x), kDesignHeight);
    }
    for (int y = 20; y <= 700; y += 40) {
        drawLine(0.0f, static_cast<float>(y), kDesignWidth, static_cast<float>(y));
    }
}

void drawAxesAndGrid() {
    setColorAlpha(0.12f, 0.95f, 1.0f, 0.18f);
    glLineWidth(1.0f);

    for (int i = 0; i <= 4; ++i) {
        const float value = kYMax * i / 4.0f;
        drawLine(kPlotLeft, mapY(value), kPlotRight, mapY(value));
    }

    for (size_t i = 0; i < kSampleCount; ++i) {
        const float x = mapX(i);
        drawLine(x, kPlotBottom, x, kPlotTop);
    }

    setColorAlpha(0.0f, 0.95f, 1.0f, 0.38f);
    glLineWidth(7.0f);
    drawLine(kPlotLeft, kPlotBottom, kPlotRight, kPlotBottom);
    drawLine(kPlotLeft, kPlotBottom, kPlotLeft, kPlotTop);

    setColor(0.24f, 1.0f, 1.0f);
    glLineWidth(2.0f);
    drawLine(kPlotLeft, kPlotBottom, kPlotRight, kPlotBottom);
    drawLine(kPlotLeft, kPlotBottom, kPlotLeft, kPlotTop);

    setColor(0.82f, 0.97f, 1.0f);
    for (int i = 0; i <= 4; ++i) {
        const float value = kYMax * i / 4.0f;
        const float y = mapY(value);
        drawLine(kPlotLeft - 6.0f, y, kPlotLeft, y);

        std::ostringstream tickLabel;
        tickLabel << static_cast<int>(value);
        drawText(kPlotLeft - 50.0f, y - 6.0f, tickLabel.str(), 2.2f);
    }

    for (size_t i = 0; i < kSampleCount; ++i) {
        const float x = mapX(i);
        drawLine(x, kPlotBottom, x, kPlotBottom - 6.0f);
        if (i % 6 == 0 || i + 1 == kSampleCount) {
            std::ostringstream label;
            if (i + 1 == kSampleCount) {
                label << "now";
            } else {
                label << "-" << std::fixed << std::setprecision(1)
                      << static_cast<float>(kSampleCount - 1 - i) * kSampleSpacingSeconds << "s";
            }
            drawCenteredText(x, kPlotBottom - 32.0f, label.str(), 2.0f);
        }
    }
}

void drawBars() {
    const float step = (kPlotRight - kPlotLeft) / static_cast<float>(kSampleCount);
    const float barWidth = step * 0.45f;

    for (size_t i = 0; i < gBarValues.size(); ++i) {
        const float x = mapX(i) - barWidth / 2.0f;
        const float y = mapY(gBarValues[i]);

        setColorAlpha(0.0f, 0.92f, 1.0f, 0.18f);
        drawRectangle(x - 4.0f, kPlotBottom, barWidth + 8.0f, y - kPlotBottom);

        setColor(0.0f, 0.84f, 1.0f);
        drawRectangle(x, kPlotBottom, barWidth, y - kPlotBottom);

        setColor(0.97f, 0.08f, 0.95f);
        glLineWidth(1.0f);
        drawRectangleOutline(x, kPlotBottom, barWidth, y - kPlotBottom);
    }
}

void drawLineGraph() {
    setColorAlpha(1.0f, 0.0f, 0.78f, 0.22f);
    glLineWidth(10.0f);
    glBegin(GL_LINE_STRIP);
    for (size_t i = 0; i < gLineValues.size(); ++i) {
        glVertex2f(mapX(i), mapY(gLineValues[i]));
    }
    glEnd();

    setColor(1.0f, 0.08f, 0.82f);
    glLineWidth(3.0f);
    glBegin(GL_LINE_STRIP);
    for (size_t i = 0; i < gLineValues.size(); ++i) {
        glVertex2f(mapX(i), mapY(gLineValues[i]));
    }
    glEnd();

    for (size_t i = 0; i < gLineValues.size(); ++i) {
        setColorAlpha(1.0f, 0.0f, 0.78f, 0.25f);
        drawCircle(mapX(i), mapY(gLineValues[i]), 11.0f);
        setColor(1.0f, 0.92f, 0.22f);
        drawCircle(mapX(i), mapY(gLineValues[i]), 6.0f);
    }
}

void drawLegend() {
    const float x = 690.0f;
    const float y = 575.0f;

    setColorAlpha(0.04f, 0.02f, 0.12f, 0.86f);
    drawRectangle(x - 18.0f, y - 52.0f, 190.0f, 70.0f);

    setColor(0.0f, 0.95f, 1.0f);
    glLineWidth(1.0f);
    drawRectangleOutline(x - 18.0f, y - 52.0f, 190.0f, 70.0f);

    setColor(0.0f, 0.84f, 1.0f);
    drawRectangle(x, y - 3.0f, 28.0f, 14.0f);
    setColor(0.82f, 0.97f, 1.0f);
    drawText(x + 40.0f, y - 1.0f, "Data Packets", 2.0f);

    setColor(1.0f, 0.08f, 0.82f);
    glLineWidth(3.0f);
    drawLine(x, y - 28.0f, x + 28.0f, y - 28.0f);
    setColor(1.0f, 0.92f, 0.22f);
    drawCircle(x + 14.0f, y - 28.0f, 4.0f);
    setColor(0.82f, 0.97f, 1.0f);
    drawText(x + 40.0f, y - 34.0f, "Neural Load", 2.0f);
}

void renderGraph() {
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
    drawCenteredText((kPlotLeft + kPlotRight) / 2.0f, 620.0f, "NEON GRID // LIVE TELEMETRY", 3.2f);
    setColor(0.82f, 0.97f, 1.0f);
    drawCenteredText((kPlotLeft + kPlotRight) / 2.0f, 80.0f, "Streaming Time Window", 2.4f);
    drawText(42.0f, kPlotTop + 16.0f, "Signal", 2.4f);

    std::ostringstream status;
    status << "t+" << std::fixed << std::setprecision(1) << gElapsedSeconds << "s";
    drawText(kPlotLeft, 595.0f, status.str(), 2.2f);
}

PyObject* py_initialize(PyObject*, PyObject*) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    updateLiveData(0.0f);
    Py_RETURN_NONE;
}

PyObject* py_resize(PyObject*, PyObject* args) {
    int width = 0;
    int height = 0;
    if (!PyArg_ParseTuple(args, "ii", &width, &height)) {
        return nullptr;
    }

    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, kDesignWidth, 0.0, kDesignHeight, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    Py_RETURN_NONE;
}

PyObject* py_update(PyObject*, PyObject* args) {
    double deltaSeconds = 0.0;
    if (!PyArg_ParseTuple(args, "d", &deltaSeconds)) {
        return nullptr;
    }

    updateLiveData(static_cast<float>(std::clamp(deltaSeconds, 0.0, 0.05)));
    Py_RETURN_NONE;
}

PyObject* py_render(PyObject*, PyObject*) {
    renderGraph();
    Py_RETURN_NONE;
}

PyMethodDef kMethods[] = {
    {"initialize", py_initialize, METH_NOARGS, "Initialize OpenGL state for the current context."},
    {"resize", py_resize, METH_VARARGS, "Resize the OpenGL viewport."},
    {"update", py_update, METH_VARARGS, "Advance the simulated live data."},
    {"render", py_render, METH_NOARGS, "Render the graph into the current OpenGL context."},
    {nullptr, nullptr, 0, nullptr},
};

PyModuleDef kModule = {
    PyModuleDef_HEAD_INIT,
    "native_graph",
    "Native C++ OpenGL renderer for the PySide cyberpunk telemetry graph.",
    -1,
    kMethods,
};

} // namespace

PyMODINIT_FUNC PyInit_native_graph() {
    return PyModule_Create(&kModule);
}
