#!/usr/bin/env python3
"""
Cross-platform Python script for building OpenHarmony screen mirror server using CMake
"""

import argparse
import os
import sys
import subprocess
import shutil
from pathlib import Path

def main():
    parser = argparse.ArgumentParser(description='Build OpenHarmony screen mirror server using CMake')
    parser.add_argument('--ohos-sdk-path', default=os.environ.get('OHOS_SDK_HOME'),
                       help='OpenHarmony SDK path (default: OHOS_SDK_HOME env var)')
    parser.add_argument('--build-dir', default='build',
                       help='Build directory (default: ./build)')
    parser.add_argument('--release-dir', default='release',
                       help='Release directory (default: ./release)')

    parser.add_argument('--arch', choices=['arm', 'aarch64', 'x86_64'], default='arm',
                       help='Target architecture (default: arm)')
    
    args = parser.parse_args()
    
    # Set default OHOS SDK path if not provided
    if not args.ohos_sdk_path:
        args.ohos_sdk_path = "E:/OpenHarmony/Sdk/15"
    
    # Convert paths to Path objects
    script_dir = Path(__file__).parent.absolute()
    os.chdir(script_dir)
    ohos_sdk_path = Path(args.ohos_sdk_path)
    build_dir = Path(args.build_dir)
    release_dir = Path(args.release_dir)

    # Error handling
    if not ohos_sdk_path:
        print("ERROR: OHOS_SDK_HOME environment variable is not set. Please set it to your OpenHarmony SDK path.")
        sys.exit(1)
    
    if not ohos_sdk_path.exists():
        print(f"ERROR: OHOS SDK path '{ohos_sdk_path}' does not exist.")
        sys.exit(1)
    
    # Check if CMake is available
    cmake_path = ohos_sdk_path / "native" / "build-tools" / "cmake" / "bin" / "cmake.exe"
    if not cmake_path.exists():
        print("ERROR: CMake is not available. Please install CMake and ensure it's in your PATH.")
        sys.exit(1)
    
    # Check if Ninja is available
    ninja_path = ohos_sdk_path / "native" / "build-tools" / "cmake" / "bin" / "ninja.exe"
    if not ninja_path.exists():
        print("ERROR: Ninja build tool is not available. Please ensure it's installed in the OHOS SDK.")
        sys.exit(1)
    
    # Set target platform according to architecture
    arch_mapping = {
        'arm': 'arm-linux-ohos',
        'aarch64': 'aarch64-linux-ohos', 
        'x86_64': 'x86_64-linux-ohos'
    }
    ohos_target_arch = arch_mapping[args.arch]
    
    # Check compiler paths
    cc_path = ohos_sdk_path / "native" / "llvm" / "bin" / "clang.exe"
    cxx_path = ohos_sdk_path / "native" / "llvm" / "bin" / "clang++.exe"
    if not cc_path.exists() or not cxx_path.exists():
        print("ERROR: Clang compiler not found in OHOS SDK. Please check your SDK installation.")
        sys.exit(1)
    
    # Create build and release directories
    build_dir.mkdir(parents=True, exist_ok=True)
    release_dir.mkdir(parents=True, exist_ok=True)
    
    # Configure CMake
    print("Configuring CMake project...")
    # Prepare path strings with forward slashes
    ohos_sdk_path_str = str(ohos_sdk_path).replace("\\", "/")
    toolchain_file_path = str(ohos_sdk_path / "native" / "build" / "cmake" / "ohos.toolchain.cmake").replace("\\", "/")
    
    cmake_cmd = [
        str(cmake_path),
        '-S', str(script_dir),
        '-B', str(build_dir),
        '-G', 'Ninja',
        f'-DOHOS_SDK_HOME={ohos_sdk_path_str}',
        '-DCMAKE_BUILD_TYPE=Release',
        f'-DCMAKE_MAKE_PROGRAM={str(ninja_path)}',
        f'-DCMAKE_TOOLCHAIN_FILE={toolchain_file_path}',
        f'-DOHOS_TARGET_ARCH={ohos_target_arch}'
    ]

    result = subprocess.run(cmake_cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print("ERROR: CMake configuration failed")
        print(result.stderr)
        sys.exit(1)

    # Build the project
    print("Building mirror_server...")
    build_cmd = [
        str(cmake_path),
        '--build', str(build_dir),
        '--target', 'mirror_server',
        '--config', 'Release'
    ]
    
    result = subprocess.run(build_cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print("ERROR: Build failed")
        print(result.stderr)
        sys.exit(1)
    
    # Copy the built executable to release directory
    built_exe = build_dir / "mirror_server"
    if built_exe.exists():
        release_exe = release_dir / "mirror_server"
        shutil.copy2(built_exe, release_exe)
        print(f"Build successful! Executable copied to: {release_exe}")
        shutil.rmtree(build_dir)
    else:
        print(f"ERROR: Built executable not found at expected location: {built_exe}")
        sys.exit(1)
    
    print("OpenHarmony CMake build completed successfully!")

if __name__ == "__main__":
    main()