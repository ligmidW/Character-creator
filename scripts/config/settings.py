import os
from pathlib import Path

MAYA_VERSION = "2022"
MAYA_PATH = r"C:\Program Files\Autodesk\Maya2022"
MAYA_BIN = os.path.join(MAYA_PATH, "bin", "maya.exe")

CMAKE_PATH = r"C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"

PROJECT_ROOT = Path(__file__).parent.parent.parent
BUILD_DIR = PROJECT_ROOT / "api" / "build"
PLUGIN_NAME = "CharacterFactory.mll"

# Maya user directories
MAYA_USER = "midlig"
MAYA_APP_DIR = r"C:\Users\midlig\Documents\maya"
MAYA_SCRIPT_PATH = os.path.join(MAYA_APP_DIR, "scripts")
MAYA_PLUGIN_PATH = os.path.join(MAYA_APP_DIR, MAYA_VERSION, "plug-ins")

# Desktop path for shortcuts
DESKTOP_PATH = r"C:\Users\midlig\Desktop"

# Task scheduler settings
MAYA_TASK_NAME = "StartMayaTask"

MAYA_PLUGIN_DIRS = [
    Path(os.path.expanduser(f"~/Documents/maya/{MAYA_VERSION}/plug-ins")),
    PROJECT_ROOT / "plug-ins",
]
