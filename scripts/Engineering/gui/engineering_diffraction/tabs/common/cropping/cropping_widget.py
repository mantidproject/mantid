# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)

from Engineering.gui.engineering_diffraction.tabs.common.cropping.cropping_model import CroppingModel
from Engineering.gui.engineering_diffraction.tabs.common.cropping.cropping_view import CroppingView
from Engineering.gui.engineering_diffraction.tabs.common.cropping.cropping_presenter import CroppingPresenter


class CroppingWidget(object):
    def __init__(self, parent, view=None):
        if view is None:
            self.view = CroppingView(parent)
        else:
            self.view = view
        self.model = CroppingModel()
        self.presenter = CroppingPresenter(self.model, self.view)

    def is_valid(self):
        return self.presenter.is_valid()

    def get_custom_spectra(self):
        return self.presenter.get_custom_spectra()

    def get_bank(self):
        return str(self.presenter.get_bank())

    def is_custom(self):
        return self.presenter.get_custom_spectra_enabled()
