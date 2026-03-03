# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from mantidqtinterfaces.sans_isis.gui_logic.presenter.run_tab_presenter import RunTabPresenter


class ConcreteRunTabListenerTest(unittest.TestCase):
    @staticmethod
    def test_on_view_update_updates_model():
        mocked_presenter = mock.Mock(spec=RunTabPresenter)
        listener = RunTabPresenter.ConcreteRunTabListener(presenter=mocked_presenter)

        listener.on_field_edit()
        mocked_presenter.update_model_from_view.assert_called_once()


if __name__ == "__main__":
    unittest.main()
