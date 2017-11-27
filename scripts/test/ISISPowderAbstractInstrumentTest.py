from __future__ import (absolute_import, division, print_function)

import mantid

from isis_powder.abstract_inst import AbstractInst
from isis_powder.routines.instrument_settings import InstrumentSettings
from isis_powder.routines.param_map_entry import ParamMapEntry

import os
import tempfile
import unittest
import warnings


class ISISPowderAbstractInstrumentTest(unittest.TestCase):

    _folders_to_remove = set()

    def _create_temp_dir(self):
        temp_dir = tempfile.mkdtemp()
        self._folders_to_remove.add(temp_dir)
        return temp_dir

    def _setup_mock_inst(self, yaml_file_path):
        calibration_dir = self._create_temp_dir()
        output_dir = self._create_temp_dir()

        calib_file_path = mantid.api.FileFinder.getFullPath(yaml_file_path)
        if not calib_file_path:
            self.fail("Could not find calibration file \"{}\"".format(yaml_file_path))

        return _MockInst(user_name="ISISPowderAbstractInstrumentTest",
                         calibration_directory=calibration_dir,
                         output_directory=output_dir)

    def tearDown(self):
        for folder in self._folders_to_remove:
            try:
                os.rmdir(folder)
            except OSError as exc:
                warnings.warn("Could not remove folder at \"{}\"\n"
                              "Error message:\n{}".format(folder, exc))

    def test_mock_inst_constructor(self):
        inst = self._setup_mock_inst("ISISPowderRunDetailsTest.yaml")


class _MockInst(AbstractInst):

    _param_map = [
        ParamMapEntry(ext_name="calibration_directory", int_name="calibration_dir"),
        ParamMapEntry(ext_name="output_directory",      int_name="output_dir"),
        ParamMapEntry(ext_name="user_name",             int_name="user_name")
    ]
    _advanced_config = {}
    INST_PREFIX = "MOCK"

    def __init__(self, **kwargs):
        self._inst_settings = InstrumentSettings(param_map=self._param_map,
                                                 adv_conf_dict=self._advanced_config,
                                                 kwargs=kwargs)

        super(_MockInst, self).__init__(user_name=self._inst_settings.user_name,
                                        calibration_dir=self._inst_settings.calibration_dir,
                                        output_dir=self._inst_settings.output_dir,
                                        inst_prefix=self.INST_PREFIX)