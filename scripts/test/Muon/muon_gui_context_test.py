# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import sys
import unittest
from Muon.GUI.Common.muon_gui_context import MuonGuiContext
from Muon.GUI.Common.observer_pattern import Observer

if sys.version_info.major < 2:
    from unittest import mock
else:
    import mock

class MuonGUIContextTest(unittest.TestCase):
    def test_can_be_created(self):
        self.assertTrue(MuonGuiContext())

    def test_can_set_variables(self):
        context = MuonGuiContext()

        context['FirstGoodData'] = 12.0

        self.assertEqual(context['FirstGoodData'], 12.0)

    def test_that_signal_is_sent_when_item_updates(self):
        context = MuonGuiContext()
        observer = Observer()
        observer.update = mock.MagicMock()

        context.add_subscriber(observer)

        context['FirstGoodData'] = 12.0

        observer.update.assert_called_once_with(context.notifier, None, FirstGoodData=12.0)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)