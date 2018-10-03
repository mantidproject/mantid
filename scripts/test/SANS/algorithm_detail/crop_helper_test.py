# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import unittest
import mantid
from sans.algorithm_detail.crop_helper import get_component_name
from sans.common.enums import DetectorType
from sans.common.constants import EMPTY_NAME
from sans.common.general_functions import create_unmanaged_algorithm
from mantid.api import FileFinder


class CropHelperTest(unittest.TestCase):
    def _get_workspace(self, file_name):
        full_file_name = FileFinder.findRuns(file_name)[0]
        load_name = "Load"
        load_options = {"Filename": full_file_name,
                        "OutputWorkspace": EMPTY_NAME}
        load_alg = create_unmanaged_algorithm(load_name, **load_options)
        load_alg.execute()
        return load_alg.getProperty("OutputWorkspace").value

    def test_that_can_get_component_name_for_sans2d(self):
        workspace = self._get_workspace("SANS2D00022024")
        self.assertTrue("front-detector" == get_component_name(workspace, DetectorType.HAB))
        self.assertTrue("rear-detector" == get_component_name(workspace, DetectorType.LAB))

    def test_that_can_get_component_name_for_loq(self):
        workspace = self._get_workspace("LOQ48127")
        self.assertTrue("HAB" == get_component_name(workspace, DetectorType.HAB))
        self.assertTrue("main-detector-bank" == get_component_name(workspace, DetectorType.LAB))

    def test_that_can_get_component_name_for_larmor(self):
        workspace = self._get_workspace("LARMOR00002260")
        self.assertTrue("DetectorBench" == get_component_name(workspace, DetectorType.HAB))

    def test_that_can_get_component_name_for_zoom(self):
        # TODO when test data is available
        pass

if __name__ == '__main__':
    unittest.main()
