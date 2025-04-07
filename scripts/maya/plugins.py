import shutil
from scripts.config.settings import *
import time
import os

def wait_and_copy(src, dst, retries=3, delay=5):
    """
    等待并尝试复制文件，如果失败则重试
    """
    for attempt in range(retries):
        try:
            if os.path.exists(dst):
                os.remove(dst)
            shutil.copy2(src, dst)
            return True
        except Exception as e:
            if attempt < retries - 1:
                print(f"复制失败，等待 {delay} 秒后重试: {e}")
                time.sleep(delay)
            else:
                print(f"复制失败，已达到最大重试次数: {e}")
                raise

def copy_plugin_to_maya():
    """
    复制插件到Maya插件目录
    """
    plugin_path = BUILD_DIR / "Release" / PLUGIN_NAME
    
    if not plugin_path.exists():
        print(f"错误：找不到插件文件: {plugin_path}")
        return False
        
    # 复制到第一个可用的插件目录
    for plugin_dir in MAYA_PLUGIN_DIRS:
        try:
            # 创建目录（包括父目录）
            plugin_dir.mkdir(parents=True, exist_ok=True)
            dest_path = plugin_dir / PLUGIN_NAME
            
            print(f"尝试复制插件到: {dest_path}")
            wait_and_copy(plugin_path, dest_path)
            print(f"插件已复制到: {dest_path}")
            
            # 复制依赖的DLL文件
            fbx_dll_src = PROJECT_ROOT / "third_party" / "FbxSdk" / "2020.3.7" / "lib" / "x64" / "release" / "libfbxsdk.dll"
            if fbx_dll_src.exists():
                dll_dest = plugin_dir / "libfbxsdk.dll"
                print(f"尝试复制FBX SDK DLL到: {dll_dest}")
                wait_and_copy(fbx_dll_src, dll_dest)
                print(f"FBX SDK DLL已复制到: {plugin_dir}")
            
            return True
        except Exception as e:
            print(f"无法复制到 {plugin_dir}: {e}")
            continue
            
    print("警告：插件未能复制到任何Maya插件目录")
    return False
