#!/usr/bin/env python3
"""
格式化C++代码的Python脚本
替换原有的format.ps1 PowerShell脚本
"""

import os
import sys
import subprocess
from pathlib import Path

def format_cpp_files():
    """格式化所有C++代码文件"""
    total = 0
    
    print("Starting code formatting...", flush=True)
    
    # 搜索目录配置
    search_dirs = [
        "./app/src",
        "./app/include",
        "./middleware/src",
        "./middleware/include",
        "./sdk/src",
        "./sdk/include",
        "./plugins",
        "./server/",
    ]
    
    # 支持的扩展名
    extensions = ['.h', '.cpp']
    
    # 遍历所有目录查找C++文件
    for search_dir in search_dirs:
        search_path = Path(search_dir)
        if not search_path.exists():
            print(f"Warning: Directory {search_dir} does not exist, skipping...")
            continue
            
        for ext in extensions:
            pattern = f"**/*{ext}"
            for file_path in search_path.glob(pattern):
                try:
                    # 格式化文件
                    result = subprocess.run([
                        'clang-format', '-i', '--style=file', '--verbose', str(file_path)
                    ], capture_output=True, text=True, check=True)
                    
                    print(f"Formatted: {file_path}")
                    total += 1
                    
                except subprocess.CalledProcessError as e:
                    print(f"Error formatting {file_path}: {e.stderr}", file=sys.stderr)
                except Exception as e:
                    print(f"Unexpected error formatting {file_path}: {e}", file=sys.stderr)
    
    print(f"\nFormatting completed!")
    print(f"Total files processed: {total}")
    
    return total

if __name__ == "__main__":
    try:
        format_cpp_files()
    except KeyboardInterrupt:
        print("\nFormatting interrupted by user.")
        sys.exit(1)
    except Exception as e:
        print(f"Unexpected error: {e}", file=sys.stderr)
        sys.exit(1)