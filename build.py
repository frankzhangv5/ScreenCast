#!/usr/bin/env python3
"""
ScreenMirror项目部署脚本 - Python版本
support Windows、Linux和macOS平台
"""

import os
import sys
import subprocess
import argparse
import shutil
import re
from pathlib import Path
import platform

# ANSI 转义码定义
RESET = '\033[0m'
GREEN = '\033[92m'
RED = '\033[91m'

# 获取当前平台
CURRENT_PLATFORM = platform.system().lower()  # 'windows', 'linux', 'darwin' (macOS)
IS_WINDOWS = CURRENT_PLATFORM == 'windows'
IS_LINUX = CURRENT_PLATFORM == 'linux'
IS_MACOS = CURRENT_PLATFORM == 'darwin'

def logd(message):
    """打印默认颜色日志"""
    print(f"{RESET}{message}")

def logi(message):
    """打印绿色信息日志"""
    print(f"{GREEN}{message}{RESET}")

def loge(message):
    """打印红色错误日志"""
    print(f"{RED}{message}{RESET}")



def find_qt_dir():
    """查找Qt安装目录"""
    # 先使用shutil.which在系统PATH中查找qmake
    qmake_candidates = ['qmake']
    if IS_WINDOWS:
        qmake_candidates.append('qmake.exe')
    
    for qmake_name in qmake_candidates:
        qmake_path = shutil.which(qmake_name)
        if qmake_path:
            return str(Path(qmake_path).parent.parent)
    
    # 根据不同操作系统设置可能的Qt安装路径
    possible_paths = []
    qmake_name = 'qmake.exe' if IS_WINDOWS else 'qmake'
    
    if IS_WINDOWS:
        possible_paths = [
            "E:/Qt/6.9.1/mingw_64",
            "C:/Qt/6.8.3/mingw_64",
        ]
    elif IS_MACOS:
        possible_paths = [
            "/Users/*/Qt/6.9.1/clang_64",
            "/Applications/Qt/6.9.1/clang_64",
        ]
    else:  # Linux 及其他 Unix 系统
        possible_paths = [
            "/opt/Qt/6.9.1/gcc_64",
            "/usr/local/Qt/6.9.1/gcc_64",
            "/usr/lib/qt6",
            "/usr/lib/qt",
        ]

    # 处理路径中的通配符
    expanded_paths = []
    for path in possible_paths:
        if '*' in path:
            for p in Path().glob(path):
                expanded_paths.append(str(p))
        else:
            expanded_paths.append(path)

    for path in expanded_paths:
        qmake_path = Path(path) / "bin" / qmake_name
        if qmake_path.exists():
            return str(path)

    # 检查环境变量
    qt_dir = os.environ.get('Qt6_DIR')
    if qt_dir:
        qmake_path = Path(qt_dir) / "bin" / qmake_name
        if qmake_path.exists():
            return str(qt_dir)

    return None

def find_compiler_path(qt_dir):
    """查找编译器路径"""
    if IS_WINDOWS:
        return find_mingw_path(qt_dir)
    elif IS_LINUX:
        # Linux通常使用系统自带的gcc/g++，检查PATH
        return None  # Linux不需要特别指定编译器路径
    elif IS_MACOS:
        # macOS通常使用Xcode的clang，检查PATH
        return None  # macOS不需要特别指定编译器路径
    return None

def find_mingw_path(qt_dir):
    """查找MinGW工具链路径 (Windows专用)"""
    mingw_make_path = shutil.which('mingw32-make.exe')
    if mingw_make_path:
        return str(Path(mingw_make_path).parent.parent)

    if not qt_dir or not IS_WINDOWS:
        return None
    
    mingw_pattern = Path(qt_dir) / "../../Tools/mingw*"
    mingw_dirs = list(mingw_pattern.parent.glob("mingw*"))
    
    if mingw_dirs:
        return str(mingw_dirs[0])
    
    return None

