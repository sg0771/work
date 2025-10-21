# -*- coding: gbk -*-
import os
import sys
import datetime
import re

def update_version(filename):
    # 获取当前时间作为版本号
    now = datetime.datetime.now()
    version_major = now.year  # 年
    version_minor = now.month   # 月
    version_patch =  now.day # 日
    version_build =   now.hour # 小时

    # 定义新的版本号
    new_file_version = f"{version_major},{version_minor},{version_patch},{version_build}"
    new_product_version = f"{version_major},{version_minor},{version_patch},{version_build}"
    new_file_version_str = f"{version_major}.{version_minor}.{version_patch}.{version_build}"
    new_product_version_str = f"{version_major}.{version_minor}.{version_patch}.{version_build}"

    encodings = ['utf-8', 'utf-16', 'gbk']
    content = None
    for encoding in encodings:
        try:
            with open(filename, "r", encoding=encoding) as file:
                content = file.read()
            print(f"using {encoding} Read File",filename)
            break
        except UnicodeDecodeError:
            continue

    if content is None:
        print("Not Read File ",filename)
        return

    # 使用正则表达式替换版本号
    content = re.sub(
        r"FILEVERSION\s+\d+,\d+,\d+,\d+",
        f"FILEVERSION {new_file_version}",
        content,
    )

    content = re.sub(
        r"PRODUCTVERSION\s+\d+,\d+,\d+,\d+",
        f"PRODUCTVERSION {new_product_version}",
        content,
    )

    content = re.sub(
        r'VALUE "FileVersion",\s+"[\d.]+"',
        f'VALUE "FileVersion", "{new_file_version_str}"',
        content,
    )

    content = re.sub(
        r'VALUE "ProductVersion",\s+"[\d.]+"',
        f'VALUE "ProductVersion", "{new_product_version_str}"',
        content,
    )

    # 写回文件，使用读取时成功的编码
    try:
        with open(filename, "w", encoding=encoding) as file:
            file.write(content)
        print(f"Updated version.rc to version {new_file_version_str}")
    except Exception as e:
        print(f"Write file: {e}")


def traverse_rc_files(directory):
    for root, dirs, files in os.walk(directory):
        for file in files:
            if file.endswith('.rc'):
                filename = os.path.join(root, file)
                print(filename)
                update_version(filename)


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("请提供目标目录作为参数。")
        sys.exit(1)
    target_directory = sys.argv[1]
    if not os.path.isdir(target_directory):
        print("你提供的不是一个有效的目录。")
        sys.exit(1)
    traverse_rc_files(target_directory)
