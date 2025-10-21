# -*- coding: gbk -*-
import os
import sys
import datetime
import re

def update_version(filename):
    # ��ȡ��ǰʱ����Ϊ�汾��
    now = datetime.datetime.now()
    version_major = now.year  # ��
    version_minor = now.month   # ��
    version_patch =  now.day # ��
    version_build =   now.hour # Сʱ

    # �����µİ汾��
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

    # ʹ��������ʽ�滻�汾��
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

    # д���ļ���ʹ�ö�ȡʱ�ɹ��ı���
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
        print("���ṩĿ��Ŀ¼��Ϊ������")
        sys.exit(1)
    target_directory = sys.argv[1]
    if not os.path.isdir(target_directory):
        print("���ṩ�Ĳ���һ����Ч��Ŀ¼��")
        sys.exit(1)
    traverse_rc_files(target_directory)
