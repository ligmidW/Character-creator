from scripts.maya.process import launch_maya, close_maya
from scripts.build import build_cpp_plugin

if __name__ == "__main__":
    if build_cpp_plugin():
        if not close_maya():
            print("无法关闭正在运行的Maya，请手动关闭后重试")
        else:
            launch_maya()