import os
from pathlib import Path

MAYA_VERSION = "2022"
MAYA_PATH = r"C:\Program Files\Autodesk\Maya2022"
MAYA_BIN = os.path.join(MAYA_PATH, "bin", "maya.exe")

CMAKE_PATH = r"C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"

PROJECT_ROOT = Path(__file__).parent.parent.parent
BUILD_DIR = PROJECT_ROOT / "api" / "build"
PLUGIN_NAME = "CharacterFactory.mll"

MAYA_PLUGIN_DIRS = [
    Path(os.path.expanduser(f"~/Documents/maya/{MAYA_VERSION}/plug-ins")),
    PROJECT_ROOT / "plug-ins",
]
