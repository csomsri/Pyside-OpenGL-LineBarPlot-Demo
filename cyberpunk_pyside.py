import sys
import importlib.util
import subprocess
import sysconfig
from pathlib import Path
from time import perf_counter

ROOT = Path(__file__).resolve().parent
for module_dir in (ROOT / "build" / "Debug", ROOT / "build" / "Release", ROOT / "build"):
    if module_dir.exists():
        sys.path.insert(0, str(module_dir))


def native_graph_candidates():
    return sorted(ROOT.glob("build/**/native_graph*.pyd"))


def build_native_graph_for_current_python():
    configure = subprocess.run(
        [
            "cmake",
            "-S",
            str(ROOT),
            "-B",
            str(ROOT / "build"),
            f"-DPython_EXECUTABLE={sys.executable}",
        ],
        cwd=ROOT,
        text=True,
        capture_output=True,
    )
    if configure.returncode != 0:
        return False, configure.stdout + configure.stderr

    build = subprocess.run(
        ["cmake", "--build", str(ROOT / "build"), "--config", "Release"],
        cwd=ROOT,
        text=True,
        capture_output=True,
    )
    return build.returncode == 0, build.stdout + build.stderr


def load_native_graph():
    try:
        import native_graph
        return native_graph
    except ModuleNotFoundError:
        pass

    extension_suffix = sysconfig.get_config_var("EXT_SUFFIX") or ".pyd"
    candidates = native_graph_candidates()
    compatible = [path for path in candidates if path.name.endswith(extension_suffix)]

    if not compatible:
        built, build_log = build_native_graph_for_current_python()
        if built:
            candidates = native_graph_candidates()
            compatible = [path for path in candidates if path.name.endswith(extension_suffix)]

    if not compatible:
        found = ", ".join(str(path.relative_to(ROOT)) for path in candidates) or "none"
        raise ModuleNotFoundError(
            "Could not find a native_graph module compatible with this Python.\n"
            f"Python wants extension suffix: {extension_suffix}\n"
            f"Found native modules: {found}\n"
            "I tried to rebuild it automatically with this Python and could not find a matching module.\n"
            "Manual rebuild:\n"
            f"  cmake -S . -B build -DPython_EXECUTABLE=\"{sys.executable}\"\n"
            "  cmake --build build --config Release\n\n"
            "Build output:\n"
            f"{build_log if 'build_log' in locals() else '(auto-build was not attempted)'}"
        )

    module_path = compatible[-1]
    spec = importlib.util.spec_from_file_location("native_graph", module_path)
    if spec is None or spec.loader is None:
        raise ImportError(f"Could not create an import spec for {module_path}")

    module = importlib.util.module_from_spec(spec)
    sys.modules["native_graph"] = module
    spec.loader.exec_module(module)
    return module


native_graph = load_native_graph()

from PySide6.QtCore import QTimer
from PySide6.QtGui import QSurfaceFormat
from PySide6.QtOpenGLWidgets import QOpenGLWidget
from PySide6.QtWidgets import (
    QApplication,
    QFrame,
    QGridLayout,
    QHBoxLayout,
    QLabel,
    QMainWindow,
    QPushButton,
    QVBoxLayout,
    QWidget,
)


class CyberpunkGraphWidget(QOpenGLWidget):
    def __init__(self):
        super().__init__()
        self.setMinimumSize(360, 250)

    def initializeGL(self):
        native_graph.initialize()

    def resizeGL(self, width, height):
        ratio = self.devicePixelRatioF()
        native_graph.resize(int(width * ratio), int(height * ratio))

    def paintGL(self):
        native_graph.render()