def build_qmake_project(target_name, project_file, build_dir, dist_dir, build_type):
    """构建Qt项目"""
    logi(f"Building Project for {target_name} with {build_type} configuration...")
    logd(f"ProjectFile: {project_file}")
    logd(f"BuildDir: {build_dir}")
    logd(f"DistDir: {dist_dir}")
    logd(f"BuildType: {build_type}")
    
    # 创建构建目录
    build_path = Path(build_dir)
    build_path.mkdir(parents=True, exist_ok=True)
    
    old_cwd = os.getcwd()
    os.chdir(build_dir)
    
    try:
        # 配置项目
        logi(f"Configuring project {project_file} for {target_name} with {build_type} configuration...")
        
        # 根据平台设置不同的spec
        config_cmd = ['qmake', project_file]
        if IS_WINDOWS:
            config_cmd.extend(['-spec', 'win32-g++'])
        elif IS_LINUX:
            config_cmd.extend(['-spec', 'linux-g++'])
        elif IS_MACOS:
            config_cmd.extend(['-spec', 'macx-clang'])
        
        config_cmd.append(f'CONFIG+={build_type}')
        
        result = subprocess.run(config_cmd, capture_output=True, text=True, encoding='utf-8', errors='ignore')
        if result.returncode != 0:
            loge(f"Failed to configure {target_name} project")
            logd(f"Error: {result.stderr}")
            return False
        
        # 构建项目
        logi(f"Compiling project {project_file} for {target_name} with {build_type} configuration...")
        
        # 根据平台选择构建工具
        if IS_WINDOWS:
            build_cmd = ['mingw32-make', '-j6']
        else:  # Linux/macOS
            build_cmd = ['make', '-j6']
        
        result = subprocess.run(build_cmd, capture_output=True, text=True, encoding='utf-8', errors='ignore')
        if result.returncode != 0:
            loge(f"Failed to build {target_name} project")
            logd(f"Error: {result.stderr}")
            return False
        
        logi(f"{target_name} project built successfully!")
        
        # 复制到分发目录
        if dist_dir:
            dist_path = Path(dist_dir)
            dist_path.mkdir(parents=True, exist_ok=True)
            
            # 根据平台确定输出目录
            if IS_WINDOWS:
                source_path = Path(build_dir) / f"{build_type}/{target_name}"
            else:  # Linux/macOS
                source_path = Path(build_dir) / target_name
                # macOS应用程序可能是.app包
                if IS_MACOS and target_name.endswith('.app'):
                    source_path = Path(build_dir) / f"{target_name}"
            
            destination_path = Path(dist_dir) / target_name
            
            if source_path.exists():
                logi(f"Copying {target_name} project to distribution directory...")
                if source_path.is_dir():
                    if destination_path.exists():
                        shutil.rmtree(destination_path)
                    shutil.copytree(source_path, destination_path)
                else:
                    shutil.copy2(source_path, destination_path)
        
        return True
        
    except Exception as e:
        loge(f"Error: {e}")
        return False
    finally:
        os.chdir(old_cwd)

def build_mirror_server(script_dir):
    """构建mirror_server"""
    # 根据平台选择python命令
    python_cmd = "python" if IS_WINDOWS else "python3"

    # android
    if os.environ.get('ANDROID_NDK_HOME'):
        server_home = script_dir / "server" / "android"
        relese_dir = script_dir / "plugins" / "android" / "res" / "server"
        build_script = server_home / "build.py"
        
        result = subprocess.run(
            [python_cmd, str(build_script), '--ndk-path', os.environ.get('ANDROID_NDK_HOME'), "--release-dir", str(relese_dir), "--arch", "arm64-v8a"],
            capture_output=True,
            text=True,
            encoding='utf-8',
            errors='ignore'
        )
        if result.stdout:
            logi(f"Android Build: {result.stdout}")
        if result.returncode != 0:
            loge(f"Failed to run {build_script}")
            logd(f"Error: {result.stderr}")
            sys.exit(1)

    # ohos
    if os.environ.get('OHOS_SDK_HOME'):
        server_home = script_dir / "server" / "ohos"
        relese_dir = script_dir / "plugins" / "ohos" / "res" / "server"
        build_script = server_home / "build.py"
        
        result = subprocess.run(
            [python_cmd, str(build_script), '--ohos-sdk-path', os.environ.get('OHOS_SDK_HOME'), "--release-dir", str(relese_dir), "--arch", "arm"],
            capture_output=True,
            text=True,
            encoding='utf-8',
            errors='ignore'
        )
        if result.stdout:
            logi(f"OHOS Build: {result.stdout}")
        if result.returncode != 0:
            loge(f"Failed to run {build_script}")
            logd(f"Error: {result.stderr}")
            sys.exit(1)

