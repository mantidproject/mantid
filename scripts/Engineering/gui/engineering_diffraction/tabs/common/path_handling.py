# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)

from os import path

OUT_FILES_ROOT_DIR = path.join(path.expanduser("~"), "Engineering_Mantid")


def get_run_number_from_path(run_path, instrument):
    return path.splitext(path.basename(run_path))[0].replace(instrument, '').lstrip('0')
