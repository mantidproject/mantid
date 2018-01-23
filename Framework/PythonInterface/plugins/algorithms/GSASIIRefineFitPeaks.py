from __future__ import (absolute_import, division, print_function)
from contextlib import contextmanager
import os
import sys
import tempfile

from mantid.kernel import *
from mantid.api import *
import mantid.simpleapi as mantid


class GSASIIRefineFitPeaks(PythonAlgorithm):
    """
    Mantid algorithm to use the powder diffraction and related data
    from the powder diffraction module of GSAS-II
    (https://subversion.xray.aps.anl.gov/trac/pyGSAS)
    """

    PROP_GROUP_PAWLEY_PARAMS = "Pawley Parameters"
    PROP_GSAS_PROJ_PATH = "SaveGSASIIProjectFile"
    PROP_INPUT_WORKSPACE = "InputWorkspace"
    PROP_OUT_GOF = "GOF"
    PROP_OUT_GROUP_RESULTS = "Results"
    PROP_OUT_LATTICE_PARAMS = "LatticeParameters"
    PROP_OUT_RWP = "Rwp"
    PROP_PATH_TO_GSASII = "PathToGSASII"
    PROP_PATH_TO_INST_PARAMS = "InstrumentFile"
    PROP_PATH_TO_PHASE = "PhaseInfoFile"
    PROP_PAWLEY_DMIN = "PawleyDMin"
    PROP_PAWLEY_NEGATIVE_WEIGHT = "PawleyNegativeWeight"
    PROP_REFINEMENT_METHOD = "RefinementMethod"
    PROP_SUPPRESS_GSAS_OUTPUT = "MuteGSASII"
    PROP_WORKSPACE_INDEX = "WorkspaceIndex"

    DEFAULT_REFINEMENT_PARAMS = {"set":
                                 {"Background": {"no.coeffs": 3,
                                                 "refine": True},
                                  "Sample Parameters": ["Scale"]}}
    LATTICE_TABLE_PARAMS = ["length_a", "length_b", "length_c", "angle_alpha", "angle_beta", "angle_gamma", "volume"]
    REFINEMENT_METHODS = ["Pawley refinement", "Rietveld refinement", "Peak fitting"]

    def category(self):
        return "Diffraction\\Engineering;Diffraction\\Fitting"

    def name(self):
        return "GSASIIRefineFitPeaks"

    def summary(self):
        return ("Perform Rietveld or Pawley refinement of lattice parameters on a diffraction spectrum "
                "using GSAS-II scriptable API")

    def PyInit(self):
        self.declareProperty(name=self.PROP_REFINEMENT_METHOD, defaultValue=self.REFINEMENT_METHODS[0],
                             validator=StringListValidator(self.REFINEMENT_METHODS), direction=Direction.Input,
                             doc="Refinement method (Rietvield or Pawley)")

        self.declareProperty(WorkspaceProperty(name=self.PROP_INPUT_WORKSPACE, defaultValue="",
                                               direction=Direction.Input), doc="Workspace with spectra to fit peaks")
        self.declareProperty(name=self.PROP_WORKSPACE_INDEX, defaultValue=0, direction=Direction.Input,
                             doc="Index of the spectrum in InputWorkspace to fit. By default, the first spectrum "
                                 "(ie the only one for a focused workspace) is used")
        self.declareProperty(FileProperty(name=self.PROP_PATH_TO_INST_PARAMS, defaultValue="", action=FileAction.Load,
                                          extensions=[".prm"]), doc="Location of the phase file")
        self.declareProperty(FileProperty(name=self.PROP_PATH_TO_PHASE, defaultValue="", action=FileAction.Load,
                                          extensions=[".cif"]), doc="Location of the phase file")
        self.declareProperty(FileProperty(name=self.PROP_PATH_TO_GSASII, defaultValue="", action=FileAction.Directory),
                             doc="Path to the directory containing GSASII executable on the user's machine")

        self.declareProperty(name=self.PROP_OUT_GOF, defaultValue=0.0, direction=Direction.Output,
                             doc="Goodness of fit value (Chi squared)")
        self.declareProperty(name=self.PROP_OUT_RWP, defaultValue=0.0, direction=Direction.Output,
                             doc="Weight profile R-factor (Rwp) discrepancy index for the goodness of fit")
        self.declareProperty(ITableWorkspaceProperty(name=self.PROP_OUT_LATTICE_PARAMS, direction=Direction.Output,
                                                     defaultValue=self.PROP_OUT_LATTICE_PARAMS),
                             doc="Table to output the lattice parameters (refined)")
        self.declareProperty(FileProperty(name=self.PROP_GSAS_PROJ_PATH, defaultValue="", action=FileAction.Save,
                                          extensions=".gpx"), doc="GSASII Project to work on")

        self.setPropertyGroup(self.PROP_OUT_GOF, self.PROP_OUT_GROUP_RESULTS)
        self.setPropertyGroup(self.PROP_OUT_RWP, self.PROP_OUT_GROUP_RESULTS)
        self.setPropertyGroup(self.PROP_OUT_LATTICE_PARAMS, self.PROP_OUT_GROUP_RESULTS)
        self.setPropertyGroup(self.PROP_GSAS_PROJ_PATH, self.PROP_OUT_GROUP_RESULTS)

        self.declareProperty(name=self.PROP_PAWLEY_DMIN, defaultValue=1.0, direction=Direction.Input,
                             doc="For Pawley refiment: as defined in GSAS-II, the minimum d-spacing to be used in a "
                                 "Pawley refinement. Please refer to the GSAS-II documentation for full details.")
        self.declareProperty(name=self.PROP_PAWLEY_NEGATIVE_WEIGHT, defaultValue=0.0, direction=Direction.Input,
                             doc="For Pawley refinement: as defined in GSAS-II, the weight for a penalty function "
                                 "applied during a Pawley refinement on resulting negative intensities. "
                                 "Please refer to the GSAS-II documentation for full details.")

        self.setPropertyGroup(self.PROP_PAWLEY_DMIN, self.PROP_GROUP_PAWLEY_PARAMS)
        self.setPropertyGroup(self.PROP_PAWLEY_NEGATIVE_WEIGHT, self.PROP_GROUP_PAWLEY_PARAMS)

        self.declareProperty(name=self.PROP_SUPPRESS_GSAS_OUTPUT, defaultValue=False, direction=Direction.Input,
                             doc="Set to True to prevent GSAS run info from being "
                                 "printed (not recommended, but can be useful for debugging)")

    def PyExec(self):
        refinement_method = self.getPropertyValue(self.PROP_REFINEMENT_METHOD)
        if refinement_method == self.REFINEMENT_METHODS[2]:  # Peak fitting
            raise NotImplementedError("GSAS-II Peak fitting not yet implemented in Mantid")

        with self._suppress_stdout():
            gsas_proj = self._initialise_GSAS()

            rwp, gof, lattice_params = \
                self._run_rietveld_pawley_refinement(gsas_proj=gsas_proj,
                                                     do_pawley=refinement_method == self.REFINEMENT_METHODS[0])
            self._set_output_properties(rwp=rwp, gof=gof, lattice_params=lattice_params)

    def _build_output_lattice_table(self, lattice_params):
        alg = self.createChildAlgorithm('CreateEmptyTableWorkspace')
        alg.execute()
        table = alg.getProperty('OutputWorkspace').value

        for param in self.LATTICE_TABLE_PARAMS:
            table.addColumn("double", param.split("_")[-1])

        table.addRow([float(lattice_params[param]) for param in self.LATTICE_TABLE_PARAMS])
        return table

    def _extract_spectrum_from_workspace(self):
        """
        Extract a single spectrum from the input workspace. If the input workspace only has one spectrum then just
        return the input workspace
        :return: Single-spectrum workspace
        """
        ws = self.getPropertyValue(self.PROP_INPUT_WORKSPACE)
        if mtd[ws].getNumberHistograms > 1:
            ws_index = self.getPropertyValue(self.PROP_WORKSPACE_INDEX)
            spectrum = mantid.ExtractSpectra(InputWorkspace=ws, StartWorkspaceIndex=ws_index, EndWorkspaceIndex=ws_index)
            mantid.DeleteWorkspace(Workspace=ws)
            return spectrum
        else:
            return ws

    def _initialise_GSAS(self):
        """
        Initialise a GSAS project object with a spectrum and an instrument parameter file
        :return: GSAS project object
        """
        gsas_path = self.getPropertyValue(self.PROP_PATH_TO_GSASII)
        sys.path.append(gsas_path)
        try:
            import GSASIIscriptable as GSASII
        except ImportError:
            error_msg = "Could not import GSAS-II. Are you sure it's installed at {}?".format(gsas_path)
            logger.error(error_msg)
            raise ImportError(error_msg)

        gsas_proj_path = self.getPropertyValue(self.PROP_GSAS_PROJ_PATH)
        gsas_proj = GSASII.G2Project(filename=gsas_proj_path)

        spectrum = self._extract_spectrum_from_workspace()
        spectrum_path = self._save_temporary_fxye(spectrum=spectrum)
        mantid.DeleteWorkspace(Workspace=spectrum)

        inst_param_path = self.getPropertyValue(self.PROP_PATH_TO_INST_PARAMS)
        gsas_proj.add_powder_histogram(datafile=spectrum_path, iparams=inst_param_path)

        self._remove_temporary_fxye(spectrum_path=spectrum_path)

        return gsas_proj

    def _remove_temporary_fxye(self, spectrum_path):
        try:
            os.remove(spectrum_path)
        except Exception as e:
            raise Warning("Couldn't remove temporary spectrum file at location \"{}\":\n{}".format(spectrum_path, e))

    def _run_rietveld_pawley_refinement(self, gsas_proj, do_pawley):
        """
        Run a Rietveld or Pawley refinement
        :param gsas_proj: The project to work on
        :param do_pawley: True if doing a Pawley refinement (the default), False if doing a Rietveld refinement
        :return: (R weighted profile, goodness-of-fit coefficient, table containing refined lattice parameters)
        """
        phase_path = self.getPropertyValue(self.PROP_PATH_TO_PHASE)
        phase = gsas_proj.add_phase(phasefile=phase_path, histograms=[gsas_proj.histograms()[0]])

        if do_pawley:
            self._set_pawley_phase_parameters(phase)

        gsas_proj.set_refinement(refinement=self.DEFAULT_REFINEMENT_PARAMS)
        gsas_proj.do_refinements([{}])

        residuals = gsas_proj.values()[2]["data"]["Rvals"]
        lattice_params = gsas_proj.phases()[0].get_cell()
        lattice_param_table = self._build_output_lattice_table(lattice_params)

        return residuals["Rwp"], residuals["GOF"], lattice_param_table

    def _save_temporary_fxye(self, spectrum):
        """
        Create a temporary fxye file for GSAS to read the spectrum from. This is required as we cannot pass a workspace
        straight to GSASIIscriptable, but rather it must be read from a file
        :param spectrum: The spectrum to save
        :return: Fully qualified path to the new file
        """
        workspace_index = self.getPropertyValue(self.PROP_WORKSPACE_INDEX)
        temp_dir = tempfile.gettempdir()
        # Output file MUST end with "-n.fxye" where n is a number
        # If you see "Runtime error: Rvals" from GSASIIscriptable.py, it may be because this name is badly formatted
        file_path = os.path.join(temp_dir, "{}_focused_spectrum-{}.fxye".format(self.name(), workspace_index))
        mantid.SaveFocusedXYE(Filename=file_path, InputWorkspace=spectrum, SplitFiles=False)
        return file_path

    def _set_output_properties(self, rwp, gof, lattice_params):
        self.setProperty(self.PROP_OUT_RWP, rwp)
        self.setProperty(self.PROP_OUT_GOF, gof)
        self.setProperty(self.PROP_OUT_LATTICE_PARAMS, lattice_params)

    def _set_pawley_phase_parameters(self, phase):
        # Note from GSAS-II doc: "you probably should clear the Histogram scale factor refinement
        # flag (found in Sample parameters for the powder data set) as it cannot be refined
        # simultaneously with the Pawley reflection intensities"
        phase.values()[2].values()[0]["Scale"] = [1.0, False]

        phase_params = phase.values()[4]
        phase_params["doPawley"] = True

        pawley_dmin = self.getPropertyValue(self.PROP_PAWLEY_DMIN)
        phase_params["Pawley dmin"] = pawley_dmin

        pawley_neg_wt = self.getPropertyValue(self.PROP_PAWLEY_NEGATIVE_WEIGHT)
        phase_params["Pawley neg wt"] = pawley_neg_wt

    @contextmanager
    def _suppress_stdout(self):
        """
        Suppress output from print statements. This is mainly useful for debugging, as GSAS does a lot of printing.
        """
        if self.getProperty(self.PROP_SUPPRESS_GSAS_OUTPUT).value:
            self.log().information("Suppressing stdout")
            with open(os.devnull, "w") as devnull:
                old_stdout = sys.stdout
                sys.stdout = devnull
                try:
                    yield
                finally:
                    sys.stdout = old_stdout
        else:
            yield


AlgorithmFactory.subscribe(GSASIIRefineFitPeaks)
