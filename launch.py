import maya.cmds as cmds
import maya.OpenMaya as om


def ensure_plugin_loaded(plugin_name="CharacterFactory.mll"):
    try:
        if not cmds.pluginInfo(plugin_name, query=True, loaded=True):
            cmds.loadPlugin(plugin_name)
        return cmds.pluginInfo(plugin_name, query=True, loaded=True)
    except Exception as e:
        om.MGlobal.displayError(f"Error loading plugin {plugin_name}: {str(e)}")
        return False


def characterfactoryfbxhandle(fbx_files, output_path, json_path=None):
    if not ensure_plugin_loaded():
        om.MGlobal.displayError("Required plugin CharacterFactory.mll is not loaded")
        return None
        
    try:
        fbx_files_str = ";".join(fbx_files)
        return cmds.characterfactoryfbxhandle(fbx_files_str, output_path, json_path)
    except Exception as e:
        om.MGlobal.displayError(f"Error executing characterfactoryfbxhandle command: {str(e)}")
        return None


fbx_files = [
    r"D:/work/Maya/CharacterFactory/resources/trump.fbx",
    r"D:/work/Maya/CharacterFactory/resources/bigear.fbx",
    r"D:/work/Maya/CharacterFactory/resources/cooper.fbx",
    r"D:/work/Maya/CharacterFactory/resources/farrukh.fbx"
]
output_path = r"D:/work/Maya/CharacterFactory/resources/head_blend.fbx"
json_path = r"D:/work/Maya/CharacterFactory/resources/skin_weights.json"

if __name__ == "__main__":
    characterfactoryfbxhandle(fbx_files, output_path, json_path)
