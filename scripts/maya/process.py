import os
import psutil
import time
import subprocess
from scripts.config.settings import (
    MAYA_BIN, MAYA_VERSION, PLUGIN_NAME, 
    MAYA_APP_DIR, MAYA_SCRIPT_PATH, MAYA_PLUGIN_PATH,
    DESKTOP_PATH, MAYA_USER, MAYA_TASK_NAME
)
from scripts.maya.plugins import copy_plugin_to_maya

def remote_run_bat(batch_content, batch_name="remote_command.bat"):
    """Helper function to create and execute a batch file on Windows from remote SSH"""
    batch_path = os.path.join(os.environ.get('TEMP', r'C:\Windows\Temp'), batch_name)
    try:
        with open(batch_path, 'w', encoding='gbk') as f:
            f.write(batch_content)
        
        # Execute the batch file using cmd /c to ensure proper Windows environment
        result = subprocess.run(f'cmd.exe /c "{batch_path}"', 
                               shell=True, 
                               capture_output=True, 
                               text=True, 
                               encoding='gbk')
        
        if result.returncode != 0:
            print(f"execute batch file failed: {result.stderr}")
            return False, result.stdout, result.stderr
        
        return True, result.stdout, result.stderr
    except Exception as e:
        print(f"execute batch file exception: {e}")
        return False, "", str(e)

def is_maya_running():
    for proc in psutil.process_iter(['name']):
        try:
            if proc.name().lower() == 'maya.exe':
                return proc
        except (psutil.NoSuchProcess, psutil.AccessDenied):
            pass
    return None

def close_maya():
    # Use simple batch file to close Maya
    batch_content = '@echo off\n'
    batch_content += 'echo Checking Maya process...\n'
    batch_content += 'taskkill /F /IM maya.exe 2>nul\n'
    batch_content += 'if %ERRORLEVEL% EQU 0 (\n'
    batch_content += '    echo Maya closed\n'
    batch_content += '    ping -n 6 127.0.0.1 > nul\n'
    batch_content += ') else (\n'
    batch_content += '    echo Maya not running\n'
    batch_content += ')\n'
    
    success, stdout, stderr = remote_run_bat(batch_content, "close_maya.bat")
    
    if not success:
        print(f"close maya failed: {stderr}")
        return False
    
    print(stdout)
    
    # Copy plugin
    if copy_plugin_to_maya():
        print("plugin copy success")
    else:
        print("plugin copy failed")
    
    return True

def launch_maya():
    if not os.path.exists(MAYA_BIN):
        print(f"error: maya bin not found: {MAYA_BIN}")
        return False
        
    try:
        # Create a batch file to launch Maya with proper environment
        batch_file_path = os.path.join(os.environ.get('TEMP', r'C:\Windows\Temp'), "start_maya.bat")
        with open(batch_file_path, 'w', encoding='gbk') as f:
            f.write('@echo off\n')
            f.write('echo Starting Maya...\n')
            f.write(f'set MAYA_APP_DIR={MAYA_APP_DIR}\n')
            f.write(f'set MAYA_SCRIPT_PATH={MAYA_SCRIPT_PATH}\n')
            f.write(f'set MAYA_PLUG_IN_PATH={MAYA_PLUGIN_PATH}\n')
            f.write(f'start "" "{MAYA_BIN}"\n')
        
        # Make sure the batch file is executable
        os.chmod(batch_file_path, 0o755)
        
        # Create a scheduled task to run the batch file immediately
        print(f"Creating scheduled task to launch Maya: {MAYA_TASK_NAME}")
        
        # Delete any existing task with the same name
        subprocess.run(f'schtasks /delete /tn "{MAYA_TASK_NAME}" /f', 
                      shell=True, 
                      stdout=subprocess.DEVNULL, 
                      stderr=subprocess.DEVNULL)
        
        # Create and run the task immediately
        create_task_cmd = f'schtasks /create /tn "{MAYA_TASK_NAME}" /tr "{batch_file_path}" /sc once /st 00:00 /ru "{MAYA_USER}" /f'
        run_task_cmd = f'schtasks /run /tn "{MAYA_TASK_NAME}"'
        
        print("Creating scheduled task...")
        create_result = subprocess.run(create_task_cmd, 
                                     shell=True, 
                                     capture_output=True, 
                                     text=True, 
                                     encoding='gbk')
        
        if create_result.returncode != 0:
            print(f"Failed to create scheduled task: {create_result.stderr}")
            
            # Alternative method: try creating a desktop shortcut
            print("Trying alternative method: creating desktop shortcut...")
            shortcut_path = os.path.join(DESKTOP_PATH, "StartMaya.bat")
            with open(shortcut_path, 'w', encoding='gbk') as f:
                f.write('@echo off\n')
                f.write(f'set MAYA_APP_DIR={MAYA_APP_DIR}\n')
                f.write(f'set MAYA_SCRIPT_PATH={MAYA_SCRIPT_PATH}\n')
                f.write(f'set MAYA_PLUG_IN_PATH={MAYA_PLUGIN_PATH}\n')
                f.write(f'start "" "{MAYA_BIN}"\n')
            
            print(f"Created desktop shortcut: {shortcut_path}")
            print("Please run this shortcut manually on the Windows desktop")
            return False
        
        print("Running scheduled task...")
        run_result = subprocess.run(run_task_cmd, 
                                  shell=True, 
                                  capture_output=True, 
                                  text=True, 
                                  encoding='gbk')
        
        if run_result.returncode != 0:
            print(f"Failed to run scheduled task: {run_result.stderr}")
            print("Please check Windows Task Scheduler or run the batch file manually")
            return False
        
        print("Maya launch task scheduled successfully")
        print("Maya should start shortly on the Windows system")
        print("If Maya does not start, please find the batch file on Windows desktop or check Task Scheduler")
        
        print(f"""
Example code:
fbx_files = [
    r"D:/work/Maya/CharacterFactory/resources/trump.fbx",
    r"D:/work/Maya/CharacterFactory/resources/bge.fbx",
    r"D:/work/Maya/CharacterFactory/resources/cooper.fbx",
    r"D:/work/Maya/CharacterFactory/resources/farrukh.fbx"
]
output_path = r"D:/work/Maya/CharacterFactory/resources/head_blend.fbx"
json_path = r"D:/work/Maya/CharacterFactory/resources/skin_weights.json"

if __name__ == "__main__":
    import maya.cmds as cmds
    cmds.characterfactoryfbxhandle(fbx_files, output_path, json_path)
""")
        return True
    except Exception as e:
        print(f"launch maya failed: {e}")
        return False
