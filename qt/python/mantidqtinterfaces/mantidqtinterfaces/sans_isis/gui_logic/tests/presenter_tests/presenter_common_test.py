# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest import mock
from mantidqtinterfaces.sans_isis.gui_logic.presenter.presenter_common import PresenterCommon


class PresenterCommonMock(PresenterCommon):
    def default_gui_setup(self):
        pass

    def update_model_from_view(self):
        pass

    def update_view_from_model(self):
        pass


class PresenterCommonTest(unittest.TestCase):
    def _get_mocked_presenter(self):
        self.mocked_view = mock.Mock()
        self.presenter_model = mock.Mock()

        return PresenterCommonMock(view=self.mocked_view, model=self.presenter_model)

    def test_set_on_view_rejects_invalid(self):
        attr_name = "test_attr"
        invalid_list = [None, ""]
        for val in invalid_list:
            presenter = self._get_mocked_presenter()
            self.presenter_model.test_attr = val
            presenter._set_on_view(attr_name)  # Presenter should copy model->view
            self.assertNotEqual(val, self.mocked_view.test_attr, "Val was incorrectly accepted")

    def test_set_on_view_accepts_falsey(self):
        attr_name = "test_attr"
        should_accept = [0.0, 0, False, 1.0, 1]
        for val in should_accept:
            presenter = self._get_mocked_presenter()
            self.presenter_model.test_attr = val
            presenter._set_on_view(attr_name)  # Presenter should copy model->view
            self.assertEqual(val, self.mocked_view.test_attr, "Val was incorrectly rejected")

    def test_set_on_model(self):
        mocked_presenter = self._get_mocked_presenter()
        custom_model = mock.Mock()

        attr_to_set = "test_attr"
        original_val = "not_changed"
        new_val = "views val"

        setattr(self.presenter_model, attr_to_set, original_val)
        setattr(custom_model, attr_to_set, original_val)

        setattr(self.mocked_view, attr_to_set, new_val)

        self.assertEqual(original_val, getattr(self.presenter_model, attr_to_set))
        self.assertEqual(original_val, getattr(custom_model, attr_to_set))

        mocked_presenter._set_on_custom_model(attribute_name=attr_to_set, model=custom_model)
        self.assertEqual(new_val, getattr(custom_model, attr_to_set))
        self.assertEqual(original_val, getattr(self.presenter_model, attr_to_set))


if __name__ == "__main__":
    unittest.main()