def build_plugins(script_dir, build_dir, dist_dir, build_type):
    """构建插件"""
    release_dir = dist_dir / "plugins" / build_type
    
    # 构建Android插件
    plugin_dir = build_dir / "android"
    project_file = script_dir / "plugins/android/android.pro"
    success = build_qmake_project(
        "android_device_plugin.dll", 
        str(project_file), 
        str(plugin_dir), 
        str(release_dir), 
        build_type
    )
    
    if not success:
        loge("Failed to build android plugin")
        sys.exit(1)
    
    # 构建OHOS插件
    plugin_dir = build_dir / "ohos"
    project_file = script_dir / "plugins/ohos/ohos.pro"
    success = build_qmake_project(
        "ohos_device_plugin.dll", 
        str(project_file), 
        str(plugin_dir), 
        str(release_dir), 
        build_type
    )
    
    if not success:
        loge("Failed to build ohos plugin")
        sys.exit(1)
    
    logi("All plugins built successfully!")
    logi(f"Plugin distribution directory: {release_dir}")
    return release_dir

def build_application(script_dir, build_dir, dist_dir, build_type):
    """构建主应用程序"""
    app_name = "ScreenCast"
    app_build_dir = build_dir / app_name
    app_project = script_dir / "app/app.pro"
    app_dist_dir = dist_dir / app_name / build_type
    
    # 根据平台确定目标文件名
    if IS_WINDOWS:
        app_target_name = f"{app_name}.exe"
    elif IS_MACOS:
        app_target_name = f"{app_name}.app"
    else:  # Linux
        app_target_name = app_name
    
    success = build_qmake_project(
        app_target_name, 
        str(app_project), 
        str(app_build_dir), 
        str(app_dist_dir), 
        build_type
    )
    
    if not success:
        loge("Failed to build application")
        sys.exit(1)
    
    logi("Application built successfully!")
    logi(f"Application distribution directory: {app_dist_dir}")
    return app_dist_dir, app_target_name

def deploy_application(app_dist_dir, app_target_name, qt_dir, plugin_dist_dir, script_dir):
    """部署应用程序"""
    exe_path = app_dist_dir / app_target_name
    
    # 根据平台执行不同的Qt部署
    if IS_WINDOWS:
        deploy_qt_windows(exe_path, qt_dir)
    elif IS_LINUX:
        deploy_qt_linux(exe_path, qt_dir)
    elif IS_MACOS:
        deploy_qt_macos(exe_path, qt_dir)
    
    # 部署插件
    logi("Deploying plugins...")
    plugins_dst = app_dist_dir / "plugins"
    
    # 根据平台选择插件文件扩展名
    plugin_extensions = [".dll"] if IS_WINDOWS else [".so"] if IS_LINUX else [".dylib", ".so"]
    
    if plugin_dist_dir.exists():
        for ext in plugin_extensions:
            for plugin_file in plugin_dist_dir.rglob(f"*{ext}"):
                relative_path = plugin_file.relative_to(plugin_dist_dir)
                destination = plugins_dst / relative_path
                destination.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(plugin_file, destination)
                print(f"  Copied: {relative_path}")
    
    logi("Plugins copied to build directory")
    
    # 部署依赖库
    deploy_dependency_libraries(app_dist_dir, script_dir)

