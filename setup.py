from scripts.maya.process import launch_maya, close_maya
from scripts.build import build_cpp_plugin
import sys
from pathlib import Path

# 保证每次 setup 前都清理一次
sys.path.insert(0, str(Path(__file__).parent))
try:
    from clean import clean_build
    clean_build()
except Exception as e:
    print(f"[Warning] Failed to clean before setup: {e}")

if __name__ == "__main__":
    if build_cpp_plugin():
        if not close_maya():
            print("无法关闭正在运行的Maya，请手动关闭后重试")
        else:
            launch_maya()