# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from abc import ABCMeta, abstractmethod
import os
import re
import mantid
import site
import systemtesting
import tempfile
from mantid.simpleapi import GSASIIRefineFitPeaks, Load


class _AbstractGSASIIRefineFitPeaksTest(systemtesting.MantidSystemTest):
    __metaclass__ = ABCMeta

    fitted_peaks_ws = None
    gamma = None
    input_ws = None
    rwp = None
    sigma = None
    lattice_params_table = None

    _FITTED_PEAKS_WS_NAME = "FittedPeaks"
    _LATTICE_PARAM_TBL_NAME = "LatticeParameters"
    _INPUT_WORKSPACE_FILENAME = "ENGINX_280625_focused_bank_1.nxs"
    _PHASE_FILENAME_1 = "Fe-gamma.cif"
    _PHASE_FILENAME_2 = "Fe-alpha.cif"
    _INST_PARAM_FILENAME = "template_ENGINX_241391_236516_North_bank.prm"
    _TEMP_DIR = tempfile.gettempdir()
    _path_to_gsas = None

    @abstractmethod
    def _get_expected_rwp(self):
        pass

    @abstractmethod
    def _get_fit_params_reference_filename(self):
        pass

    @abstractmethod
    def _get_fitted_peaks_reference_filename(self):
        pass

    @abstractmethod
    def _get_gsas_proj_filename(self):
        pass

    @abstractmethod
    def _get_refinement_method(self):
        pass

    def cleanup(self):
        mantid.mtd.clear()
        self.remove_all_gsas_files(gsas_filename_without_extension=self._get_gsas_proj_filename().split(".")[0])

    def excludeInPullRequests(self):
        return True

    def input_ws_path(self):
        return mantid.FileFinder.getFullPath(self._INPUT_WORKSPACE_FILENAME)

    def inst_param_file_path(self):
        return mantid.FileFinder.getFullPath(self._INST_PARAM_FILENAME)

    def path_to_gsas(self):
        if self._path_to_gsas is None:
            gsas_location = os.path.join(site.USER_SITE, "g2conda", "GSASII")
            if os.path.isdir(gsas_location):
                self._path_to_gsas = os.path.join(site.USER_SITE, "g2conda", "GSASII")
            else:
                self._path_to_gsas = ""

        return self._path_to_gsas

    def phase_file_paths(self):
        return mantid.FileFinder.getFullPath(self._PHASE_FILENAME_1) + "," + mantid.FileFinder.getFullPath(self._PHASE_FILENAME_2)

    def remove_all_gsas_files(self, gsas_filename_without_extension):
        for filename in os.listdir(self._TEMP_DIR):
            if re.search(gsas_filename_without_extension, filename):
                os.remove(os.path.join(self._TEMP_DIR, filename))

    def runTest(self):
        self.input_ws = Load(Filename=self.input_ws_path(), OutputWorkspace="input_ws")

        gsas_path = self.path_to_gsas()
        if not gsas_path:
            self.fail("Could not find GSAS-II installation")

        self.fitted_peaks_ws, self.lattice_params_table, self.rwp, self.sigma, self.gamma = GSASIIRefineFitPeaks(
            RefinementMethod=self._get_refinement_method(),
            OutputWorkspace=self._FITTED_PEAKS_WS_NAME,
            InputWorkspace=self.input_ws,
            PhaseInfoFiles=self.phase_file_paths(),
            InstrumentFile=self.inst_param_file_path(),
            PathToGSASII=gsas_path,
            SaveGSASIIProjectFile=self._get_gsas_proj_filename(),
            MuteGSASII=True,
            XMin=10000,
            XMax=40000,
            LatticeParameters=self._LATTICE_PARAM_TBL_NAME,
            RefineSigma=True,
            RefineGamma=True,
        )

    def skipTests(self):
        # Skip this test, as it's just a wrapper for the Rietveld and Pawley tests
        return True

    def validate(self):
        self.tolerance = 1e-4
        self.assertAlmostEqual(self.rwp, self._get_expected_rwp(), delta=1e-5)
        return (
            self._LATTICE_PARAM_TBL_NAME,
            mantid.FileFinder.getFullPath(self._get_fit_params_reference_filename()),
            self._FITTED_PEAKS_WS_NAME,
            mantid.FileFinder.getFullPath(self._get_fitted_peaks_reference_filename()),
        )


class GSASIIRefineFitPeaksRietveldTest(_AbstractGSASIIRefineFitPeaksTest):
    def skipTests(self):
        return not self.path_to_gsas()

    def _get_expected_rwp(self):
        return 39.09515

    def _get_fit_params_reference_filename(self):
        return "GSASIIRefineFitPeaksRietveldFitParams.nxs"

    def _get_fitted_peaks_reference_filename(self):
        return "GSASIIRefineFitPeaksRietveldFittedPeaks.nxs"

    def _get_gsas_proj_filename(self):
        return "GSASIIRefineFitPeaksRietveldTest.gpx"

    def _get_refinement_method(self):
        return "Rietveld refinement"


class GSASIIRefineFitPeaksPawleyTest(_AbstractGSASIIRefineFitPeaksTest):
    def skipTests(self):
        return not self.path_to_gsas()

    def _get_expected_rwp(self):
        return 35.02589

    def _get_fit_params_reference_filename(self):
        return "GSASIIRefineFitPeaksPawleyFitParams.nxs"

    def _get_fitted_peaks_reference_filename(self):
        return "GSASIIRefineFitPeaksPawleyFittedPeaks.nxs"

    def _get_gsas_proj_filename(self):
        return "GSASIIRefineFitPeaksPawleyTest.gpx"

    def _get_refinement_method(self):
        return "Pawley refinement"
