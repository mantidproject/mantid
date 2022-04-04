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


def return_filelist(datadir):
    """
    Return list of names of dnsfiles in datadir
    """
    filelist = []
    if not os.path.isdir(datadir):
        return []
    for fname in sorted(os.listdir(datadir)):
        if re.match(r".*?_[0-9]+.d_dat", fname):
            filelist.append(fname)
    return filelist


def return_standard_zip(datadir):
    if not os.path.isdir(datadir):
        return ''
    zipfiles = sorted(glob.glob(datadir + "/standard*.zip"))
    if zipfiles:
        latest_standard_file = max(zipfiles, key=os.path.getmtime)
        return latest_standard_file
    return ''


def unzip_latest_standard(datadir, standarddir):
    latest_zip = return_standard_zip(standarddir)
    if not latest_zip:
        latest_zip = return_standard_zip(datadir)
    if latest_zip:
        with zipfile.ZipFile(latest_zip, "r") as zip_ref:
            zip_ref.extractall(standarddir)
        return True
    return False


def create_dir_from_filename(filename):
    cddir = os.path.dirname(filename)
    create_dir(cddir)


def create_dir(crdir):
    if not os.path.exists(crdir):
        os.makedirs(crdir)


def save_txt(txt, filename, crdir=None):
    if crdir is not None and crdir:
        crpath = ''.join((crdir, '/', filename))
    else:
        crpath = filename
    with open(crpath, 'w', encoding="utf8") as myfile:
        myfile.write(txt)
    return [filename, crpath]


def load_txt(filename, crdir=None):
    if crdir is not None and crdir:
        crpath = ''.join((crdir, '/', filename))
    else:
        crpath = filename
    with open(crpath, 'r', encoding="utf8") as myfile:
        txt = myfile.readlines()
    return txt


def open_editor(filename, crdir=None):
    if crdir is not None and crdir:
        crpath = ''.join((crdir, '/', filename))
    else:
        crpath = filename
    if os.path.exists(crpath):
        if sys.platform.startswith("win"):
            os.startfile(crpath)
        elif sys.platform.startswith("linux"):
            subprocess.call(["xdg-open", filename])
        elif sys.platform == "darwin":
            subprocess.call(["open", filename])


def get_path_and_prefix(path):
    prefix = os.path.basename(path)
    path = os.path.dirname(path)
    return path, prefix
