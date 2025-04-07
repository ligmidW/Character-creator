import os
import psutil
import time
import subprocess
from scripts.config.settings import MAYA_BIN, MAYA_VERSION, PLUGIN_NAME
from scripts.maya.plugins import copy_plugin_to_maya

def is_maya_running():
    for proc in psutil.process_iter(['name']):
        try:
            if proc.name().lower() == 'maya.exe':
                return proc
        except (psutil.NoSuchProcess, psutil.AccessDenied):
            pass
    return None

def close_maya():
    maya_proc = is_maya_running()
    if maya_proc:
        print("检测到Maya正在运行，正在关闭...")
        try:
            maya_proc.terminate()
            maya_proc.wait(timeout=3)
            print("Maya已关闭")
            time.sleep(5)  # 等待5秒确保文件释放
            # 复制插件
            if copy_plugin_to_maya():
                print("插件复制完成")
            else:
                print("插件复制失败")
            return True
        except Exception as e:
            print(f"关闭Maya时出错: {e}")
    else:
        # 如果Maya没有运行，直接复制插件
        if copy_plugin_to_maya():
            print("插件复制完成")
        else:
            print("插件复制失败")
    return True

def launch_maya():
    if not os.path.exists(MAYA_BIN):
        print(f"错误：找不到Maya可执行文件: {MAYA_BIN}")
        return False
        
    try:
        subprocess.Popen(MAYA_BIN)
        print(f"Maya {MAYA_VERSION} 启动成功！")
        print("\n在Maya的脚本编辑器中运行以下代码：")
        print(f"""
fbx_files = [
    r"D:/work/Maya/CharacterFactory/resources/trump.fbx",
    r"D:/work/Maya/CharacterFactory/resources/bge.fbx",
    r"D:/work/Maya/CharacterFactory/resources/cooper.fbx",
    r"D:/work/Maya/CharacterFactory/resources/farrukh.fbx"
]
output_path = r"D:/work/Maya/CharacterFactory/resources/head_blend.fbx"
json_path = r"D:/work/Maya/CharacterFactory/resources/skin_weights.json"

if __name__ == "__main__":
    characterfactoryfbxhandle(fbx_files, output_path, json_path)

""")
        return True
    except Exception as e:
        print(f"启动Maya失败: {e}")
        return False
