import os
import shutil
import subprocess
import sys
from pathlib import Path
from xml.etree import ElementTree as ET


def generate_qrc():
    # 定义资源目录和输出文件
    res_root = "res"
    qrc_file = "resources.qrc"
    target_dirs = ["icons", "i18n", 'images', 'server', 'app_icons' ]

    # 创建 XML 根节点
    rcc = ET.Element("RCC", version="1.0")

    # 遍历目标目录
    for dir_name in target_dirs:
        dir_path = os.path.join(res_root, dir_name)
        if not os.path.exists(dir_path) or not os.listdir(dir_path):
            print(f"⚠️ 目录 {dir_path} 不存在或为空，已跳过")
            continue

        # 创建 qresource 节点（前缀设置为目录名）
        qresource = ET.SubElement(rcc, "qresource", prefix=f"/{dir_name}")

        # 收集所有文件路径（相对于项目根目录）
        for root, _, files in os.walk(dir_path):
            for file in files:
                abs_path = os.path.join(root, file)
                rel_path = os.path.relpath(abs_path, start=os.curdir)
                # 提取文件名作为别名
                alias_name = os.path.splitext(os.path.basename(rel_path))[0]
                ET.SubElement(qresource, "file", alias=alias_name).text = rel_path.replace('\\', '/')

    # 生成 XML 文件
    tree = ET.ElementTree(rcc)
    ET.indent(tree, space="  ", level=0)
    tree.write(qrc_file, encoding="utf-8", xml_declaration=False)

    print(f"✅ 资源文件 {qrc_file} 生成成功")


def generate_ts_files():
    # 检查 lupdate 工具可用性
    if not shutil.which("lupdate"):
        print("❌ 未找到 lupdate 工具，请确保已安装 Qt 工具链")
        return False

    # 配置参数
    source_dir = Path(".")  # Python源码目录
    output_dir = Path("res/translations")  # 输出目录
    languages = ['app_en_US']
    # 确保输出目录存在
    output_dir.mkdir(parents=True, exist_ok=True)

    pro_files = list(source_dir.glob("*.pro"))
    if not pro_files:
        print(f"在 {source_dir} 中未找到.pro文件")
        return

    # 构建文件列表
    file_list = [str(p) for p in pro_files]

    # 生成每个语言的TS文件
    for lang in languages:
        ts_file = output_dir / f"{lang}.ts"

        cmd = [
                  "lupdate",
                  "-verbose",
                  "-noobsolete",
              ]
        cmd.extend(file_list)
        cmd.extend(["-ts", str(ts_file)])
        # 执行命令
        print(f"为 {lang} 生成翻译文件: {ts_file}")
        print(' '.join(cmd))
        result = subprocess.run(cmd, capture_output=True, text=True)

        # 检查结果
        if result.returncode == 0:
            print(result.stdout)
            print(f"成功生成 {ts_file} ({len(pro_files)}个文件处理)")
        else:
            print(f"生成失败! 错误信息: {result.stderr}")
            print("命令:", " ".join(cmd))


def compile_i18n():
    # 配置路径
    source_dir = Path("res/translations")
    dest_dir = Path("res/i18n")

    # 确保目标目录存在
    if dest_dir.exists():
        shutil.rmtree(dest_dir, ignore_errors=True)

    dest_dir.mkdir(parents=True, exist_ok=True)

    print(f"🔍 扫描翻译文件: {source_dir}")
    ts_files = list(f for f in source_dir.glob("*.ts") if f.name != "app_en_US.ts")

    if not ts_files:
        print("⚠️ 未找到任何 .ts 文件")
        return False

    # 检查 lrelease 工具可用性
    if not shutil.which("lrelease"):
        print("❌ 未找到 lrelease 工具，请确保已安装 Qt 工具链")
        return False

    print(f"🛠 使用工具: lrelease")

    success_count = 0
    fail_list = []

    # 处理每个 TS 文件
    for ts_file in ts_files:
        qm_file = dest_dir / f"{ts_file.stem}.qm"

        print(f"\n📄 处理: {ts_file.name}")

        # 构建编译命令
        cmd = ["lrelease", "-compress", "-silent", str(ts_file), "-qm", str(qm_file)]
        print(' '.join(cmd))
        try:
            # 执行编译命令
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=30  # 超时设置
            )

            if result.returncode == 0:
                print(result.stdout)

                # 验证文件是否生成
                if qm_file.exists():
                    print(f"✅ 成功生成: {qm_file}")
                    success_count += 1
                else:
                    print(f"⛔ 文件未生成: {qm_file}")
                    print(f"错误: {result.stderr or '无错误信息'}")
                    fail_list.append(ts_file.name)
            else:
                print(f"⚠️ 编译失败 [代码 {result.returncode}]: {ts_file}")
                print(f"错误详情: {result.stderr}")
                fail_list.append(ts_file.name)

        except Exception as e:
            print(f"🚨 处理 {ts_file.name} 时发生意外错误: {str(e)}")
            fail_list.append(ts_file.name)

    # 结果报告
    print("\n" + "=" * 50)
    print(f"🌍 翻译文件编译完成")
    print(f"✅ 成功: {success_count}/{len(ts_files)}")

    if fail_list:
        print(f"❌ 失败列表:")
        for f in fail_list:
            print(f"  - {f}")

    return len(fail_list) == 0


if __name__ == "__main__":
    generate_ts_files()
    compile_i18n()

    generate_qrc()
