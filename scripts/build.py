import subprocess
from scripts.config.settings import PROJECT_ROOT, BUILD_DIR, MAYA_VERSION, MAYA_PATH, CMAKE_PATH

def build_cpp_plugin():
    print("开始构建C++插件...")
    BUILD_DIR.mkdir(parents=True, exist_ok=True)
    
    # 清理CMake缓存
    cache_file = BUILD_DIR / "CMakeCache.txt"
    if cache_file.exists():
        print("清理CMake缓存...")
        cache_file.unlink()
    
    # CMake配置
    config_cmd = [CMAKE_PATH, "-S", str(PROJECT_ROOT / "api"), "-B", str(BUILD_DIR),
                 f"-DMAYA_VERSION={MAYA_VERSION}", f"-DCMAKE_PREFIX_PATH={MAYA_PATH}"]
    
    # CMake构建
    build_cmd = [CMAKE_PATH, "--build", str(BUILD_DIR), "--config", "Release"]
    
    try:
        # 配置项目
        process = subprocess.run(config_cmd, capture_output=True, text=True, encoding='utf-8', errors='ignore')
        if process.returncode != 0:
            print(f"CMake配置失败:\n{process.stderr}")
            print(f"CMake输出:\n{process.stdout}")  
            return False
            
        # 构建项目
        process = subprocess.run(build_cmd, capture_output=True, text=True, encoding='utf-8', errors='ignore')
        if process.returncode != 0:
            print(f"CMake构建失败:\n{process.stderr}")
            print(f"CMake输出:\n{process.stdout}")  
            return False
        
        print("C++插件构建成功！")
        return True
    except Exception as e:
        print(f"构建失败: {e}")
        return False
