from __future__ import (absolute_import, division, print_function)
from abc import ABCMeta, abstractmethod
import os
import re
import mantid
import stresstesting
import tempfile
from mantid.simpleapi import GSASIIRefineFitPeaks, Load


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


class _AbstractGSASIIRefineFitPeaksTest(stresstesting.MantidStressTest):

    __metaclass__ = ABCMeta

    fitted_peaks_ws = None
    input_ws = None
    residuals_table = None
    lattice_params_table = None

    _LATTICE_PARAM_TBL_NAME = "LatticeParameters"
    _RESIDUALS_TBL_NAME = "Residuals"
    _INPUT_WORKSPACE_FILENAME = "focused_bank1_ENGINX00256663.nxs"
    _PHASE_FILENAME = "FE_ALPHA.cif"
    _INST_PARAM_FILENAME = "template_ENGINX_241391_236516_North_bank.prm"
    _TEMP_DIR = tempfile.gettempdir()

    @abstractmethod
    def _get_fit_params_reference_filename(self):
        pass

    @abstractmethod
    def _get_gsas_proj_filename(self):
        pass

    @abstractmethod
    def _get_refinement_method(self):
        pass

    @abstractmethod
    def _get_residuals_reference_filename(self):
        pass

    def cleanup(self):
        mantid.mtd.clear()
        self.remove_all_gsas_files(gsas_filename_without_extension=self._get_gsas_proj_filename().split(".")[0])

    def excludeInPullRequests(self):
        return True

    def runTest(self):
        gsas_proj_path = os.path.join(self._TEMP_DIR, self._get_gsas_proj_filename())
        self.input_ws = Load(Filename=self.input_ws_path(), OutputWorkspace="input_ws")

        gsas_path = self.path_to_gsas()
        if not gsas_path:
            self.fail("Could not find GSAS-II installation")

        self.fitted_peaks_ws, self.residuals_table, self.lattice_params_table = \
            GSASIIRefineFitPeaks(RefinementMethod=self._get_refinement_method(),
                                 InputWorkspace=self.input_ws,
                                 PhaseInfoFile=self.phase_file_path(),
                                 InstrumentFile=self.inst_param_file_path(),
                                 PathToGSASII=gsas_path,
                                 SaveGSASIIProjectFile=gsas_proj_path,
                                 MuteGSASII=True,
                                 LatticeParameters=self._LATTICE_PARAM_TBL_NAME,
                                 ResidualsTable=self._RESIDUALS_TBL_NAME)

    def skipTests(self):
        # Skip this test, as it's just a wrapper for the Rietveld and Pawley tests
        return True

    def validate(self):
        # TODO: Check fitted_peaks_ws has correct values once we have some data we're sure of
        self.assertEqual(self.input_ws.getNumberBins(), self.fitted_peaks_ws.getNumberBins())
        return (self._LATTICE_PARAM_TBL_NAME, mantid.FileFinder.getFullPath(self._get_fit_params_reference_filename()),
                self._RESIDUALS_TBL_NAME, mantid.FileFinder.getFullPath(self._get_residuals_reference_filename()))

    def input_ws_path(self):
        return mantid.FileFinder.getFullPath(self._INPUT_WORKSPACE_FILENAME)

    def inst_param_file_path(self):
        return mantid.FileFinder.getFullPath(self._INST_PARAM_FILENAME)

    def path_to_gsas(self):
        return _GSASFinder.GSASIIscriptable_location()

    def phase_file_path(self):
        return mantid.FileFinder.getFullPath(self._PHASE_FILENAME)

    def remove_all_gsas_files(self, gsas_filename_without_extension):
        for filename in os.listdir(self._TEMP_DIR):
            if re.search(gsas_filename_without_extension, filename):
                os.remove(os.path.join(self._TEMP_DIR, filename))


class GSASIIRefineFitPeaksRietveldTest(_AbstractGSASIIRefineFitPeaksTest):

    def skipTests(self):
        return False

    def _get_fit_params_reference_filename(self):
        return "GSASIIRefineFitPeaksRietveldFitParams.nxs"

    def _get_gsas_proj_filename(self):
        return "GSASIIRefineFitPeaksRietveldTest.gpx"

    def _get_refinement_method(self):
        return "Rietveld refinement"

    def _get_residuals_reference_filename(self):
        return "GSASIIRefineFitPeaksRietveldResiduals.nxs"


class GSASIIRefineFitPeaksPawleyTest(_AbstractGSASIIRefineFitPeaksTest):

    def skipTests(self):
        return False

    def _get_fit_params_reference_filename(self):
        return "GSASIIRefineFitPeaksPawleyFitParams.nxs"

    def _get_gsas_proj_filename(self):
        return "GSASIIRefineFitPeaksPawleyTest.gpx"

    def _get_refinement_method(self):
        return "Pawley refinement"

    def _get_residuals_reference_filename(self):
        return "GSASIIRefineFitPeaksPawleyResiduals.nxs"