def deploy_qt_windows(exe_path, qt_dir):
    """Windows平台部署Qt"""
    windeployqt = Path(qt_dir) / "bin/windeployqt6.exe"
    
    if not windeployqt.exists():
        loge("Error: windeployqt6.exe not found")
        sys.exit(1)
    
    logi("Deploying Qt runtime libraries (Windows)...")
    deploy_cmd = [
        str(windeployqt), str(exe_path), 
        '--no-translations', '--no-system-d3d-compiler', '--no-opengl-sw', '--release'
    ]
    
    result = subprocess.run(deploy_cmd, capture_output=True, text=True, encoding='utf-8', errors='ignore')
    if result.returncode != 0:
        loge("Warning: windeployqt completed with warnings")
        logd(f"Error: {result.stderr}")
    else:
        logi("Qt dependencies deployed successfully on Windows")

def deploy_qt_linux(exe_path, qt_dir):
    """Linux平台部署Qt"""
    logi("Deploying Qt runtime libraries (Linux)...")
    # Linux通常使用linuxdeployqt或手动复制库
    # 这里提供一个基本实现，可能需要根据具体情况调整
    qt_lib_dir = Path(qt_dir) / "lib"
    if qt_lib_dir.exists():
        app_lib_dir = exe_path.parent / "lib"
        app_lib_dir.mkdir(parents=True, exist_ok=True)
        
        # 复制基本Qt库
        for lib_file in qt_lib_dir.glob("libQt6*.so*"):
            try:
                shutil.copy2(lib_file, app_lib_dir)
                logi(f"  Copied: {lib_file.name}")
            except Exception as e:
                loge(f"  Failed to copy {lib_file.name}: {e}")
    else:
        loge("Qt lib directory not found, skipping Qt deployment on Linux")

def deploy_qt_macos(exe_path, qt_dir):
    """macOS平台部署Qt"""
    logi("Deploying Qt runtime libraries (macOS)...")
    # macOS通常使用macdeployqt
    macdeployqt = Path(qt_dir) / "bin/macdeployqt"
    
    if macdeployqt.exists():
        result = subprocess.run(
            [str(macdeployqt), str(exe_path)],
            capture_output=True,
            text=True,
            encoding='utf-8',
            errors='ignore'
        )
        if result.returncode != 0:
            loge("Warning: macdeployqt completed with warnings")
            logd(f"Error: {result.stderr}")
        else:
            logi("Qt dependencies deployed successfully on macOS")
    else:
        loge("macdeployqt not found, skipping Qt deployment on macOS")

def deploy_dependency_libraries(app_dist_dir, script_dir):
    """部署依赖库"""
    # 根据平台确定库目录和扩展名
    libs = ["middleware/lib/ffmpeg/bin"]
    if IS_WINDOWS:
        lib_extensions = [".dll"]
    elif IS_LINUX:
        lib_extensions = [".so"]
    elif IS_MACOS:
        lib_extensions = [".dylib", ".so"]
    
    for lib in libs:
        lib_dir = script_dir / lib
        if lib_dir.exists():
            logi(f"Deploying {lib} libraries...")
            for file in lib_dir.iterdir():
                if file.is_file() and any(file.name.endswith(ext) for ext in lib_extensions):
                    try:
                        shutil.copy2(file, app_dist_dir)
                        logi(f"  Copied: {file.name}")
                    except Exception as e:
                        loge(f"  Failed to copy {file.name}: {e}")

def package_application(app_dist_dir, app_target_name, script_dir, build_type):
    """打包应用程序生成最终安装包"""
    logi(f"Packaging application for {CURRENT_PLATFORM}...")
    
    # 确保打包目录存在
    pack_dir = script_dir / "pack"
    if not pack_dir.exists():
        loge(f"Pack directory not found: {pack_dir}")
        return False
    
    # 根据平台执行不同的打包操作
    try:
        if IS_WINDOWS:
            return package_windows(app_dist_dir, app_target_name, script_dir, build_type)
        elif IS_LINUX:
            return package_linux(app_dist_dir, app_target_name, script_dir, build_type)
        elif IS_MACOS:
            return package_macos(app_dist_dir, app_target_name, script_dir, build_type)
        else:
            loge(f"Unsupported platform for packaging: {CURRENT_PLATFORM}")
            return False
    except Exception as e:
        loge(f"Error during packaging: {e}")
        return False

