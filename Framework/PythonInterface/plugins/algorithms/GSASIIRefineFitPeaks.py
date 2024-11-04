# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from contextlib import contextmanager
import numpy
import os
import sys
import tempfile

from mantid.api import (
    AlgorithmFactory,
    FileAction,
    FileProperty,
    ITableWorkspaceProperty,
    MultipleFileProperty,
    Progress,
    PythonAlgorithm,
    WorkspaceProperty,
)
from mantid.kernel import logger, Direction, StringListValidator
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
    PROP_OUT_FITTED_PEAKS_WS = "OutputWorkspace"
    PROP_OUT_GAMMA = "Gamma"
    PROP_OUT_GROUP_RESULTS = "Results"
    PROP_OUT_LATTICE_PARAMS = "LatticeParameters"
    PROP_OUT_RWP = "Rwp"
    PROP_OUT_SIGMA = "Sigma"
    PROP_PATH_TO_GSASII = "PathToGSASII"
    PROP_PATH_TO_INST_PARAMS = "InstrumentFile"
    PROP_PATHS_TO_PHASE_FILES = "PhaseInfoFiles"
    PROP_PAWLEY_DMIN = "PawleyDMin"
    PROP_PAWLEY_NEGATIVE_WEIGHT = "PawleyNegativeWeight"
    PROP_REFINE_GAMMA = "RefineGamma"
    PROP_REFINE_SIGMA = "RefineSigma"
    PROP_REFINEMENT_METHOD = "RefinementMethod"
    PROP_SUPPRESS_GSAS_OUTPUT = "MuteGSASII"
    PROP_WORKSPACE_INDEX = "WorkspaceIndex"
    PROP_XMAX = "XMax"
    PROP_XMIN = "XMin"

    LATTICE_TABLE_PARAMS = ["length_a", "length_b", "length_c", "angle_alpha", "angle_beta", "angle_gamma", "volume"]
    REFINEMENT_METHODS = ["Pawley refinement", "Rietveld refinement"]

    def category(self):
        return "Diffraction\\Engineering;Diffraction\\Fitting"

    def seeAlso(self):
        return ["LoadGSS", "SaveGSS", "Fit", "EnggFitPeaks"]

    def name(self):
        return "GSASIIRefineFitPeaks"

    def summary(self):
        return "Perform Rietveld or Pawley refinement of lattice parameters on a diffraction spectrum " "using GSAS-II scriptable API"

    def validateInputs(self):
        errors = {}

        x_min = self.getProperty(self.PROP_XMIN)
        x_max = self.getProperty(self.PROP_XMAX)
        if not x_max.isDefault and x_max.value <= x_min.value:
            errors[self.PROP_XMAX] = "{} must be greater than {}".format(self.PROP_XMAX, self.PROP_XMIN)

        input_ws = self.getProperty(self.PROP_INPUT_WORKSPACE).value
        input_ws_d = mantid.ConvertUnits(InputWorkspace=input_ws, Target="dSpacing", StoreInADS=False)
        max_d = max(input_ws_d.readX(0))
        pawley_dmin = self.getProperty(self.PROP_PAWLEY_DMIN).value
        if pawley_dmin > max_d:
            errors[self.PROP_PAWLEY_DMIN] = "{}={} is greater than the max dSpacing value in the input workspace ({})".format(
                self.PROP_PAWLEY_DMIN, pawley_dmin, max_d
            )

        return errors

    def PyInit(self):
        self.declareProperty(
            name=self.PROP_REFINEMENT_METHOD,
            defaultValue=self.REFINEMENT_METHODS[0],
            validator=StringListValidator(self.REFINEMENT_METHODS),
            direction=Direction.Input,
            doc="Refinement method (Rietvield or Pawley)",
        )

        self.declareProperty(
            WorkspaceProperty(name=self.PROP_INPUT_WORKSPACE, defaultValue="", direction=Direction.Input),
            doc="Workspace with spectra to fit peaks",
        )
        self.declareProperty(
            name=self.PROP_WORKSPACE_INDEX,
            defaultValue=0,
            direction=Direction.Input,
            doc="Index of the spectrum in InputWorkspace to fit. By default, the first spectrum "
            "(ie the only one for a focused workspace) is used",
        )
        self.declareProperty(
            FileProperty(name=self.PROP_PATH_TO_INST_PARAMS, defaultValue="", action=FileAction.Load, extensions=[".prm"]),
            doc="Location of the phase file",
        )
        self.declareProperty(
            MultipleFileProperty(name=self.PROP_PATHS_TO_PHASE_FILES, extensions=[".cif"]), doc="Paths to each required phase file"
        )
        self.declareProperty(
            FileProperty(name=self.PROP_PATH_TO_GSASII, defaultValue="", action=FileAction.Directory),
            doc="Path to the directory containing GSASII executable on the user's machine",
        )

        self.declareProperty(
            name=self.PROP_XMIN,
            defaultValue=0.0,
            direction=Direction.Input,
            doc="Minimum x value to use for refinement, in the same units as the input workspace. "
            + "Leave blank to refine from the start of the data to {0}. Note, if {1} corresponds"
            "  to a greater TOF value than this, then {1} is used".format(self.PROP_XMAX, self.PROP_PAWLEY_DMIN),
        )
        self.declareProperty(
            name=self.PROP_XMAX,
            defaultValue=0.0,
            direction=Direction.Input,
            doc="Maximum x value to use for refinement, in the same units as the input workspace. "
            + "Leave blank to refine in the range {} to the end of the data".format(self.PROP_XMIN),
        )
        self.declareProperty(
            name=self.PROP_REFINE_SIGMA,
            defaultValue=False,
            direction=Direction.Input,
            doc="Whether to refine the sigma-1 profile coefficient",
        )
        self.declareProperty(
            name=self.PROP_REFINE_GAMMA,
            defaultValue=False,
            direction=Direction.Input,
            doc="Whether to refine the gamma-1 (called 'X' in GSAS-II) profile coefficient",
        )

        self.declareProperty(
            WorkspaceProperty(name=self.PROP_OUT_FITTED_PEAKS_WS, defaultValue="", direction=Direction.Output),
            doc="Workspace with fitted peaks",
        )
        self.declareProperty(
            ITableWorkspaceProperty(
                name=self.PROP_OUT_LATTICE_PARAMS, direction=Direction.Output, defaultValue=self.PROP_OUT_LATTICE_PARAMS
            ),
            doc="Table to output the lattice parameters (refined)",
        )
        self.declareProperty(
            name=self.PROP_OUT_RWP, direction=Direction.Output, defaultValue=0.0, doc="Weighted profile R factor (as a percentage)"
        )
        self.declareProperty(name=self.PROP_OUT_SIGMA, direction=Direction.Output, defaultValue=0.0, doc="Sigma-1 profile coefficient")
        self.declareProperty(
            name=self.PROP_OUT_GAMMA, direction=Direction.Output, defaultValue=0.0, doc="Gamma-1 profile coefficient (called X in GSAS-II)"
        )
        self.declareProperty(
            FileProperty(name=self.PROP_GSAS_PROJ_PATH, defaultValue="", action=FileAction.Save, extensions=".gpx"),
            doc="GSASII Project to work on",
        )

        self.setPropertyGroup(self.PROP_OUT_FITTED_PEAKS_WS, self.PROP_OUT_GROUP_RESULTS)
        self.setPropertyGroup(self.PROP_OUT_LATTICE_PARAMS, self.PROP_OUT_GROUP_RESULTS)
        self.setPropertyGroup(self.PROP_OUT_RWP, self.PROP_OUT_GROUP_RESULTS)
        self.setPropertyGroup(self.PROP_GSAS_PROJ_PATH, self.PROP_OUT_GROUP_RESULTS)

        self.declareProperty(
            name=self.PROP_PAWLEY_DMIN,
            defaultValue=1.0,
            direction=Direction.Input,
            doc="For Pawley refiment: as defined in GSAS-II, the minimum d-spacing to be used in a "
            "Pawley refinement. Please refer to the GSAS-II documentation for full details. Note, "
            "if this corresponds to a TOF value less than {0} or the lowest TOF value in the data,"
            " the greatest of the 3 values is used as {0}".format(self.PROP_XMIN),
        )
        self.declareProperty(
            name=self.PROP_PAWLEY_NEGATIVE_WEIGHT,
            defaultValue=0.0,
            direction=Direction.Input,
            doc="For Pawley refinement: as defined in GSAS-II, the weight for a penalty function "
            "applied during a Pawley refinement on resulting negative intensities. "
            "Please refer to the GSAS-II documentation for full details.",
        )

        self.setPropertyGroup(self.PROP_PAWLEY_DMIN, self.PROP_GROUP_PAWLEY_PARAMS)
        self.setPropertyGroup(self.PROP_PAWLEY_NEGATIVE_WEIGHT, self.PROP_GROUP_PAWLEY_PARAMS)

        self.declareProperty(
            name=self.PROP_SUPPRESS_GSAS_OUTPUT,
            defaultValue=False,
            direction=Direction.Input,
            doc="Set to True to prevent GSAS run info from being " "printed (not recommended, but can be useful for debugging)",
        )

    def PyExec(self):
        with self._suppress_stdout():
            gsas_proj = self._initialise_GSAS()

            rwp, lattice_params = self._run_rietveld_pawley_refinement(gsas_proj=gsas_proj, do_pawley=self._refinement_method_is_pawley())

            self._set_output_properties(
                lattice_params=lattice_params,
                rwp=rwp,
                fitted_peaks_ws=self._generate_fitted_peaks_ws(gsas_proj),
                gamma=gsas_proj.values()[5]["Instrument Parameters"][0]["X"][1],
                sigma=gsas_proj.values()[5]["Instrument Parameters"][0]["sig-1"][1],
            )

    def _build_output_lattice_table(self, lattice_params):
        table_name = self.getPropertyValue(self.PROP_OUT_LATTICE_PARAMS)
        table = mantid.CreateEmptyTableWorkspace(OutputWorkspace=table_name, StoreInADS=False)

        for param in self.LATTICE_TABLE_PARAMS:
            table.addColumn("double", param.split("_")[-1])

        table.addRow([float(lattice_params[param]) for param in self.LATTICE_TABLE_PARAMS])
        return table

    def _create_refinement_params_dict(self, num_phases, pawley_tmin=None):
        basic_refinement = {"set": {"Background": {"no.coeffs": 3, "refine": True}, "Sample Parameters": ["Scale"]}}

        input_ws = self.getProperty(self.PROP_INPUT_WORKSPACE).value
        x_max = self.getProperty(self.PROP_XMAX).value
        if not x_max:
            x_max = max(input_ws.readX(0))
        x_min = max(pawley_tmin, min(input_ws.readX(0)), self.getProperty(self.PROP_XMIN).value)
        self.setProperty(self.PROP_XMIN, x_min)
        self.setProperty(self.PROP_XMAX, x_max)
        basic_refinement["set"].update({"Limits": [x_min, x_max]})

        scale_refinement = {"set": {"Scale": True}, "phases": range(1, num_phases)}
        unit_cell_refinement = {"set": {"Cell": True}}

        profile_coeffs_refinement = {"set": {"Instrument Parameters": []}}

        refine_sigma = self.getProperty(self.PROP_REFINE_SIGMA).value
        if refine_sigma:
            profile_coeffs_refinement["set"]["Instrument Parameters"].append("sig-1")

        refine_gamma = self.getProperty(self.PROP_REFINE_GAMMA).value
        if refine_gamma:
            profile_coeffs_refinement["set"]["Instrument Parameters"].append("X")

        return [basic_refinement, scale_refinement, unit_cell_refinement, profile_coeffs_refinement, {}]

    def _refinement_method_is_pawley(self):
        return self.getPropertyValue(self.PROP_REFINEMENT_METHOD) == self.REFINEMENT_METHODS[0]

    def _extract_spectrum_from_workspace(self):
        """
        Extract a single spectrum from the input workspace. If the input workspace only has one spectrum then just
        return the input workspace
        :return: Single-spectrum workspace
        """
        ws = self.getProperty(self.PROP_INPUT_WORKSPACE).value
        if ws.getNumberHistograms > 1:
            ws_index = self.getPropertyValue(self.PROP_WORKSPACE_INDEX)
            spectrum = mantid.ExtractSpectra(InputWorkspace=ws, StartWorkspaceIndex=ws_index, EndWorkspaceIndex=ws_index, StoreInADS=False)
        else:
            spectrum = mantid.CloneWorkspace(InputWorkspace=ws, StoreInADS=False)

        return spectrum

    def _generate_fitted_peaks_ws(self, gsas_proj):
        input_ws = self.getProperty(self.PROP_INPUT_WORKSPACE).value
        fitted_peaks_ws_name = self.getPropertyValue(self.PROP_OUT_FITTED_PEAKS_WS)
        fitted_peaks_ws = mantid.CloneWorkspace(InputWorkspace=input_ws, OutputWorkspace=fitted_peaks_ws_name, StoreInADS=False)

        hist = gsas_proj.histogram(0)
        fitted_peaks_y = hist.getdata(datatype="yCalc")
        fitted_peaks_y_unmasked = self._replace_masked_elements_with_default(masked_array=fitted_peaks_y, default=0)
        fitted_peaks_ws.setY(0, fitted_peaks_y_unmasked)
        return fitted_peaks_ws

    def _generate_pawley_reflections(self, phase):
        # Note: this is pretty much just copied over from GSASIIphsGUI.UpdatePhaseData.OnPawleyLoad
        # Once it is possible to do this from GSASIIscriptable, this method should be replaced
        phase_data = phase.data["General"]  # Parameters corresponding to the 'General' tab in the GSASII GUI
        cell = phase_data["Cell"][1:7]
        A = GSASIIlattice.cell2A(cell)
        space_group = phase_data["SGData"]
        d_min = phase_data["Pawley dmin"]

        reflections = numpy.array(GSASIIlattice.GenHLaue(d_min, space_group, A))

        peaks = []
        for h, k, l, d in reflections:
            forbidden_by_symmetry, multiplicity = GSASIIspc.GenHKLf([h, k, l], space_group)[:2]
            if not forbidden_by_symmetry:
                multiplicity *= 2
                peaks.append([h, k, l, multiplicity, d, True, 100.0, 1.0])
        GSASIImath.sortArray(peaks, 4, reverse=True)
        return peaks

    def _initialise_GSAS(self):
        """
        Initialise a GSAS project object with a spectrum and an instrument parameter file
        :return: GSAS project object
        """
        gsas_path = self.getPropertyValue(self.PROP_PATH_TO_GSASII)
        sys.path.append(gsas_path)
        try:
            global GSASII
            global GSASIIlattice
            global GSASIIspc
            global GSASIImath
            import GSASIIscriptable as GSASII
            import GSASIIlattice
            import GSASIIspc
            import GSASIImath
        except ImportError:
            error_msg = "Could not import GSAS-II. Are you sure it's installed at {}?".format(gsas_path)
            logger.error(error_msg)
            raise ImportError(error_msg)

        gsas_proj_path = self.getPropertyValue(self.PROP_GSAS_PROJ_PATH)
        gsas_proj = GSASII.G2Project(filename=gsas_proj_path)

        spectrum = self._extract_spectrum_from_workspace()
        spectrum_path = self._save_temporary_fxye(spectrum=spectrum)

        inst_param_path = self.getPropertyValue(self.PROP_PATH_TO_INST_PARAMS)
        gsas_proj.add_powder_histogram(datafile=spectrum_path, iparams=inst_param_path, fmthint="xye")

        self._remove_temporary_fxye(spectrum_path=spectrum_path)

        return gsas_proj

    def _replace_masked_elements_with_default(self, masked_array, default):
        return numpy.array([val if not masked else default for val, masked in zip(masked_array, masked_array.mask)])

    def _remove_temporary_fxye(self, spectrum_path):
        try:
            os.remove(spectrum_path)
        except Exception as e:
            raise Warning('Couldn\'t remove temporary spectrum file at location "{}":\n{}'.format(spectrum_path, e))

    def _run_rietveld_pawley_refinement(self, gsas_proj, do_pawley):
        """
        Run a Rietveld or Pawley refinement
        :param gsas_proj: The project to work on
        :param do_pawley: True if doing a Pawley refinement (the default), False if doing a Rietveld refinement
        :return: (R weighted profile, goodness-of-fit coefficient, table containing refined lattice parameters)
        """
        phase_paths = self.getPropertyValue(self.PROP_PATHS_TO_PHASE_FILES).split(",")
        pawley_tmin = None
        if self._refinement_method_is_pawley():
            pawley_dmin = float(self.getPropertyValue(self.PROP_PAWLEY_DMIN))
            pawley_tmin = GSASIIlattice.Dsp2pos(Inst=gsas_proj.histogram(0).data["Instrument Parameters"][0], dsp=pawley_dmin)
        refinements = self._create_refinement_params_dict(num_phases=len(phase_paths), pawley_tmin=pawley_tmin)
        prog = Progress(self, start=0, end=1, nreports=2)

        prog.report("Reading phase files")
        for phase_path in phase_paths:
            phase = gsas_proj.add_phase(phasefile=phase_path, histograms=[gsas_proj.histograms()[0]])
            if do_pawley:
                self._set_pawley_phase_parameters(phase)
                pawley_reflections = self._generate_pawley_reflections(phase)
                phase.data["Pawley ref"] = pawley_reflections

        prog.report("Running {} refinement steps".format(len(refinements)))
        for refinement in refinements:
            gsas_proj.do_refinements([refinement])
        gsas_proj.save()

        rwp = gsas_proj.histogram(0).get_wR()
        lattice_params = gsas_proj.phases()[0].get_cell()
        lattice_params_table = self._build_output_lattice_table(lattice_params)

        return rwp, lattice_params_table

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
        mantid.SaveFocusedXYE(Filename=file_path, InputWorkspace=spectrum, SplitFiles=False, IncludeHeader=False)
        return file_path

    def _set_output_properties(self, fitted_peaks_ws, rwp, lattice_params, sigma, gamma):
        self.setProperty(self.PROP_OUT_FITTED_PEAKS_WS, fitted_peaks_ws)
        self.setProperty(self.PROP_OUT_RWP, rwp)
        self.setProperty(self.PROP_OUT_LATTICE_PARAMS, lattice_params)
        self.setProperty(self.PROP_OUT_GAMMA, gamma)
        self.setProperty(self.PROP_OUT_SIGMA, sigma)

    def _set_pawley_phase_parameters(self, phase):
        # Note from GSAS-II doc: "you probably should clear the Histogram scale factor refinement
        # flag (found in Sample parameters for the powder data set) as it cannot be refined
        # simultaneously with the Pawley reflection intensities"
        phase.values()[2].values()[0]["Scale"] = [1.0, False]

        phase_params = phase.values()[4]
        phase_params["doPawley"] = True

        pawley_dmin = self.getPropertyValue(self.PROP_PAWLEY_DMIN)
        phase_params["Pawley dmin"] = float(pawley_dmin)

        pawley_neg_wt = self.getPropertyValue(self.PROP_PAWLEY_NEGATIVE_WEIGHT)
        phase_params["Pawley neg wt"] = float(pawley_neg_wt)

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
