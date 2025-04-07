import os
import shutil
from pathlib import Path

def clean_build():
    """清理构建产生的缓存文件和目录"""
    try:
        print("开始清理...")
        
        # 获取项目根目录
        root_dir = Path(__file__).parent
        
        # 需要清理的目录和文件类型
        dirs_to_clean = [
            "build",           # CMake构建目录
            "__pycache__",     # Python字节码缓存
            ".pytest_cache",   # PyTest缓存
        ]
        
        # 需要清理的文件扩展名
        extensions_to_clean = [
            ".pyc",           # Python编译文件
            ".pyo",           # Python优化文件
            ".pyd",           # Python动态链接库
            ".obj",           # C++对象文件
            ".ilk",           # Visual Studio增量链接文件
            ".pdb",           # 程序数据库文件
            ".exp",           # 导出文件
            # ".lib",           # 库文件
        ]
        
        # 遍历项目目录
        for path in root_dir.rglob("*"):
            # 清理目录
            if path.is_dir() and path.name in dirs_to_clean:
                try:
                    shutil.rmtree(path)
                    print(f"已删除目录: {path}")
                except Exception as e:
                    print(f"删除目录失败 {path}: {e}")
            
            # 清理文件
            if path.is_file() and path.suffix in extensions_to_clean:
                try:
                    path.unlink()
                    print(f"已删除文件: {path}")
                except Exception as e:
                    print(f"删除文件失败 {path}: {e}")
        
        print("清理完成！")
        return True
    except Exception as e:
        print(f"清理过程中出错: {e}")
        return False

if __name__ == "__main__":
    clean_build()
