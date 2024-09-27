# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.api import FileFinder
from sans_core.algorithm_detail.crop_helper import get_component_name
from sans_core.common.constants import EMPTY_NAME
from sans_core.common.enums import DetectorType
from sans_core.common.general_functions import create_unmanaged_algorithm


class CropHelperTest(unittest.TestCase):
    def _get_workspace(self, file_name):
        full_file_name = FileFinder.findRuns(file_name)[0]
        load_name = "Load"
        load_options = {"Filename": full_file_name, "OutputWorkspace": EMPTY_NAME}
        load_alg = create_unmanaged_algorithm(load_name, **load_options)
        load_alg.execute()
        return load_alg.getProperty("OutputWorkspace").value

    def test_that_can_get_component_name_for_sans2d(self):
        workspace = self._get_workspace("SANS2D00022024")
        self.assertEqual("front-detector", get_component_name(workspace, DetectorType.HAB))
        self.assertEqual("rear-detector", get_component_name(workspace, DetectorType.LAB))

    def test_that_can_get_component_name_for_loq(self):
        workspace = self._get_workspace("LOQ48127")
        self.assertEqual("HAB", get_component_name(workspace, DetectorType.HAB))
        self.assertEqual("main-detector-bank", get_component_name(workspace, DetectorType.LAB))

    def test_that_can_get_component_name_for_larmor(self):
        workspace = self._get_workspace("LARMOR00002260")
        self.assertEqual("DetectorBench", get_component_name(workspace, DetectorType.HAB))

    def test_that_can_get_component_name_for_zoom(self):
        # TODO when test data is available
        pass


if __name__ == "__main__":
    unittest.main()
