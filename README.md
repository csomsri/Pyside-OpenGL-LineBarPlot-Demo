# Cyberpunk OpenGL Graph

A Win32 OpenGL program and PySide OpenGL version that draw a cyberpunk-style live telemetry graph with simulated real-time data. It includes a bar plot, line graph, neon grid, title, axis labels, tick labels, status readout, and legend.

## Build

```powershell
cmake -S . -B build
cmake --build build
```

Run the generated `GraphOpenGl` executable from the build directory. The data updates automatically while the window is open.

## PySide OpenGL Version

The PySide version uses a dashboard-style main window with four framed `QOpenGLWidget` viewports on one page. A native C++ Python module named `native_graph` performs the OpenGL rendering. It does not use PyOpenGL.

Requirements:

```powershell
python -m pip install PySide6
```

Build the native module:

```powershell
python build_native.py
```

Run:

```powershell
python cyberpunk_pyside.py
```

If Python is not on your `PATH`, run both commands with the Python executable from your PySide environment. The native module must be built with the same Python version that runs `cyberpunk_pyside.py`.
# Pyside-OpenGL-LineBarPlot-Demo
