# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS file helpers
"""

import glob
import os
import re
import subprocess
import sys
import zipfile


def filter_filenames(alldatafiles, start, end):
    """
    Filter datafilenames to the range given
    """
    filtered = []
    for filename in alldatafiles:
        number = int(filename.split('_')[-2][:-2])
        if end >= number >= start:
            filtered.append(filename)
    return filtered


def return_filelist(data_dir):
    """
    Return list of names of dnsfiles in data_dir
    """
    filelist = []
    if not os.path.isdir(data_dir):
        return []
    for fname in sorted(os.listdir(data_dir)):
        if re.match(r".*?_[0-9]+.d_dat", fname):
            filelist.append(fname)
    return filelist


def return_standard_zip(data_dir):
    if not os.path.isdir(data_dir):
        return ''
    zipfiles = sorted(glob.glob(data_dir + "/standard*.zip"))
    if zipfiles:
        latest_standard_file = max(zipfiles, key=os.path.getmtime)
        return latest_standard_file
    return ''


def unzip_latest_standard(data_dir, standarddir):
    latest_zip = return_standard_zip(standarddir)
    if not latest_zip:
        latest_zip = return_standard_zip(data_dir)
    if latest_zip:
        with zipfile.ZipFile(latest_zip, "r") as zip_ref:
            zip_ref.extractall(standarddir)
        return True
    return False


def create_dir_from_filename(filename):
    cddir = os.path.dirname(filename)
    create_dir(cddir)


def create_dir(current_dir):
    if not os.path.exists(current_dir):
        os.makedirs(current_dir)


def save_txt(txt, filename, current_dir=None):
    if current_dir is not None and current_dir:
        current_path = ''.join((current_dir, '/', filename))
    else:
        current_path = filename
    with open(current_path, 'w', encoding="utf8") as myfile:
        myfile.write(txt)
    return [filename, current_path]


def load_txt(filename, current_dir=None):
    if current_dir is not None and current_dir:
        current_path = ''.join((current_dir, '/', filename))
    else:
        current_path = filename
    with open(current_path, 'r', encoding="utf8") as myfile:
        txt = myfile.readlines()
    return txt


def open_editor(filename, current_dir=None):
    if current_dir is not None and current_dir:
        current_path = ''.join((current_dir, '/', filename))
    else:
        current_path = filename
    if os.path.exists(current_path):
        if sys.platform.startswith("win"):
            os.startfile(current_path)
        elif sys.platform.startswith("linux"):
            subprocess.call(["xdg-open", current_path])
        elif sys.platform == "darwin":
            subprocess.call(["open", current_path])


def get_path_and_prefix(path):
    prefix = os.path.basename(path)
    path = os.path.dirname(path)
    return path, prefix
