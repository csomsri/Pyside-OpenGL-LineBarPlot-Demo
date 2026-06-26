import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parent


def run(command):
    print(" ".join(command))
    subprocess.check_call(command, cwd=ROOT)


def main():
    run([
        "cmake",
        "-S",
        str(ROOT),
        "-B",
        str(ROOT / "build"),
        f"-DPython_EXECUTABLE={sys.executable}",
    ])
    run(["cmake", "--build", str(ROOT / "build"), "--config", "Release"])


if __name__ == "__main__":
    main()
