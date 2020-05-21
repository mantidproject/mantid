# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
#
from mantidqt.utils.qt import import_qt

from qtpy.QtWidgets import QVBoxLayout, QCheckBox, QWidget
from qtpy.QtCore import Qt

ImageInfoWidget_cpp = import_qt('.._common', 'mantidqt.widgets', 'ImageInfoWidget')


class ImageInfoWidget(ImageInfoWidget_cpp):

    def __init__(self, workspace, parent):
        super(ImageInfoWidget, self).__init__(workspace, parent)
