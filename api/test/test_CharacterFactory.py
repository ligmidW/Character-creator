import maya.cmds as cmds
import unittest

class TestCharacterFactory(unittest.TestCase):

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
