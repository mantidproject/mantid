# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
DNS script generator for TOF powder data.
"""

import unittest
from unittest import mock

# yapf: disable
from mantidqtinterfaces.dns_powder_tof.script_generator.\
    common_script_generator_presenter import DNSScriptGeneratorPresenter
from mantidqtinterfaces.dns_powder_tof.script_generator.\
    common_script_generator_view import DNSScriptGeneratorView
from mantidqtinterfaces.dns_powder_tof.script_generator.\
    tof_powder_script_generator_model import DNSTofPowderScriptGeneratorModel
from mantidqtinterfaces.dns_powder_tof.script_generator.\
    tof_powder_script_generator_presenter import \
    DNSTofPowderScriptGeneratorPresenter


class DNSTofPowderScriptGenerator_presenterTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.parent = mock.Mock()
        cls.view = mock.create_autospec(DNSScriptGeneratorView, instance=True)
        cls.model = mock.create_autospec(DNSTofPowderScriptGeneratorModel, instance=True)
        cls.view.sig_progress_canceled.connect = mock.Mock()
        cls.view.sig_generate_script.connect = mock.Mock()
        cls.presenter = DNSTofPowderScriptGeneratorPresenter(
            view=cls.view,
            model=cls.model,
            name='tof_powder_script_generator',
            parent=cls.parent)

    def test___init__(self):
        self.assertIsInstance(self.presenter, DNSScriptGeneratorPresenter)
        self.assertIsInstance(self.presenter,
                              DNSTofPowderScriptGeneratorPresenter)


if __name__ == '__main__':
    unittest.main()
