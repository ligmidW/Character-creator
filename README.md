# CharacteCreator Maya Plugin

"""持续更新中"""
ssh -v midlig@192.168.31.58 "python D:\\work\\Maya\\CharacterFactory\\setup.py"

[![Maya 2022](https://img.shields.io/badge/Maya-2022-blue.svg)](https://www.autodesk.com/products/maya/overview)
[![FBX SDK 2020.3.7](https://img.shields.io/badge/FBX%20SDK-2020.3.7-orange.svg)](https://www.autodesk.com/developer-network/platform-technologies/fbx-sdk-2020-3)
[![Python 3.12](https://img.shields.io/badge/Python-3.12-green.svg)](https://www.python.org/)
[![Visual Studio 2022](https://img.shields.io/badge/Visual%20Studio-2022-purple.svg)](https://visualstudio.microsoft.com/)

Character-creator 是一个 Maya 插件，用于处理角色相关的操作，主要功能包括：

- cmake一键构建maya插件
- 处理多个 FBX 文件并提取顶点数据,创建 BlendShape 节点
- 计算不同 FBX 文件之间的网格差异
- 读取权重 JSON 文件进行调整

## 项目结构

```
CharacterFactory/
├── api/                    # C++插件源代码
│   ├── src/               # 源文件
│   ├── include/           # 头文件
│   └── build/             # 构建目录
├── scripts/               # Python脚本
│   ├── build.py          # 构建相关
│   ├── maya/             # Maya相关
│   │   ├── process.py    # Maya进程管理
│   │   └── plugins.py    # 插件管理
│   └── config/           # 配置相关
│       └── settings.py   # 配置文件
├── third_party/          # 第三方库
│   └── FbxSdk/          # Autodesk FBX SDK
├── plug-ins/             # Maya插件目录
└── setup.py             # 主安装脚本
```

## 环境要求

### 软件要求
- **Maya 2022**：主要运行环境，需安装在默认路径或在配置中指定
- **Python 3.12**：用于运行脚本和构建过程
- **Visual Studio 2022**：用于编译C++插件（需包含C++桌面开发工作负载）
- **CMake 3.28.1+**：用于构建系统（Visual Studio 2022自带）
- **Autodesk FBX SDK 2020.3.7**：用于FBX文件处理

## 依赖项

### Python 依赖
```
psutil==7.0.0  # 用于Maya进程管理
nlohmann-json==3.11.2  # 用于JSON处理
```

### C++ 依赖
- **Maya API**：用于与Maya交互
- **FBX SDK**：用于FBX文件处理
- **nlohmann/json**：用于C++中的JSON处理（已包含在项目中）

## 安装步骤

### 自动安装
1. 确保已安装所有环境要求中列出的软件
2. 克隆或下载此仓库到本地
3. 安装Python依赖：
   ```bash
   pip install -r requirements.txt
   ```
4. 运行安装脚本：
   ```bash
   py setup.py
   ```

   此脚本会自动执行以下操作：
   - 构建C++插件
   - 关闭当前运行的Maya（如果有）
   - 将插件复制到Maya插件目录
   - 启动Maya

### 手动安装
1. 使用CMake构建C++插件：
   ```bash
   cd api
   mkdir build
   cd build
   cmake ..
   cmake --build . --config Release
   ```
2. 将生成的 `CharacterFactory.mll` 复制到Maya插件目录：
   - 默认路径：`C:\Program Files\Autodesk\Maya2022\bin\plug-ins`
   - 或用户路径：`%USERPROFILE%\Documents\maya\2022\plug-ins`
3. 启动Maya并加载插件

## 使用方法

### 在Maya中直接使用

在Maya的脚本编辑器中执行以下Python代码：

```python
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
```