class CyberpunkDashboard(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Cyberpunk Telemetry Console")
        self.resize(1320, 860)

        self.graphs = [CyberpunkGraphWidget() for _ in range(4)]
        self._last_time = perf_counter()
        self._paused = False
        self.pause_button = QPushButton("PAUSE")
        self.pause_button.setCheckable(True)
        self.pause_button.clicked.connect(self._toggle_pause)

        central = QWidget()
        root_layout = QHBoxLayout(central)
        root_layout.setContentsMargins(18, 18, 18, 18)
        root_layout.setSpacing(16)

        sidebar = self._build_sidebar()
        viewport = self._build_viewport()

        root_layout.addWidget(sidebar)
        root_layout.addWidget(viewport, 1)

        self.setCentralWidget(central)
        self._apply_style()

        self._timer = QTimer(self)
        self._timer.setInterval(16)
        self._timer.timeout.connect(self._tick)
        self._timer.start()

    def _build_sidebar(self):
        sidebar = QFrame()
        sidebar.setObjectName("Sidebar")
        sidebar.setFixedWidth(270)

        layout = QVBoxLayout(sidebar)
        layout.setContentsMargins(18, 18, 18, 18)
        layout.setSpacing(14)

        title = QLabel("NEON OPS")
        title.setObjectName("AppTitle")

        subtitle = QLabel("Live telemetry console")
        subtitle.setObjectName("MutedText")

        status_card = QFrame()
        status_card.setObjectName("Panel")
        status_layout = QVBoxLayout(status_card)
        status_layout.setContentsMargins(14, 14, 14, 14)
        status_layout.setSpacing(8)

        status_title = QLabel("STREAM STATUS")
        status_title.setObjectName("SectionTitle")
        status_layout.addWidget(status_title)
        status_layout.addWidget(QLabel("Renderer: C++ OpenGL"))
        status_layout.addWidget(QLabel("Host: PySide6"))
        status_layout.addWidget(QLabel("Viewports: 4"))
        status_layout.addWidget(QLabel("Refresh: 60 FPS"))

        controls = QFrame()
        controls.setObjectName("Panel")
        controls_layout = QVBoxLayout(controls)
        controls_layout.setContentsMargins(14, 14, 14, 14)
        controls_layout.setSpacing(10)

        controls_title = QLabel("CONTROLS")
        controls_title.setObjectName("SectionTitle")
        controls_layout.addWidget(controls_title)
        controls_layout.addWidget(self.pause_button)

        layout.addWidget(title)
        layout.addWidget(subtitle)
        layout.addSpacing(12)
        layout.addWidget(status_card)
        layout.addWidget(controls)
        layout.addStretch(1)

        footer = QLabel("native_graph.pyd")
        footer.setObjectName("MutedText")
        layout.addWidget(footer)

        return sidebar

    def _build_viewport(self):
        frame = QFrame()
        frame.setObjectName("ViewportShell")

        layout = QVBoxLayout(frame)
        layout.setContentsMargins(10, 10, 10, 10)
        layout.setSpacing(8)

        title_bar = QFrame()
        title_bar.setObjectName("ViewportTitleBar")
        title_layout = QHBoxLayout(title_bar)
        title_layout.setContentsMargins(12, 8, 12, 8)
        title_layout.setSpacing(8)

        title = QLabel("LIVE OPENGL VIEWPORTS")
        title.setObjectName("ViewportTitle")
        chip = QLabel("ACTIVE")
        chip.setObjectName("ActiveChip")

        title_layout.addWidget(title)
        title_layout.addStretch(1)
        title_layout.addWidget(chip)

        grid = QGridLayout()
        grid.setContentsMargins(0, 0, 0, 0)
        grid.setSpacing(10)

        for index, graph in enumerate(self.graphs):
            graph_frame = self._build_graph_frame(f"VIEW {index + 1:02d}", graph)
            grid.addWidget(graph_frame, index // 2, index % 2)

        layout.addWidget(title_bar)
        layout.addLayout(grid, 1)

        return frame

    def _build_graph_frame(self, label, graph):
        frame = QFrame()
        frame.setObjectName("GraphFrame")

        layout = QVBoxLayout(frame)
        layout.setContentsMargins(6, 6, 6, 6)
        layout.setSpacing(6)

        title = QLabel(label)
        title.setObjectName("MiniViewportTitle")

        layout.addWidget(title)
        layout.addWidget(graph, 1)
        return frame

    def _toggle_pause(self, checked):
        self._paused = checked
        self._last_time = perf_counter()
        self.pause_button.setText("RESUME" if checked else "PAUSE")

    def _tick(self):
        if self._paused:
            return

        now = perf_counter()
        native_graph.update(now - self._last_time)
        self._last_time = now

        for graph in self.graphs:
            graph.update()

    def _apply_style(self):
        self.setStyleSheet(
            """
            QMainWindow, QWidget {
                background: #070713;
                color: #d5fbff;
                font-family: Segoe UI;
                font-size: 14px;
            }

            #Sidebar, #ViewportShell {
                background: #0d0b20;
                border: 1px solid #13e8ff;
                border-radius: 8px;
            }

            #Panel {
                background: #11102a;
                border: 1px solid #33415f;
                border-radius: 6px;
            }

            #AppTitle {
                color: #ffe84a;
                font-size: 28px;
                font-weight: 700;
                letter-spacing: 0px;
            }

            #SectionTitle, #ViewportTitle, #MiniViewportTitle {
                color: #ff2bd6;
                font-size: 13px;
                font-weight: 700;
            }

            #MutedText {
                color: #7fa6b6;
            }

            #ViewportTitleBar {
                background: #12122d;
                border: 1px solid #253c58;
                border-radius: 6px;
            }

            #GraphFrame {
                background: #02020a;
                border: 2px solid #13e8ff;
                border-radius: 6px;
            }

            QOpenGLWidget {
                border-radius: 4px;
            }

            #ActiveChip {
                color: #061018;
                background: #21ff9b;
                border-radius: 5px;
                padding: 3px 10px;
                font-size: 12px;
                font-weight: 700;
            }

            QPushButton {
                color: #071018;
                background: #13e8ff;
                border: 0;
                border-radius: 5px;
                padding: 10px 12px;
                font-weight: 700;
            }

            QPushButton:checked {
                background: #ffe84a;
            }
            """
        )


def main():
    surface_format = QSurfaceFormat()
    surface_format.setRenderableType(QSurfaceFormat.OpenGL)
    surface_format.setProfile(QSurfaceFormat.CompatibilityProfile)
    surface_format.setVersion(2, 1)
    surface_format.setSwapInterval(1)
    QSurfaceFormat.setDefaultFormat(surface_format)

    app = QApplication(sys.argv)
    widget = CyberpunkDashboard()
    widget.show()
    return app.exec()


if __name__ == "__main__":
    raise SystemExit(main())
