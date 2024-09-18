# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
DNS file processing helpers.
"""

import glob
import os
import re
import subprocess
import sys
import zipfile


def filter_filenames(all_datafiles, start, end):
    """
    Filter data filenames to the range given.
    """
    filtered = []
    for filename in all_datafiles:
        digit_patterns = re.findall(r"\d+", filename)
        empty_delimiter = ""
        number = int(empty_delimiter.join(digit_patterns))
        if end >= number >= start:
            filtered.append(filename)
    return filtered


def return_filelist(data_dir):
    """
    Return list of names of dns files in data_dir.
    """
    filelist = []
    if not os.path.isdir(data_dir):
        return []
    for file_name in sorted(os.listdir(data_dir)):
        if file_name.endswith(".d_dat"):
            filelist.append(file_name)
    return filelist


def return_standard_zip(data_dir):
    if not os.path.isdir(data_dir):
        return ""
    zip_files = sorted(glob.glob(data_dir + "/standard*.zip"))
    if zip_files:
        latest_standard_file = max(zip_files, key=os.path.getmtime)
        return latest_standard_file
    return ""


def unzip_latest_standard(data_dir, standard_dir):
    latest_zip = return_standard_zip(standard_dir)
    if not latest_zip:
        latest_zip = return_standard_zip(data_dir)
    if latest_zip:
        with zipfile.ZipFile(latest_zip, "r") as zip_ref:
            zip_ref.extractall(standard_dir)
        return True
    return False


def create_dir_from_filename(filename):
    cur_dir = os.path.dirname(filename)
    create_dir(cur_dir)


def create_dir(current_dir):
    if not os.path.exists(current_dir):
        os.makedirs(current_dir)


def save_txt(txt, filename, current_dir=None):
    if current_dir is not None and current_dir:
        current_path = "".join((current_dir, "/", filename))
    else:
        current_path = filename
    with open(current_path, "w", encoding="utf8") as my_file:
        my_file.write(txt)
    return [filename, current_path]


def load_txt(filename, current_dir=None):
    if current_dir is not None and current_dir:
        current_path = "".join((current_dir, "/", filename))
    else:
        current_path = filename
    with open(current_path, "r", encoding="utf8") as my_file:
        txt = my_file.read()
    return txt


def open_editor(filename, current_dir=None):
    if current_dir is not None and current_dir:
        current_path = "".join((current_dir, "/", filename))
    else:
        current_path = filename
    if os.path.exists(current_path):
        if sys.platform.startswith("win"):
            subprocess.call(["cmd.exe", "/c", current_path])
        elif sys.platform.startswith("linux"):
            subprocess.call(["xdg-open", current_path])
        elif sys.platform == "darwin":
            subprocess.call(["open", current_path])


def get_path_and_prefix(path):
    prefix = os.path.basename(path)
    path = os.path.dirname(path)
    return path, prefix