def get_version_from_file(script_dir):
    """从version.pri文件中提取版本信息
    
    Args:
        script_dir: 脚本所在目录
        
    Returns:
        str: 版本号，格式为x.y.z
    """
    version = "1.0.0"  # 默认版本
    version_file = script_dir / "app" / "version.pri"
    if version_file.exists():
        try:
            with open(version_file, 'r', encoding='utf-8') as f:
                content = f.read()
                major = re.search(r'VER_MAJOR\s*=\s*(\d+)', content)
                minor = re.search(r'VER_MINOR\s*=\s*(\d+)', content)
                patch = re.search(r'VER_PATCH\s*=\s*(\d+)', content)
                if major and minor and patch:
                    version = f"{major.group(1)}.{minor.group(1)}.{patch.group(1)}"
        except Exception as e:
            loge(f"Failed to read version file: {e}")
    else:
        logi(f"Version file not found: {version_file}, using default version {version}")
    return version

def package_windows(app_dist_dir, app_target_name, script_dir, build_type):
    """Windows平台打包"""
    logi("Creating Windows installer package...")
    
    # 读取版本信息
    version = get_version_from_file(script_dir)
    
    # 获取系统架构
    arch = "x86_64"  # 默认架构
    if platform.machine().lower() == "arm64":
        arch = "arm64"
    
    # 检查Inno Setup是否安装
    iscc_paths = [
        os.path.join(os.environ.get('ProgramFiles(x86)', 'C:\Program Files (x86)'), 'Inno Setup 6\ISCC.exe'),
        os.path.join(os.environ.get('ProgramFiles', 'C:\Program Files'), 'Inno Setup 6\ISCC.exe')
    ]
    iscc_path = None
    for path in iscc_paths:
        if os.path.exists(path):
            iscc_path = path
            break
    
    if not iscc_path:
        loge("Inno Setup 6 not found. Please install Inno Setup to create Windows installer.")
        logi("Skipping Windows installer creation.")
        return False
    
    # 创建安装程序目录
    installer_dir = script_dir / "pack" / "installer"
    installer_dir.mkdir(parents=True, exist_ok=True)
    
    # 创建Inno Setup脚本
    app_name = "ScreenCast"
    app_icon_path = script_dir / "app" / "res" / "app_icons" / "windows_icon.ico"
    languages_dir = script_dir / "pack" / "Languages"

    # 打包绿色版本（zip格式）
    logi("Creating portable version (ZIP package)...")
    import zipfile

    # 定义要打包的源目录和输出zip文件路径
    portable_source_dir = app_dist_dir  # 使用函数参数中的应用分发目录
    portable_output_dir = script_dir / "pack" / "installer"
    portable_output_dir.mkdir(parents=True, exist_ok=True)
    portable_zip_name = f"ScreenCast-v{version}-windows-{arch}.zip"  # 使用动态版本号和架构
    portable_zip_path = portable_output_dir / portable_zip_name
    
    if portable_source_dir.exists():
        try:
            # 创建zip文件
            with zipfile.ZipFile(portable_zip_path, 'w', zipfile.ZIP_DEFLATED) as zipf:
                # 遍历源目录中的所有文件和子目录
                for root, dirs, files in os.walk(portable_source_dir):
                    for file in files:
                        # 获取文件的完整路径
                        file_path = Path(root) / file
                        # 计算文件在zip中的相对路径
                        arcname = file_path.relative_to(portable_source_dir.parent)
                        # 将文件添加到zip中
                        zipf.write(file_path, arcname)
                        logd(f"  Added to zip: {arcname}")
            
            if portable_zip_path.exists():
                logi(f"Portable ZIP package created successfully: {portable_zip_path}")
                logi(f"File size: {os.path.getsize(portable_zip_path) / 1024 / 1024:.2f} MB")
            else:
                loge("Portable ZIP package file not found after creation")
        except Exception as e:
            loge(f"Error creating portable ZIP package: {e}")
    else:
        loge(f"Portable source directory not found: {portable_source_dir}")


    # 使用字符串格式化而非f-string来避免花括号转义问题
    iss_content = '''
; Auto-generated installer script
#define MyAppName "{app_name}"
#define MyAppVersion "{version}"
#define MyAppPublisher "ZhangFeng"
#define MyAppURL "https://github.com/frankzhangv5/ScreenCast"
#define MyAppExeName "{app_target_name}"
#define SourceDir "{source_dir}"
#define MyOutputDir "{output_dir}"
#define MyAppIcon "{app_icon}"
#define MyLanguageDir "{language_dir}"
#define MyInstallerName "{installer_name}"

[Setup]
AppId={{{{08002A78-CCA5-4196-9B6A-FD8301627357}}}}
AppName={{#MyAppName}}
AppVersion={{#MyAppVersion}}
AppVerName={{#MyAppName}} {{#MyAppVersion}}
AppPublisher={{#MyAppPublisher}}
AppPublisherURL={{#MyAppURL}}
AppSupportURL={{#MyAppURL}}
AppUpdatesURL={{#MyAppURL}}
DefaultDirName={{autopf}}\{{#MyAppName}}
DisableProgramGroupPage=yes
OutputDir={{#MyOutputDir}}
OutputBaseFilename={{#MyInstallerName}}-Setup
SetupIconFile={{#MyAppIcon}}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64

[Languages]
Name: "English"; MessagesFile: "compiler:Default.isl"
Name: "ChineseSimplified"; MessagesFile: "{{#MyLanguageDir}}\\ChineseSimplified.isl"
Name: "ChineseTraditional"; MessagesFile: "{{#MyLanguageDir}}\\ChineseTraditional.isl"

[Tasks]
Name: "desktopicon"; Description: "{{cm:CreateDesktopIcon}}"; GroupDescription: "{{cm:AdditionalIcons}}"
Name: "quicklaunchicon"; Description: "{{cm:CreateQuickLaunchIcon}}"; GroupDescription: "{{cm:AdditionalIcons}}"; Flags: unchecked

[Files]
Source: "{{#SourceDir}}\\*"; DestDir: "{{app}}"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: *.pdb, *.ilk, *.exp, *.obj, debug

[Icons]
Name: "{{autoprograms}}\{{#MyAppName}}"; Filename: "{{app}}\{{#MyAppExeName}}"
Name: "{{autodesktop}}\{{#MyAppName}}"; Filename: "{{app}}\{{#MyAppExeName}}"; Tasks: desktopicon
Name: "{{userappdata}}\\Microsoft\\Internet Explorer\\Quick Launch\{{#MyAppName}}"; Filename: "{{app}}\{{#MyAppExeName}}"; Tasks: quicklaunchicon

[Run]
Filename: "{{app}}\{{#MyAppExeName}}"; Description: "{{cm:LaunchProgram,{{#StringChange(MyAppName, '&', '&&')}}}}"; Flags: nowait postinstall skipifsilent
'''.format(
        app_name=app_name,
        version=version,
        app_target_name=app_target_name,
        source_dir=os.path.normpath(app_dist_dir),
        output_dir=os.path.normpath(installer_dir),
        app_icon=os.path.normpath(app_icon_path) if app_icon_path.exists() else '',
        language_dir=os.path.normpath(languages_dir),
        installer_name=f"{app_name}-v{version}-windows-{arch}"
    )
    
    iss_path = installer_dir / "win_installer.iss"
    with open(iss_path, 'w', encoding='utf-8') as f:
        f.write(iss_content)
    
    logi(f"Running Inno Setup to create installer...")
    result = subprocess.run(
        [iscc_path, str(iss_path), "/Q"],
        capture_output=True,
        text=True,
        encoding='utf-8',
        errors='ignore'
    )
    
    if result.returncode != 0:
        loge(f"Error running Inno Setup: {result.stderr}")
        return False
    
    # 继续执行安装程序的创建
    if result.returncode == 0:
        installer_path = installer_dir / f"{app_name}-v{version}-windows-{arch}-Setup.exe"
        if installer_path.exists():
            logi(f"Windows installer created successfully: {installer_path}")
            logi(f"File size: {os.path.getsize(installer_path) / 1024 / 1024:.2f} MB")
            return True
        else:
            loge("Windows installer file not found after successful compilation")
            return False
    else:
        loge(f"Inno Setup compilation failed with exit code: {result.returncode}")
        logd(f"Error: {result.stderr}")
        return False

