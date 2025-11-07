#!/usr/bin/env python3
"""
Cross-platform Python script to build Android NDK C++ executable using Android.mk

This script provides a platform-independent way to build Android NDK projects
using the same functionality as the PowerShell build script.

Usage:
    python build.py [--ndk-path NDK_PATH] [--build-dir BUILD_DIR] [--release-dir RELEASE_DIR]

Arguments:
    --ndk-path: Path to Android NDK (default: $ANDROID_NDK_HOME or default path)
    --build-dir: Build directory (default: current directory)
    --release-dir: Release output directory (default: ./release)
"""

import os
import sys
import subprocess
import shutil
import argparse
from pathlib import Path
import re


def update_application_mk(arch: str):
    """Update Application.mk with the specified architecture."""
    app_mk_path = Path("Application.mk")
    
    # Read current content
    with open(app_mk_path, 'r') as f:
        content = f.read()
    
    # Update APP_ABI line
    new_content = re.sub(r'APP_ABI := .*', f'APP_ABI := {arch}', content, flags=re.MULTILINE)
    
    # Write back to file
    with open(app_mk_path, 'w') as f:
        f.write(new_content)
    
    print(f"Updated Application.mk with architecture: {arch}")


def build_with_android_mk(ndk_path: str, release_dir: str, arch: str):
    """Build using Android.mk with the specified NDK path and architecture."""
    print("Building with Android.mk...")
    
    # Update Application.mk with the specified architecture
    update_application_mk(arch)
    
    # Create release directory if it doesn't exist
    release_path = Path(release_dir)
    release_path.mkdir(parents=True, exist_ok=True)
    
    # Set NDK project path environment variable
    env = os.environ.copy()
    env['NDK_PROJECT_PATH'] = str(Path.cwd())
    
    # Determine the ndk-build command based on platform
    if os.name == 'nt':  # Windows
        ndk_build_cmd = str(Path(ndk_path) / "ndk-build.cmd")
    else:  # Unix-like systems
        ndk_build_cmd = str(Path(ndk_path) / "ndk-build")
    
    # Build command arguments
    cmd_args = [
        ndk_build_cmd,
        "NDK_APPLICATION_MK=Application.mk",
        "APP_BUILD_SCRIPT=Android.mk"
    ]
    
    # Run ndk-build
    print(f"Running: {' '.join(cmd_args)}")
    result = subprocess.run(cmd_args, env=env, capture_output=True, text=True)
    
    if result.returncode != 0:
        print(f"Android.mk build failed!")
        print(f"STDERR: {result.stderr}")
        print(f"STDOUT: {result.stdout}")
        sys.exit(1)
    
    # Copy executable to release directory
    libs_dir = Path("libs") / arch
    # Android NDK executable name (without 'android_' prefix)
    exe_path = libs_dir / "mirror_server"
    
    if exe_path.exists():
        target_path = release_path / "mirror_server"
        shutil.copy2(exe_path, target_path)
        print(f"Build done. Output: {target_path}")
        
        # Clean up build directories
        libs_dir = Path("libs")
        obj_dir = Path("obj")
        
        if libs_dir.exists():
            shutil.rmtree(libs_dir)
        if obj_dir.exists():
            shutil.rmtree(obj_dir)
    else:
        print(f"Error: Executable not found at expected location: {exe_path}")
        sys.exit(1)


def main():
    """Main function to parse arguments and run the build."""
    parser = argparse.ArgumentParser(description="Build Android NDK project")
    parser.add_argument("--ndk-path", 
                       default=os.environ.get('ANDROID_NDK_HOME', "E:\\Android\\Sdk\\ndk\\21.1.6352462"),
                       help="Path to Android NDK")
    parser.add_argument("--build-dir", 
                       default=".",
                       help="Build directory (default: current directory)")
    parser.add_argument("--release-dir", 
                       default="./release",
                       help="Release output directory (default: ./release)")
    parser.add_argument("--arch", 
                       default="arm64-v8a",
                       choices=['arm64-v8a', 'armeabi-v7a', 'x86', 'x86_64'],
                       help="Target architecture (default: arm64-v8a)")
    
    args = parser.parse_args()
    
    # Change to build directory if specified
    # 获取 build.py 所在的目录
    script_dir = os.path.dirname(os.path.abspath(__file__))
    os.chdir(script_dir)
    
    # Validate NDK path
    ndk_path = Path(args.ndk_path)
    if not ndk_path.exists():
        print(f"Error: NDK path '{ndk_path}' does not exist.")
        sys.exit(1)
    
    # Validate ndk-build command exists
    if os.name == 'nt':
        ndk_build_cmd = ndk_path / "ndk-build.cmd"
    else:
        ndk_build_cmd = ndk_path / "ndk-build"
    
    if not ndk_build_cmd.exists():
        print(f"Error: ndk-build command not found at '{ndk_build_cmd}'")
        sys.exit(1)
    
    # Run the build
    try:
        build_with_android_mk(str(ndk_path), args.release_dir, args.arch)
        print("Android NDK build completed successfully!")
    except Exception as e:
        print(f"Build failed with error: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()