# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS Widget, mainly for DI of view and model to presenter
"""


class DNSWidget:
    """widget that owns presenter, view and model"""

    def __init__(self, name, parent):
        self.parent = parent
        self.view = None
        self.presenter = None
        self.model = None
        self.name = name

    def has_view(self):
        return self.view is not None

    def has_model(self):
        return self.model is not None

    def has_presenter(self):
        return self.presenter is not None

    def update_progress(self, i, end_i=None):
        self.presenter.update_progress(i, end_i)
