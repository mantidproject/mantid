# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import os


def compile_ui(ui_filename):
    """
    Hand this a ui file and it will make a py file.
    """
    pyuic_dir = os.path.dirname(ui_filename)
    pyuic_filename = "ui_%s.py" % os.path.basename(ui_filename).split('.')[0]
    command = "pyuic4 -o %s/%s %s" % (pyuic_dir, pyuic_filename, ui_filename)
    os.system(command)