def package_linux(app_dist_dir, app_target_name, script_dir, build_type):
    """Linux平台打包"""
    logi("Creating Linux package...")
    
    # 读取版本信息
    version = get_version_from_file(script_dir)
    
    # 获取系统架构
    arch = platform.machine()
    arch_name = "amd64" if arch == "x86_64" else ("arm64" if arch in ["aarch64", "arm64"] else arch)
    
    # 检查打包工具
    pack_dir = script_dir / "pack"
    linuxdeploy_tool = pack_dir / "tools" / "linuxdeployqt-x86_64.AppImage"
    appimagetool = pack_dir / "tools" / "appimagetool-x86_64.AppImage"
    
    if not (linuxdeploy_tool.exists() and appimagetool.exists()):
        loge("Linux packaging tools not found. Please ensure linuxdeployqt and appimagetool are available in pack/tools/")
        logi("Skipping Linux package creation.")
        return False
    
    # 创建AppDir结构
    appdir = pack_dir / "AppDir"
    if appdir.exists():
        shutil.rmtree(appdir)
    appdir.mkdir(parents=True, exist_ok=True)
    
    # 创建必要的目录结构
    (appdir / "usr" / "bin").mkdir(parents=True, exist_ok=True)
    (appdir / "usr" / "share" / "applications").mkdir(parents=True, exist_ok=True)
    
    # 复制可执行文件
    shutil.copy2(app_dist_dir / app_target_name, appdir / "usr" / "bin")
    
    # 复制desktop文件
    desktop_file = pack_dir / "app.desktop"
    if desktop_file.exists():
        shutil.copy2(desktop_file, appdir / "usr" / "share" / "applications" / "ScreenCast.desktop")
    else:
        # 创建默认desktop文件
        with open(appdir / "usr" / "share" / "applications" / "ScreenCast.desktop", 'w') as f:
            f.write('''
[Desktop Entry]
Name=ScreenCast
Comment=Screen recording and casting tool
Exec=ScreenCast
Icon=ScreenCast
Terminal=false
Type=Application
Categories=Utility;Graphics;
''')
    
    # 创建AppRun脚本
    apprun_content = '''
#!/bin/bash

HERE="$(dirname "$(readlink -f "$0")")"
export QT_QPA_PLATFORM_PLUGIN_PATH="${HERE}/usr/plugins"
export LD_LIBRARY_PATH="${HERE}/usr/lib:${LD_LIBRARY_PATH}"

# Set XDG_DATA_DIRS to include our icons
export XDG_DATA_DIRS="${HERE}/usr/share:${XDG_DATA_DIRS:-/usr/local/share:/usr/share}"

# Set application name for better desktop integration
export DESKTOP_FILE="${HERE}/usr/share/applications/ScreenCast.desktop"

# Set application name for Qt
export QAPPLICATION_NAME="ScreenCast"
export QAPPLICATION_ORGANIZATION="ZhangFeng"

# For debugging: uncomment to see window class
# export QT_DEBUG_PLUGINS=1

exec "${HERE}/usr/bin/ScreenCast" "$@"
'''
    
    apprun_path = appdir / "AppRun"
    with open(apprun_path, 'w') as f:
        f.write(apprun_content)
    os.chmod(apprun_path, 0o755)
    
    logi("Skipping Linux package creation in Windows environment. Please run on Linux for proper packaging.")
    logi("For Linux packaging, use the linux.sh script in the pack directory.")
    return False

