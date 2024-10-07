# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""The run tab presenter.

This abstract class allows other presenters to share functions which set attributes on
the view or retrieve them from the view
"""

from abc import ABCMeta, abstractmethod


class PresenterCommon(metaclass=ABCMeta):
    def __init__(self, view, model):
        self._view = view
        self._model = model

    @abstractmethod
    def default_gui_setup(self):
        pass

    @abstractmethod
    def update_model_from_view(self):
        pass

    @abstractmethod
    def update_view_from_model(self):
        pass

    def set_view(self, view):
        assert view is not None
        self._view = view

    def set_model(self, model):
        assert model is not None
        self._model = model

    def _set_on_state_model(self, attribute_name):
        self._set_on_custom_model(attribute_name, self._model)

    def _set_on_view(self, attribute_name, decimal_places=None):
        self._set_on_view_to_custom_view(attribute_name, self._view, decimal_places)

    def _set_on_custom_model(self, attribute_name, model):
        attribute = getattr(self._view, attribute_name)
        if attribute is not None and attribute != "":
            setattr(model, attribute_name, attribute)

    def _set_on_view_to_custom_view(self, attribute_name, view, decimal_places):
        attribute = getattr(self._model, attribute_name)
        # We need to be careful here. We don't want to set empty strings, or None.
        if attribute is None or attribute == "":
            return
        if decimal_places:
            attribute = round(attribute, decimal_places)
        setattr(view, attribute_name, attribute)
