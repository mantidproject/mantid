# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#

# 3rd party imports
from qtpy.QtWidgets import QMessageBox
from qtpy.QtGui import QIcon

# local imports
from mantid import simpleapi


def mantid_algorithm_used(content):
    for attr in dir(simpleapi):
        if not attr.startswith('_') and attr == attr.title():
            if attr in content:
                return True
    return False


def mantid_api_import_needed(content):
    """Check if a python script's contents uses Mantid algorithms
    but does not import Mantid's SimpleAPI. Launch QMessageBox to
    ask if import should be added.
    """
    if not mantid_api_imported(content):
        if mantid_algorithm_used(content):
            if permission_box_to_prepend_import():
                return True
    return False


def mantid_api_imported(content):
    if 'from mantid.simpleapi import ' in content:
        return True
    return False


def permission_box_to_prepend_import():
    msg_box = QMessageBox()
    msg_box.setWindowTitle("Mantid Workbench")
    msg_box.setWindowIcon(QIcon(':/images/MantidIcon.ico'))
    msg_box.setText("It looks like this python file uses a Mantid "
                    "algorithm but does not import the Mantid API.")
    msg_box.setInformativeText("Would you like to add a line to import "
                               "the Mantid API?")
    msg_box.setStandardButtons(QMessageBox.Yes | QMessageBox.No)
    msg_box.setDefaultButton(QMessageBox.Yes)
    permission = msg_box.exec_()
    if permission == QMessageBox.Yes:
        return True
    return False
