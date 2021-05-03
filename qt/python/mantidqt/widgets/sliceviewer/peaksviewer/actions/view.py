# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# coding=utf-8
#  This file is part of the mantidqt package.

from qtpy import QtWidgets, QtGui

class PeakActionsView:
    def __init__(self):
        peak_grid = QtGui.QGridLayout()

        self.add_btn = QtWidgets.QPushButton('Add peak', self)
        self.rm_btn = QtWidgets.QPushButton('Remove peak', self)
        self.peaks_menu = QtWidgets.QComboBox()

        wksp_list = ["Workspace 1", "Workspace 2"]
        self.peaks_menu.addItems(wksp_list)

        self.add_btn.setStyleSheet("background-color:lightgrey")
        self.rm_btn.setStyleSheet("background-color:lightgrey")

        self.add_btn.clicked.connect(self.add_peak_click)
        self.rm_btn.clicked.connect(self.rm_peak_click)

        peak_grid.addWidget(self.add_btn)
        peak_grid.addWidget(self.rm_btn)
        peak_grid.addWidget(self.peaks_menu)

        self.setLayout(peak_grid)

    def add_btn_click(self):
        pass

    def rm_btn_click(self):
        pass
