from __future__ import (absolute_import, division, print_function)
import os
import re
import mantid
import site
import stresstesting
import tempfile
from mantid.simpleapi import GSASIIRefineFitPeaks, Load


class _GSASIIRefineFitPeaksTestHelper(object):

    _LATTICE_PARAM_TBL_NAME = "LatticeParameters"
    _INPUT_WORKSPACE_FILENAME = "focused_bank1_ENGINX00256663.nxs"
    _PHASE_FILENAME = "FE_ALPHA.cif"
    _INST_PARAM_FILENAME = "template_ENGINX_241391_236516_North_bank.prm"
    _TEMP_DIR = tempfile.gettempdir()
    _path_to_gsas = None

    def path_to_gsas(self):
        if self._path_to_gsas is None:
            gsas_location = os.path.join(site.USER_SITE, "g2conda", "GSASII")
            if os.path.isdir(gsas_location):
                self._path_to_gsas = os.path.join(site.USER_SITE, "g2conda", "GSASII")
            else:
                self._path_to_gsas = ""

        return self._path_to_gsas

    def input_ws_path(self):
        return mantid.FileFinder.getFullPath(self._INPUT_WORKSPACE_FILENAME)

    def phase_file_path(self):
        return mantid.FileFinder.getFullPath(self._PHASE_FILENAME)

    def inst_param_file_path(self):
        return mantid.FileFinder.getFullPath(self._INST_PARAM_FILENAME)

    def remove_all_gsas_files(self, gsas_filename_without_extension):
        for filename in os.listdir(self._TEMP_DIR):
            if re.search(gsas_filename_without_extension, filename):
                os.remove(os.path.join(self._TEMP_DIR, filename))


class GSASIIRefineFitPeaksRietveldTest(stresstesting.MantidStressTest, _GSASIIRefineFitPeaksTestHelper):

    rwp = None
    gof = None
    gsas_proj_path = None
    _REFERENCE_FILE_NAME = "GSASIIRefineFitPeaksRietveldFitParams.nxs"
    _GSAS_PROJ_FILE_NAME = "GSASIIRefineFitPeaksRietveldTest.gpx"

    def excludeInPullRequests(self):
        return True

    def runTest(self):
        self.gsas_proj_path = os.path.join(self._TEMP_DIR, self._GSAS_PROJ_FILE_NAME)
        input_ws = Load(Filename=self.input_ws_path())

        self.gof, self.rwp, _ = GSASIIRefineFitPeaks(RefinementMethod="Rietveld refinement",
                                                     InputWorkspace=input_ws,
                                                     PhaseInfoFile=self.phase_file_path(),
                                                     InstrumentFile=self.inst_param_file_path(),
                                                     PathToGSASII=self.path_to_gsas(),
                                                     SaveGSASIIProjectFile=self.gsas_proj_path,
                                                     MuteGSASII=True,
                                                     LatticeParameters=self._LATTICE_PARAM_TBL_NAME)

    def skipTests(self):
        return not self.path_to_gsas()

    def validate(self):
        self.assertAlmostEqual(self.gof, 3.57776, delta=1e-6)
        self.assertAlmostEqual(self.rwp, 77.754994, delta=1e-6)
        return self._LATTICE_PARAM_TBL_NAME, mantid.FileFinder.getFullPath(self._REFERENCE_FILE_NAME)

    def cleanup(self):
        mantid.mtd.clear()
        self.remove_all_gsas_files(gsas_filename_without_extension=self._GSAS_PROJ_FILE_NAME.split(".")[0])


class GSASIIRefineFitPeaksPawleyTest(stresstesting.MantidStressTest, _GSASIIRefineFitPeaksTestHelper):

    rwp = None
    gof = None
    gsas_proj_path = None
    _REFERENCE_FILE_NAME = "GSASIIRefineFitPeaksPawleyFitParams.nxs"
    _GSAS_PROJ_FILE_NAME = "GSASIIRefineFitPeaksPawleyTest.gpx"

    def excludeInPullRequests(self):
        return True

    def runTest(self):
        self.gsas_proj_path = os.path.join(self._TEMP_DIR, self._GSAS_PROJ_FILE_NAME)
        input_ws = Load(Filename=self.input_ws_path())

        self.gof, self.rwp, _ = GSASIIRefineFitPeaks(RefinementMethod="Pawley refinement",
                                                     InputWorkspace=input_ws,
                                                     PhaseInfoFile=self.phase_file_path(),
                                                     InstrumentFile=self.inst_param_file_path(),
                                                     PathToGSASII=self.path_to_gsas(),
                                                     SaveGSASIIProjectFile=self.gsas_proj_path,
                                                     MuteGSASII=True,
                                                     LatticeParameters=self._LATTICE_PARAM_TBL_NAME)

    def skipTests(self):
        return not self.path_to_gsas()

    def validate(self):
        self.assertAlmostEquals(self.gof, 3.57847, delta=1e-6)
        self.assertAlmostEquals(self.rwp, 77.755147, delta=1e-6)
        return self._LATTICE_PARAM_TBL_NAME, mantid.FileFinder.getFullPath(self._REFERENCE_FILE_NAME)

    def cleanup(self):
        mantid.mtd.clear()
        self.remove_all_gsas_files(gsas_filename_without_extension=self._GSAS_PROJ_FILE_NAME.split(".")[0])
