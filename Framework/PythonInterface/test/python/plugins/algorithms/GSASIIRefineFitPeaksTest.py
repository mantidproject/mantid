from __future__ import (absolute_import, division, print_function)

import os
import tempfile
import unittest
import mantid.simpleapi as mantid
from mantid.api import *


class _GSASFinder(object):
    """
    Helper class for unit test - the algorithm can't run without a version of GSAS-II that includes the module
    GSASIIscriptable (added April 2017)
    """
    @staticmethod
    def _find_directory_by_name(cur_dir_name, cur_dir_path, name_to_find, level, max_level):
        """
        Perform a depth-limited depth-first search to try and find a directory with a given name
        """
        if level == max_level:
            return None

        if cur_dir_name == name_to_find:
            return cur_dir_path

        list_dir = os.listdir(cur_dir_path)
        for child in list_dir:
            child_path = os.path.join(cur_dir_path, child)
            if os.path.isdir(child_path):
                try:
                    path = _GSASFinder._find_directory_by_name(cur_dir_name=child, cur_dir_path=child_path,
                                                               level=level + 1, name_to_find=name_to_find,
                                                               max_level=max_level)
                except OSError:  # Probably "Permission denied". Either way, just ignore it
                    pass
                else:
                    if path is not None:
                        return path

    @staticmethod
    def _path_to_g2conda():
        """
        Find the g2conda directory (where GSAS-II normally sits), as long as it exists less than 2 levels away from
        the root directory
        """
        root_directory = os.path.abspath(os.sep)
        return _GSASFinder._find_directory_by_name(cur_dir_path=root_directory, cur_dir_name=root_directory, level=0,
                                                   name_to_find="g2conda", max_level=2)

    @staticmethod
    def GSASIIscriptable_location():
        """
        Find the path to GSASIIscriptable.py, if it exists and is less than 2 levels away from the root directory
        """
        path_to_g2conda = _GSASFinder._path_to_g2conda()
        if path_to_g2conda is None:
            return None

        path_to_gsasii_scriptable = os.path.join(path_to_g2conda, "GSASII")
        if os.path.isfile(os.path.join(path_to_gsasii_scriptable, "GSASIIscriptable.py")):
            return path_to_gsasii_scriptable

        return None


class GSASIIRefineFitPeaksTest(unittest.TestCase):

    _path_to_gsas = None
    _gsas_proj = None
    _input_ws = None
    _phase_file = None
    _inst_file = None

    def setUp(self):
        data_dir = mantid.config["datasearch.directories"].split(";")[0]
        print(mantid.config["datasearch.directories"].split(";"))
        
        self._phase_file = os.path.join(data_dir, "FE_ALPHA.cif")
        self._inst_file = os.path.join(data_dir, "template_ENGINX_241391_236516_North_bank.prm")

        self._path_to_gsas = _GSASFinder.GSASIIscriptable_location()
        if self._path_to_gsas is None:
            self.skipTest("Could not find GSASIIscriptable.py")

        temp_dir = tempfile.gettempdir()
        self._gsas_proj = os.path.join(temp_dir, "GSASIIRefineFitPeaksTest.gpx")

        spectrum_file = os.path.join(data_dir, "focused_bank1_ENGINX00256663.nxs")
        self._input_ws = mantid.Load(Filename=spectrum_file, OutputWorkspace="input")

    def tearDown(self):
        if os.path.isfile(self._gsas_proj):
            os.remove(self._gsas_proj)

    def test_rietveld_refinement_with_default_params(self):
        gof, rwp, lattice_table = mantid.GSASIIRefineFitPeaks(RefinementMethod="Rietveld refinement",
                                                              InputWorkspace=self._input_ws,
                                                              PhaseInfoFile=self._phase_file,
                                                              InstrumentFile=self._inst_file,
                                                              PathToGSASII=self._path_to_gsas,
                                                              SaveGSASIIProjectFile=self._gsas_proj,
                                                              MuteGSASII=True)

        self.assertAlmostEqual(gof, 3.57776, delta=1e-6)
        self.assertAlmostEquals(rwp, 77.75499, delta=1e6)
        row = lattice_table.row(0)
        self.assertAlmostEqual(row["a"], 2.8665)
        self.assertAlmostEqual(row["b"], 2.8665)
        self.assertAlmostEqual(row["c"], 2.8665)
        self.assertAlmostEqual(row["alpha"], 90)
        self.assertAlmostEqual(row["beta"], 90)
        self.assertAlmostEqual(row["gamma"], 90)
        self.assertAlmostEqual(row["volume"], 23.554, delta=1e4)

    def test_pawley_refinement_with_default_params(self):
        gof, rwp, lattice_table = mantid.GSASIIRefineFitPeaks(RefinementMethod="Pawley refinement",
                                                              InputWorkspace=self._input_ws,
                                                              PhaseInfoFile=self._phase_file,
                                                              InstrumentFile=self._inst_file,
                                                              PathToGSASII=self._path_to_gsas,
                                                              SaveGSASIIProjectFile=self._gsas_proj,
                                                              MuteGSASII=True)

        self.assertAlmostEqual(gof, 3.57847, delta=1e-6)
        self.assertAlmostEquals(rwp, 77.75515, delta=1e6)
        row = lattice_table.row(0)
        self.assertAlmostEqual(row["a"], 2.8665)
        self.assertAlmostEqual(row["b"], 2.8665)
        self.assertAlmostEqual(row["c"], 2.8665)
        self.assertAlmostEqual(row["alpha"], 90)
        self.assertAlmostEqual(row["beta"], 90)
        self.assertAlmostEqual(row["gamma"], 90)
        self.assertAlmostEqual(row["volume"], 23.554, delta=1e4)

if __name__ == '__main__':
    unittest.main()