def package_macos(app_dist_dir, app_target_name, script_dir, build_type):
    """macOS平台打包"""
    logi("Creating macOS package...")
    
    # 读取版本信息
    version = get_version_from_file(script_dir)
    
    # 获取系统架构
    arch = platform.machine()
    arch_name = "x86_64" if arch == "x86_64" else ("arm64" if arch == "arm64" else arch)
    
    logi("Skipping macOS package creation in non-macOS environment. Please run on macOS for proper packaging.")
    logi("For macOS packaging, use the macos.sh script in the pack directory.")
    return False

def main():
    """主函数"""
    parser = argparse.ArgumentParser(description='ScreenMirror项目部署脚本')
    parser.add_argument('--configuration', choices=['Debug', 'Release'], 
                       default='Release', help='构建配置 (默认: Release)')
    parser.add_argument('--qt-dir', default=os.environ.get('QTDIR'), 
                       help='Qt安装目录路径')
    parser.add_argument('--clean', action='store_true', 
                       help='清理构建目录')
    parser.add_argument('--no-deploy', action='store_true', 
                       help='不进行部署')
    parser.add_argument('--package', action='store_true', 
                       help='打包生成最终安装包')

    args = parser.parse_args()
    
    # 查找Qt目录
    qt_dir = args.qt_dir or find_qt_dir()
    
    # 根据平台检查qmake存在性
    if IS_WINDOWS:
        qmake_name = "qmake.exe"
    else:
        qmake_name = "qmake"
    
    if not qt_dir or not (Path(qt_dir) / "bin" / qmake_name).exists():
        loge(f"Error: Invalid Qt path or {qmake_name} not found")
        sys.exit(1)
    
    # 查找编译器路径
    compiler_path = find_compiler_path(qt_dir)
    
    # Windows平台需要检查mingw
    if IS_WINDOWS:
        if not compiler_path or not (Path(compiler_path) / "bin/mingw32-make.exe").exists():
            loge("Error: mingw32-make not found")
            sys.exit(1)
    
    # 设置环境变量
    if IS_WINDOWS:
        # Windows环境变量设置
        os.environ['PATH'] = f"{compiler_path}/bin;{qt_dir}/bin;{os.environ['PATH']}"
    else:
        # Linux/macOS环境变量设置
        os.environ['PATH'] = f"{qt_dir}/bin:{os.environ['PATH']}"
    
    os.environ['QTDIR'] = qt_dir

    # 仅在Windows平台设置默认路径，其他平台使用系统环境变量
    DEBUG_SERVER = False
    if IS_WINDOWS and DEBUG_SERVER:
        if not os.environ.get('ANDROID_NDK_HOME'):
            os.environ['ANDROID_NDK_HOME'] = 'D:/Programs/Android/Sdk/ndk/29.0.14206865'
        if not os.environ.get('OHOS_SDK_HOME'):
            os.environ['OHOS_SDK_HOME'] = 'D:/Openharmony/Sdk/5.1.0.107'

    # 设置构建目录
    script_dir = Path(__file__).parent
    build_dir = script_dir / "build"
    dist_dir = script_dir / "dist"
    build_type = args.configuration.lower()
    
    # 清理构建目录
    if args.clean:
        logi("Cleaning build directories...")
        if build_dir.exists():
            shutil.rmtree(build_dir)
        if dist_dir.exists():
            shutil.rmtree(dist_dir)
    
    # 显示当前平台信息
    logi(f"Building for platform: {CURRENT_PLATFORM}")
    
    # 构建mirror_server
    build_mirror_server(script_dir)
    
    # 构建插件
    plugin_dist_dir = build_plugins(script_dir, build_dir, dist_dir, build_type)
    
    # 构建主应用程序
    app_dist_dir, app_target_name = build_application(script_dir, build_dir, dist_dir, build_type)
    
    # 部署
    if not args.no_deploy:
        deploy_application(app_dist_dir, app_target_name, qt_dir, plugin_dist_dir, script_dir)
        
        # 打包生成安装包
        if args.package:
            package_application(app_dist_dir, app_target_name, script_dir, build_type)
    else:
        logi("Build completed without deployment")
    
    logi("=== Build Complete ===")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        loge("\nBuild interrupted by user.")
        sys.exit(1)
    except Exception as e:
        loge(f"Unexpected error: {e}")
        sys.exit(1)