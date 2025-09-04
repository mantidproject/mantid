# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest import mock

from mantidqt.gui_helper import get_qapplication
from mantidqtinterfaces.dns_powder_tof.dns_modus import DNSModus
from mantidqtinterfaces.dns_powder_tof.main_presenter import DNSReductionGUIPresenter
from mantidqtinterfaces.dns_powder_tof.main_view import DNSReductionGUIView
from mantidqtinterfaces.dns_powder_tof.parameter_abo import ParameterAbo

app, within_mantid = get_qapplication()


class DNSReductionGUIPresenterTest(unittest.TestCase):
    # pylint: disable=protected-access, too-many-public-methods
    modus = None
    widget = None
    parameter_abo = None
    view = None
    parent = None
    command_line_reader = None

    @classmethod
    def setUpClass(cls):
        cls.parent = mock.Mock()
        cls.view = mock.create_autospec(DNSReductionGUIView, instance=True)
        cls.parameter_abo = mock.create_autospec(ParameterAbo, instance=True)
        cls.modus = mock.create_autospec(DNSModus, instance=True)
        cls.widget = mock.Mock()
        cls.modus.widgets = {"observer1": cls.widget}
        cls.widget.view = mock.Mock()
        # cls.widget.view.has_tab = True
        cls.widget.view.menus = True
        cls.widget.presenter = mock.Mock()
        cls.widget.presenter.view = cls.widget.view
        cls.parameter_abo.observers = [cls.widget.presenter]
        dummy_observer = mock.Mock()
        cls.parameter_abo.observer_dict = {"paths": dummy_observer, "file_selector": dummy_observer}

        cls.view.sig_tab_changed.connect = mock.Mock()
        cls.view.sig_save_as_triggered.connect = mock.Mock()
        cls.view.sig_save_triggered.connect = mock.Mock()
        cls.view.sig_open_triggered.connect = mock.Mock()
        cls.view.sig_modus_change.connect = mock.Mock()

        cls.presenter = DNSReductionGUIPresenter(
            name="reduction_gui",
            view=cls.view,
            parameter_abo=cls.parameter_abo,
            modus=cls.modus,
            parent=cls.parent,
            command_line_reader=cls.command_line_reader,
        )

    def setUp(self):
        self.modus.reset_mock()
        self.view.reset_mock()
        self.parameter_abo.reset_mock()

    def test___init__(self):
        self.presenter = DNSReductionGUIPresenter(
            name="reduction_gui", view=self.view, parameter_abo=self.parameter_abo, modus=self.modus, parent=self.parent
        )
        self.assertIsInstance(self.presenter, DNSReductionGUIPresenter)
        self.assertIsInstance(self.presenter, object)
        self.view.clear_subviews.assert_called_once()
        self.view.add_subview.assert_called_once()
        self.view.add_submenu.assert_called_once()
        self.parameter_abo.register.assert_called_once()

    def test__load_xml(self):
        self.presenter._load_xml()
        self.parameter_abo.xml_load.assert_called_once()

    def test__save_as(self):
        self.presenter._save_as()
        self.parameter_abo.xml_save_as.assert_called_once()

    def test__save(self):
        self.presenter._save()
        self.parameter_abo.xml_save.assert_called_once()

    def test__switch_mode(self):
        self.view.clear_subviews.reset_mock()
        self.presenter._switch_mode("powder_tof")
        self.view.clear_subviews.assert_called_once()
        self.view.clear_submenus.assert_called_once()
        self.modus.change.assert_called_once()
        self.parameter_abo.register.assert_called_once()
        self.view.add_subview.assert_called_once()
        self.view.add_submenu.assert_called_once()
        self.parameter_abo.notify_modus_change.assert_called_once()

    def test__tab_changed(self):
        self.view.get_view_for_tab_index.return_value = self.widget.view
        self.presenter._tab_changed(1, 2)
        self.assertEqual(self.view.get_view_for_tab_index.call_count, 2)
        self.parameter_abo.update_from_observer.assert_called_once()
        self.parameter_abo.notify_focused_tab.assert_called_once()


if __name__ == "__main__":
    unittest.main()
