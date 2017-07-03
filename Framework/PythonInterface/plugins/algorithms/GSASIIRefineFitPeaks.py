from __future__ import (absolute_import, division, print_function)
from mantid.api import AlgorithmFactory, ITableWorkspaceProperty, FileAction, FileProperty, \
    MatrixWorkspaceProperty, Progress, PropertyMode, PythonAlgorithm
from mantid.kernel import Direction, FloatArrayProperty, Property, StringListValidator
import mantid.simpleapi as msapi

# Too many properties!
#pylint: disable=too-many-instance-attributes


class GSASIIRefineFitPeaks(PythonAlgorithm):
    """
    Mantid algorithm to use the powder diffraction and related data
    from the powder diffraction module of GSAS-II
    (https://subversion.xray.aps.anl.gov/trac/pyGSAS)
    """

    def category(self):
        """
        Override required for Mantid algorithms
        """
        return "Diffraction\\Engineering;Diffraction\\Fitting"

    def name(self):
        """
        Override required for Mantid algorithms
        """
        return "GSASIIRefineFitPeaks"

    def summary(self):
        """
        Override required for Mantid algorithms
        """
        return ("Uses GSAS-II (powder diffraction and structure modules) to perform whole "
                "pattern refinement of lattice parameters (or fit peaks) on an diffraction "
                "spectrum")

    def __init__(self):
        PythonAlgorithm.__init__(self)

        # For the wsPython app underlying GSAS-II
        self._gsas2_app = None

        self.PROP_INPUT_WORKSPACE = 'InputWorkspace'
        self.PROP_WORKSPACE_INDEX = 'WorkspaceIndex'
        self.PROP_INSTR_FILE = 'InstrumentFile'
        self.PROP_PHASE_INFO_FILE = 'PhaseInfoFile'
        self.PROP_PATH_TO_GSASII = 'PathToGSASII'
        self.PROP_PAWLEY_DMIN = "PawleyDmin"
        self.PROP_PAWLEY_NEG_WEIGHT = "PawleyNegativeWeight"
        self.PROP_BACKGROUND_TYPE = 'BackgroundType'
        self.PROP_MINX = 'MinX'
        self.PROP_MAXX = 'MaxX'
        self.PROP_EXPECTED_PEAKS = "ExpectedPeaks"
        self.PROP_EXPECTED_PEAKS_FROM_FILE = "ExpectedPeaksFromFile"
        self.PROP_OUT_LATTICE_PARAMS = 'LatticeParameters'
        self.PROP_OUT_FITTED_PARAMS = 'FittedPeakParameters'
        self.PROP_OUT_PROJECT_FILE = 'SaveGSASIIProjectFile'
        self.PROP_OUT_GOF = 'GoF'
        self.PROP_OUT_RWP = 'Rwp'
        self.PROP_REFINE_CENTER = 'RefineCenter'
        self.PROP_REFINE_INTENSITY = 'RefineIntensity'
        self.PROP_REFINE_ALPHA = 'RefineAlpha'
        self.PROP_REFINE_BETA = 'RefineBeta'
        self.PROP_REFINE_SIGMA = 'RefineSigma'
        self.PROP_REFINE_GAMMA = 'RefineGamma'
        self.PROP_METHOD = "Method"

    def PyInit(self):

        refine_methods = ["Pawley refinement", "Rietveld refinement", "Peak fitting"]
        self.declareProperty(self.PROP_METHOD, defaultValue = refine_methods[0],
                             validator = StringListValidator(refine_methods),
                             doc = 'Rietveld corresponds to the Calculate/Refine option of the '
                             'GSAS-II GUI. Peak fitting is single peak (does not use phase '
                             'information  and corresponds to the option '
                             'Peaks List/Peak Fitting/PeakFitType of the GSAS-II GUI. The '
                             'third alternative requires a list of peaks which can be bassed in '
                             'the properties ' + self.PROP_EXPECTED_PEAKS + ' and ' +
                             self.PROP_EXPECTED_PEAKS_FROM_FILE + '.')

        self.declareProperty(MatrixWorkspaceProperty(self.PROP_INPUT_WORKSPACE, '',
                                                     optional = PropertyMode.Mandatory,
                                                     direction = Direction.Input),
                             doc = 'Workspace with spectra to fit peaks. ToF is expected X unit.')

        self.declareProperty(self.PROP_WORKSPACE_INDEX, 0,
                             doc = 'Index of the workspace for the spectrum to fit. By default '
                             'the first spectrum will be processed (that is, the only spectrum '
                             'for focussed data workspaces.', direction = Direction.Input)

        self.declareProperty(FileProperty(name = self.PROP_INSTR_FILE, defaultValue = '',
                                          action = FileAction.Load,
                                          extensions = [".par", ".prm", ".ipar", ".iparm"]),
                             doc = 'File with instrument parameters (in GSAS format).')

        self.declareProperty(FileProperty(name = self.PROP_PHASE_INFO_FILE, defaultValue = '',
                                          action = FileAction.OptionalLoad, extensions = [".cif"]),
                             doc = 'File with phase information for the material.')

        self.declareProperty(FileProperty(name = self.PROP_PATH_TO_GSASII, defaultValue = '',
                                          action = FileAction.OptionalDirectory),
                             doc = 'Optional path to GSAS-II software installation. '
                             'This will be used to import several Python modules from GSAS-II.')

        GRP_RESULTS = "Results"

        self.declareProperty('GoF', 0.0, direction = Direction.Output,
                             doc = 'Goodness of fit value (Chi squared).')

        self.declareProperty('Rwp', 0.0, direction = Direction.Output,
                             doc = 'Weighted profile R-factor (Rwp) discrepancy index for the '
                             'goodness of fit.')

        self.declareProperty(ITableWorkspaceProperty(self.PROP_OUT_LATTICE_PARAMS, "", Direction.Output),
                             doc = 'Table to output the the lattice parameters (refined).')

        self.declareProperty(self.PROP_OUT_FITTED_PARAMS, "", direction=Direction.Input,
                             doc = "Name for an (output) table of fitted parameters. This is used "
                             "with the peak fitting method. The table will have one row per peak "
                             "found.")

        # This is mandatory. We could also make it FileAction.OptionalSave, and use a temporary
        # project file when the option is not given by the user.
        self.declareProperty(FileProperty(name = self.PROP_OUT_PROJECT_FILE, defaultValue = '',
                                          direction = Direction.Input,
                                          action = FileAction.Save, extensions = [".gpx"]),
                             doc = 'GSAS-II project file (that can be openened in the GSAS-II GUI).')

        self.setPropertyGroup(self.PROP_OUT_GOF, GRP_RESULTS)
        self.setPropertyGroup(self.PROP_OUT_RWP, GRP_RESULTS)
        self.setPropertyGroup(self.PROP_OUT_LATTICE_PARAMS, GRP_RESULTS)
        self.setPropertyGroup(self.PROP_OUT_FITTED_PARAMS, GRP_RESULTS)
        self.setPropertyGroup(self.PROP_OUT_PROJECT_FILE, GRP_RESULTS)

        GRP_FITTING_OPTS = "Fitting options"
        background_types = ["Chebyshev", "None"]
        self.declareProperty(self.PROP_BACKGROUND_TYPE, defaultValue = background_types[0],
                             validator = StringListValidator(background_types),
                             doc = 'Type of background for the peak fitting. Currently only the '
                             'default option of GSAS-II (chebyshev) is supported.')

        self.declareProperty(self.PROP_MINX, Property.EMPTY_DBL,
                             direction = Direction.Input,
                             doc = "Minimum x value for the fitting, in the same units as the input "
                             "workspace (TOF). Defines the range or domain of fitting together "
                             "with the property {0}. Leave empty to use the whole range".
                             format(self.PROP_MAXX))

        self.declareProperty(self.PROP_MAXX, Property.EMPTY_DBL,
                             direction = Direction.Input,
                             doc = "Maximum x value for the fitting, in the same units as the input "
                             "workspace (TOF). Defines the range or domain of fitting together "
                             "with the property {0}. Leave empty to use the whole range".
                             format(self.PROP_MINX))

        self.setPropertyGroup(self.PROP_BACKGROUND_TYPE, GRP_FITTING_OPTS)
        self.setPropertyGroup(self.PROP_MINX, GRP_FITTING_OPTS)
        self.setPropertyGroup(self.PROP_MAXX, GRP_FITTING_OPTS)

        GRP_PAWLEY_OPTIONS = "Pawley refinement options"

        self.declareProperty(self.PROP_PAWLEY_DMIN, 1.0, direction = Direction.Input,
                             doc = "For Pawley refiment: as defined in GSAS-II, the minimum d-spacing "
                             "to be used in a Pawley refinement. Please refer to the GSAS-II "
                             "documentation for full details.")

        self.declareProperty(self.PROP_PAWLEY_NEG_WEIGHT, 0.0, direction = Direction.Input,
                             doc = "For Pawley refinement: as defined in GSAS-II, the weight for a "
                             "penalty function applied during a Pawley refinement on resulting negative "
                             "intensities. Please refer to the GSAS-II documentation for full details.")

        self.setPropertyGroup(self.PROP_PAWLEY_DMIN, GRP_PAWLEY_OPTIONS)
        self.setPropertyGroup(self.PROP_PAWLEY_NEG_WEIGHT, GRP_PAWLEY_OPTIONS)

        GRP_PEAKS = "Expected peaks (phase information takes precedence)"

        self.declareProperty(FloatArrayProperty(self.PROP_EXPECTED_PEAKS, [],
                                                direction = Direction.Input),
                             "A list of dSpacing values for the peak centers. These will be "
                             "converted into TOF to find expected peaks.")

        self.declareProperty(FileProperty(name = self.PROP_EXPECTED_PEAKS_FROM_FILE, defaultValue = "",
                                          action = FileAction.OptionalLoad, extensions = [".csv"],
                                          direction = Direction.Input),
                             doc = "Load from this file a list of dSpacing values to be converted "
                             "into TOF . This takes precedence over '" + self.PROP_EXPECTED_PEAKS + "' "
                             "when both options are given.")

        self.setPropertyGroup(self.PROP_EXPECTED_PEAKS, GRP_PEAKS)
        self.setPropertyGroup(self.PROP_EXPECTED_PEAKS_FROM_FILE, GRP_PEAKS)

        GRP_PARAMS = "Refinement of peak parameters"

        self.declareProperty(name = self.PROP_REFINE_CENTER, defaultValue = False,
                             doc = 'Whether to refine the peak centers.')

        self.declareProperty(name = self.PROP_REFINE_INTENSITY, defaultValue = False,
                             doc = 'Whether to refine the peak function intensity parameters '
                             '(assuming a shape of type back-to-back exponential convoluted '
                             'with pseudo-voigt (BackToBackExponentialPV).')

        self.declareProperty(name = self.PROP_REFINE_ALPHA, defaultValue = False,
                             doc = 'Whether to refine the peak function beta parameters '
                             '(assuming a BackToBackExponentialPV peak shape.')

        self.declareProperty(name = self.PROP_REFINE_BETA, defaultValue = False,
                             doc = 'Whether to refine the peak function beta parameters '
                             '(assuming a BackToBackExponentialPV peak shape.')

        self.declareProperty(name = self.PROP_REFINE_SIGMA, defaultValue = True,
                             doc = 'Whether to refine the peak function sigma parameters '
                             '(assuming a BackToBackExponentialPV peak shape.')

        self.declareProperty(name = self.PROP_REFINE_GAMMA, defaultValue = True,
                             doc = 'Whether to refine the peak function gamma parameters '
                             '(assuming a BackToBackExponentialPV peak shape.')

        self.setPropertyGroup(self.PROP_REFINE_CENTER, GRP_PARAMS)
        self.setPropertyGroup(self.PROP_REFINE_INTENSITY, GRP_PARAMS)
        self.setPropertyGroup(self.PROP_REFINE_ALPHA, GRP_PARAMS)
        self.setPropertyGroup(self.PROP_REFINE_BETA, GRP_PARAMS)
        self.setPropertyGroup(self.PROP_REFINE_SIGMA, GRP_PARAMS)
        self.setPropertyGroup(self.PROP_REFINE_GAMMA, GRP_PARAMS)

    def validateInputs(self):
        errors = {}
        pfm_name = "Peak fitting"
        # This could check if the required inputs for different methods have been provided
        if pfm_name == self.getPropertyValue(self.PROP_METHOD) and\
           not self.getPropertyValue(self.PROP_OUT_FITTED_PARAMS):
            errors[self.PROP_OUT_FITTED_PARAMS] = ("Must be provided when the method is {0}.".
                                                   format(pfm_name))

        if pfm_name != self.getPropertyValue(self.PROP_METHOD) and\
           not self.getPropertyValue(self.PROP_PHASE_INFO_FILE):
            errors[self.PROP_PHASE_INFO_FILE] = ("Must be provided when using cell lattice "
                                                 "parameters revinement methods")

        min_x = self.getPropertyValue(self.PROP_MINX)
        max_x = self.getPropertyValue(self.PROP_MAXX)
        if min_x != Property.EMPTY_DBL and max_x != Property.EMPTY_DBL and min_x > max_x:
            errors[self.PROP_MINX] = ("The minimum given in {0} must be <= than the maximum "
                                      "given in {1}".format(self.PROP_MINX, self.PROP_MAXX))

        return errors

    def PyExec(self):
        prog = Progress(self, start=0, end=1, nreports=5)

        prog.report('Importing GSAS-II ')
        self._run_threadsafe(self._import_gsas2, self.getProperty(self.PROP_PATH_TO_GSASII).value)

        prog.report('Initializing GSAS-II ')
        gs2 = self._run_threadsafe(self._init_gs2)

        prog.report('Loading and preparing input data')
        focused_wks = self._get_focused_wks(self.PROP_INPUT_WORKSPACE, self.PROP_WORKSPACE_INDEX)

        inst_file = self.getProperty(self.PROP_INSTR_FILE).value
        try:
            (gs2_rd, limits, peaks_init, background_def) =\
                self._run_threadsafe(self._load_prepare_data_for_fit, gs2, focused_wks, inst_file)
        except RuntimeError as rexc:
            raise RuntimeError("Error in execution of GSAS-II data loading routines: "
                               "{0}.".format(str(rexc)))

        # No obvious way to provide proper progress report from inside the refinement/fitting routines
        prog.report('Running refinement. This may take some times')
        try:
            (gof_estimates, lattice_params, parm_dict) = \
                self._run_threadsafe(self._run_refinement,
                                     gs2, gs2_rd, (limits, peaks_init, background_def))
        except RuntimeError as rexc:
            raise RuntimeError("Error in execution of GSAS-II refinement routines: "
                               "{0}".format(str(rexc)))

        prog.report('Producing outputs')
        self._save_project_read_lattice(gs2, gs2_rd)
        self._produce_outputs(gof_estimates, lattice_params, parm_dict)

        import time
        time.sleep(0.1)

    def _run_threadsafe(self, func_call, *args, **kwargs):
        """
        GSAS-II is a wx application. When running inside MantidPlot it needs GUI/threadsafe
        treatment
        """
        # if 'mantidplot' in locals() or 'mantidplot' in globals():
        try:
            import pymantidplot
            return pymantidplot.threadsafe_call(func_call, *args, **kwargs)
        except ImportError:
            return func_call(*args, **kwargs)

    def _get_focused_wks(self, wks_prop_name, index_prop_name):
        in_wks = self.getProperty(wks_prop_name).value
        in_idx = self.getProperty(index_prop_name).value

        if in_wks.getNumberHistograms() > 1 or 0 != in_idx:
            focused_wks = msapi.ExtractSpectra(InputWorkspace = in_wks, StartWorkspaceIndex = in_idx,
                                               EndworkspaceIndex = in_idx)
        else:
            focused_wks = in_wks

        return focused_wks

    def _load_prepare_data_for_fit(self, gs2, gs2_focused_wks, inst_file):
        """
        Loads the data into an "rd" object as used in the GSAS-II python modules, and
        prepares an initia peaks list and a background function definition.
        Importantly, this reads the rd.powderdata list with the histogram data,
        and the rd.pwdparms tuple with instrument parameter lists

        @param gs2 :: the main GSAS-II object
        @param gs2_focused_wks :: a focused (single spectrum) workspace
        @param inst_file :: GSAS instrument parameters file (.par / .prm / .iparm, etc.)

        @returns a tuple with: 1) 3) GSAS-II "rd" object. 2) limits for fitting,
        3) list of peaks, 4) background definition, These are ready to be passed to the
        peak fitting functions
        """
        gs2_rd = self._build_gsas2_reader_with_data(gs2, gs2_focused_wks)
        self.log().information("Loaded histogram data in GSAS-II data object: {0}".
                               format(gs2_rd.powderdata))

        gs2_rd, inst_parms = self._add_instrument_info(gs2, gs2_rd, inst_file)
        self.log().information("Parameters from instrument file: {0}".format(gs2_rd.pwdparms))

        background_def = self._build_add_background_def(gs2_rd)
        self.log().information("Using background function: {0}".format(background_def))

        limits = self._build_add_limits(gs2_rd)
        self.log().information("Fitting loaded histogram data, with limits: {0}".format(limits))

        # self.PROP_PHASE_INFO_FILE - phase information into rd, this is loaded elsewhere when
        # doing Pawley/Rietveld refinement

        # Assumes peaks of type back-to-back exponential convoluted with pseudo-Voigt
        # That is true in ENGIN-X instrument parameters file
        peaks_init = self._init_peaks_list(gs2_rd, limits, inst_parms)
        self.log().information("Peaks parameters initialized as: {0}".format(peaks_init))

        return (gs2_rd, limits, peaks_init, background_def)

    def _run_refinement(self, gs2, gs2_rd, fit_inputs):
        """
        Run the different refinement/fitting methods

        @param gs2 :: the main GSAS-II object
        @param gs2_rd :: the GSAS-II "rd" object with powder data in it
        @param fit_inputs :: a tuple with inputs for the fitting process. 3 elements:
        limits, peaks_list, background_def. 1) limits is a tuple with the min X and max X values
        for the fitting. 2) peaks_list is a list of peaks to fit. 3) background_def is the
        background function as defined in GSAS-II (a list of parameters)

        @return a tuple with 1) the two goodness-of-fit estimates, 2) lattice params (list with
        7 values), 3) parameters fitted (when doing peak fitting)
        """
        method = self.getProperty(self.PROP_METHOD).value
        gof_estimates = [0, 0]
        lattice_params = 7*[0.0]
        parm_dict = {}
        if "Pawley refinement" == method:
            (gof_estimates, lattice_params) = self._run_rietveld_pawley_refinement(gs2, gs2_rd, True)
        elif "Rietveld refinement" == method:
            (gof_estimates, lattice_params) = self._run_rietveld_pawley_refinement(gs2, gs2_rd, False)
        elif "Peak fitting" == method:
            (gof_estimates, parm_dict) = self._run_peak_fit(gs2_rd, fit_inputs)
        else:
            raise RuntimeError("Inconsistency found. Unknown refinement method: {0}".format(method))

        return (gof_estimates, lattice_params, parm_dict)

    def _run_rietveld_pawley_refinement(self, gs2, gs2_rd, do_pawley):
        """
        Run Rietveld or Pawley refinement

        @param do_pawley :: Select Pawley (True) or Rietveld (False)
        @param gs2 :: the main GSAS-II object
        @param gs2_rd :: the GSAS-II "rd" object with powder data in it. Phase information will be
        connected to it

        @return a tuple with 1) the two goodness-of-fit estimates, 2) lattice params (list with
        7 values)
        """
        phase_data = self._load_prepare_phase_data(gs2, gs2_rd,
                                                   self.getProperty(self.PROP_PHASE_INFO_FILE).value)

        # Enable / tick on "Refine unit cell"
        general_phase_data = phase_data['General']
        general_phase_data['Cell'][0] = True
        if do_pawley:
            # Note from GSAS-II doc: "you probably should clear the Histogram scale factor refinement
            # flag (found in Sample parameters for the powder data set) as it cannot be refined
            # simultaneously with the Pawley reflection intensities"
            gs2_rd.Sample['Scale'] = [1.0, False]

            # Flag for Pawley intensity extraction (bool)
            general_phase_data['doPawley'] = True
            # maximum Q (as d-space) to use for Pawley extraction
            general_phase_data['Pawley dmin'] = self.getProperty(self.PROP_PAWLEY_DMIN).value
            # Restraint value for negative Pawley intensities
            general_phase_data['Pawley neg wt'] = self.getProperty(self.PROP_PAWLEY_NEG_WEIGHT).value

        proj_filename = self.getProperty(self.PROP_OUT_PROJECT_FILE).value
        self.log().notice("Saving GSAS-II project file before starting refinement, into: {0}".
                          format(proj_filename))

        # Save project with phase data imported and connected now
        self._save_gsas2_project(gs2, gs2_rd, proj_filename)

        residuals = self._do_refinement(gs2)
        gof_estimates = (residuals['Rwp'], residuals['GOF'], residuals['chisq'])
        # the first value is the flag refine on/off. The following 7 are the proper lattice params
        lattice_params = general_phase_data['Cell'][1:]

        import os
        self.log().notice("GSAS-II refinement details produced in file: {0}".
                          format(os.path.splitext(gs2.GSASprojectfile)[0]) + '.lst')

        return (gof_estimates, lattice_params)

    def  _do_refinement(self, gs2):
        """
        Calls the refinement routines of the structure module of GSASII

        @param gs2 :: the main GSAS-II object

        @return residuals, a dictionary that among other parameters contains 'Rwp',
        'GOF',  'chisq', and 'converged'
        """
        # assume gs2.GSASprojectfile == proj_filename which should be true because
        # The project file always needs to be saved before running refine
        import GSASIIstrIO
        import GSASIIstrMain
        err_msg, warn_msg = GSASIIstrIO.ReadCheckConstraints(gs2.GSASprojectfile)
        if err_msg:
            raise RuntimeError("Error in refinement: {0}".format(err_msg))
        if warn_msg:
            raise RuntimeError("Conflict between refinement flag settings "
                               "and constraints: {0}".format(err_msg))

        # note: if result_ok==False, residuals is actually an error message text!
        result_ok, residuals = GSASIIstrMain.Refine(gs2.GSASprojectfile, dlg=None, useDlg=False)
        if not result_ok:
            raise RuntimeError("There was a problem while running the core refinement routine. "
                               "Error description: {0}".format(residuals))
        else:
            self.log().notice("Refinement routine finished successfully with Rwp (weighted) "
                              "profile R-factor: {0}".format(residuals['Rwp']))

        return residuals

    def _run_peak_fit(self, gs2_rd, fit_inputs):
        """
        This performs peak fitting as in GSAS-II "Peaks List/Peak Fitting/PeakFitType".
        Does not require/use phase information. Requires histogram data, instrument parameters
        and background.

        @param gs2_rd :: the GSAS-II "rd" object with powder data in it. It must have a
        'powderdata' member with the histogram data as used in GSAS-II. a list of vectors
        (X, Y vectors)
        @param fit_inputs :: tuple with three inputs:
        1) tuple with the min X and max X values for the fitting
        2) peaks_list :: list of peaks to fit
        3) background_def :: background function as defined in GSAS-II, a list of parameters

        @return a tuple with: 1) a tuple with the Rwp and GoF values (weighted profile
        R-factor, goodness of fit), 2) the parameters dictionary
        """
        import GSASIIpwd

        (limits, peaks_list, background_def) = fit_inputs
        (inst_parm1, inst_parm2) = gs2_rd.pwdparms['Instrument Parameters']
        # peaks: ['pos','int','alp','bet','sig','gam'] / with the refine flag each
        sig_dict, result, sig, Rvals, vary_list, parm_dict, full_vary_list, bad_vary = \
            GSASIIpwd.DoPeakFit(FitPgm = 'LSQ', Peaks = peaks_list,
                                Background = background_def,
                                Limits = limits,
                                Inst = inst_parm1, Inst2 = inst_parm2,
                                data = gs2_rd.powderdata,
                                prevVaryList = None
                                # OneCycle = False, controls = None, dlg = None
                                )
        self.log().debug("Result: : {0}".format(result))
        Rwp = Rvals['Rwp']
        gof = Rvals['GOF']
        self.log().information("Rwp: : {0}".format(Rwp))
        self.log().information("GoF: : {0}".format(gof))
        self.log().information("'Sig': {0}".format(sig))
        self.log().information("'Sig', values: : {0}".format(sig_dict))
        self.log().information("List of parameters fitted: : {0}".format(vary_list))
        self.log().information("Parameters fitted, values: {0}".format(parm_dict))
        self.log().information("Full list of parameters: {0}".format(full_vary_list))
        self.log().information("Parameters for which issues were found when refining: {0}".
                               format(bad_vary))

        # chisq value (the 3rd) is not returned by DoPeakFit - TODO, how?
        gof_estimates = (Rwp, gof, 0)
        return (gof_estimates, parm_dict)

    def _import_gsas2(self, additional_path_prop):
        try:

            import sys
            import os
            if additional_path_prop:
                sys.path.append(additional_path_prop)
                os.chdir(additional_path_prop)
            self._import_global("GSASII")
            # It will be needed for save project
            self._import_global("GSASIIIO")
            # For powder diffraction data fitting routines
            self._import_global("GSASIIpwd")
            self._import_global("GSASIIgrid")
            # For phase data loading (yes, GUI)
            self._import_global("GSASIIphsGUI")
            self._import_global("GSASIIspc")
            # for Rietveld/Pawley refinement
            self._import_global("GSASIIstrIO")
            self._import_global("GSASIIstrMain")
            if additional_path_prop:
                sys.path.pop()
        except ImportError as ierr:
            raise ImportError("Failed to import the GSASII and its required sub-modules "
                              "from GSAS-II. Please make sure that it is available in the "
                              "Mantid Python path and/or the path to GSAS-II given in the "
                              "input property " + additional_path_prop + ". More error "
                              "details: " + str(ierr))

    def _import_global(self, mod_name):
        globals()[mod_name] = __import__(mod_name)

    def _init_gs2(self):
        # Do not feel tempted to create the GSASII wx app in the usual way:
        # _gsas2_app = GSASII.GSASIImain(0)
        # This will use Show() and SetTopWindow() and that will cause a crash when the
        # algorithm finishes and is destroyed!
        # This seems to destroy/close safely
        import wx
        import GSASII

        self._gsas2_app = wx.App()

        gs2 = GSASII.GSASII(None)
        return gs2

    def _build_gsas2_reader_with_data(self, gs2, gs2_focused_wks):
        """
        Build an "rd" GSAS-II data structure with reader, and importantly the rd.powderdata
        list with the histogram data.

        @param gs2_focused_wks :: a workspace with a histogram from focused data

        @return an "rd" object as used in GSAS-II, with histogram data in the 'powderdata'
        data member
        """

        # produce histo_file, to "import" it with the reader from GSAS-II which will initialize
        # the "rd" object.
        import tempfile
        with tempfile.NamedTemporaryFile(mode='w', suffix='.xye', delete=False) as histo_data_tmp:
            # SplitFiles=False is important to get the filename without suffix
            msapi.SaveFocusedXYE(InputWorkspace=gs2_focused_wks, Filename=histo_data_tmp.name,
                                 Format="XYE", SplitFiles=False)
            gs2_rd = self._get_histo_data_reader(gs2, histo_data_tmp.name)

        gs2_rd.powderdata = self._transform_to_centers_bins(gs2_rd.powderdata)

        if not isinstance(gs2_rd.powderdata, list):
            raise ValueError('rd.powderdata is not a list of array as expected')

        return gs2_rd

    def _get_histo_data_reader(self, gs2, histo_data_file):
        readers_list = self._init_histo_data_readers(gs2)
        if not isinstance(readers_list, list) or len(readers_list) < 6:
            raise RuntimeError("Could not find the reader of type G2pwd_xye.xye_ReaderClass. "
                               "Got a list of only {0} readers.".format(len(readers_list)))

        # 6 is a: 'G2pwd_xye.xye_ReaderClass object'. Warning: this can change sometimes
        reader_xye = [readers_list[6]]
        if not isinstance(reader_xye[0], object):
            raise RuntimeError("Could not find the reader of type G2pwd_xye.xye_ReaderClass. "
                               "Got this object: {0}".format(reader_xye))

        success, gs2_rd_list, err_msg = gs2.ImportDataGeneric(histo_data_file, reader_xye, [])
        if not success or 0 == len(gs2_rd_list):
            raise RuntimeError('Empty list of readers. Cannot continue. The error message from '
                               'GSAS-II is: ' + err_msg)

        # work only with the first one
        gs2_rd = gs2_rd_list[0]

        return gs2_rd

    def _transform_to_centers_bins(self, powderdata):
        """
        Transform data that comes as a list: X vector (bin boundaries), multiple Y vectors
        (values) into a list: X vector (bin centers), multiply Y vectors. This replicates
        the behavior of GSAS-II and the way it loads XYE files.

        @param powderdata :: two dimensional array. In the outermost dimension, the first
        element is a vector of X values. The next elements are vectors of Y values.
        """
        powderdata[0] = self._calc_centers_bins(powderdata[0])
        for pdi in range(1, len(powderdata)):
            powderdata[pdi] = powderdata[pdi][:-1]

        return powderdata

    def _calc_centers_bins(self, data):
        """
        Assuming that data is a vector of bin limits, changes it to the centers
        of the bins.

        @param data :: one-dimensional array
        """
        return (data[0:-1]+data[1:])/2.0

    def _add_instrument_info(self, gs2, gs2_rd, inst_file):
        if 'Instrument Parameters' not in gs2_rd.pwdparms:
            gs2.zipfile = None # init required before GetPowderIparm
            # Trick: if you pass lastIparmfile (being sure that it exits) it will be
            # picked up without having to ask via a pop-up dialog
            # An alternative is to set 'gs2_rd.instparm = inst_file' but that assumes both
            # the data and instrument files are in the same directory
            inst_parm1, inst_parm2 = gs2.GetPowderIparm(rd=gs2_rd, prevIparm=None,
                                                        lastIparmfile=inst_file, lastdatafile='')
            if not inst_parm1: # or not inst_parm2:  # (note inst_parm2 is commonly an empty dict)
                raise RuntimeError('Failed to import the instrument parameter structure')

        gs2_rd.pwdparms['Instrument Parameters'] = (inst_parm1, inst_parm2)

        return (gs2_rd, (inst_parm1, inst_parm2))

    def _build_add_background_def(self, gs2_rd):
        # Note: blatantly ignores self.getProperty(self.PROP_BACKGROUND_TYPE)
        backg_def = [['chebyschev', True, 3, 1.0, 0.0, 0.0],
                     {'peaksList': [], 'debyeTerms': [], 'nPeaks': 0, 'nDebye': 0}]

        gs2_rd.pwdparms['Background'] = backg_def

        return backg_def

    def _build_add_limits(self, gs2_rd):

        min_x = self.getProperty(self.PROP_MINX).value
        if Property.EMPTY_DBL == min_x:
            min_x = gs2_rd.powderdata[0].min()
        max_x = self.getProperty(self.PROP_MAXX).value
        if Property.EMPTY_DBL == max_x:
            max_x = gs2_rd.powderdata[0].max()

        limits = [min_x, max_x]
        gs2_rd.pwdparms['Limits'] = limits

        return limits

    def _init_peaks_list(self, gs2_rd, limits, inst_parms):
        # Bring the auto-search code out of that file! - TODO in GSAS
        import GSASIIpwdGUI

        (inst_parm1, inst_parm2) = inst_parms
        peaks_init = GSASIIpwdGUI.DoPeaksAutoSearch(gs2_rd.powderdata, limits, inst_parm1, inst_parm2)
        # Note this sets as default: refine intensity, and no other parameters
        for peak in peaks_init:
            peak[1] = self.getProperty(self.PROP_REFINE_CENTER).value
            peak[3] = self.getProperty(self.PROP_REFINE_INTENSITY).value
            peak[5] = self.getProperty(self.PROP_REFINE_ALPHA).value
            peak[7] = self.getProperty(self.PROP_REFINE_BETA).value
            # sigma (Gaussian)
            peak[9] = self.getProperty(self.PROP_REFINE_SIGMA).value
            # gamma (Lorentzian)
            peak[11] = self.getProperty(self.PROP_REFINE_GAMMA).value

        # Just to have the same sequence as GSAS-II in its tables/standard output
        peaks_init.sort()
        peaks_init.reverse()

        return peaks_init

    def _load_prepare_phase_data(self, gs2, gs2_rd, phase_filename):
        """
        Loads and sets up phase data from a phase information (CIF) file

        @param gs2 :: the main GSAS-II object
        @param gs2_rd :: the GSAS-II "rd" object with powder data in it
        @param phase_filename :: name of the CIF file

        @return phase data object as defined in GSAS-II GSASIIobj.py, with the imported phase
        information and other fields set up to defaults.
        """
        import GSASIIphsGUI

        # Import phase data from (CIF) file
        phase_readers_list = gs2.ImportPhaseReaderlist
        # 3 is G2phase_CIF.CIFPhaseReader
        phase_readers_list = [phase_readers_list[3]]

        _success, _rd_list, err_msg = gs2.ImportDataGeneric(phase_filename, phase_readers_list, [],
                                                            usedRanIdList=['noGUI'], Start=False)
        if err_msg:
            raise RuntimeError("There was a problem while importing the phase information file ({0}. "
                               "Error details: {1}".format(phase_filename, err_msg))

        phase_reader = phase_readers_list[0]
        GSASIIphsGUI.SetupGeneralWithoutGUI(gs2, phase_reader.Phase)
        phase_data = self._register_phase_data_to_histo(gs2, gs2_rd, phase_reader, phase_filename)

        return phase_data

    def _register_phase_data_to_histo(self, gs2, gs2_rd, phase_reader, phase_filename):
        # Register phase data and add it to the histo data
        import os
        import GSASIIgrid
        phase_name = os.path.basename(phase_filename)
        phase_reader.Phase['General']['Name'] = phase_name
        self.log().debug(" Phase information name: {0}".format(phase_name))

        if not GSASIIgrid.GetPatternTreeItemId(gs2, gs2.root, 'Phases'):
            sub = gs2.PatternTree.AppendItem(parent=gs2.root, text='Phases')
        else:
            sub = GSASIIgrid.GetPatternTreeItemId(gs2, gs2.root, 'Phases')

        psub = gs2.PatternTree.AppendItem(parent=sub, text=phase_name)
        gs2.PatternTree.SetItemPyData(psub, phase_reader.Phase)

        # Connect the phase information to the histogram data
        sub = GSASIIgrid.GetPatternTreeItemId(gs2, gs2.root, 'Phases')
        item, cookie = gs2.PatternTree.GetFirstChild(sub)
        phase_name = gs2.PatternTree.GetItemText(item)
        self.log().debug("Connecting phase information (name {0} to histogram data, with item: {1}, "
                         "cookie: {2}".format(phase_name, item, cookie))
        # the histo data is in for example 'PWDR ENGINX_ceria_1000_spectrum-0.txt'
        phase_data = gs2.PatternTree.GetItemPyData(item)

        self._setup_additional_phase_data(gs2_rd.idstring, phase_data)

        return phase_data

    def _setup_additional_phase_data(self, powder_histo_name, phase_data):
        """
        Setup more phase data parameters in 'Phases' / 'General' and
        'Phases' / 'Histograms'.

        @param phase_data :: from GSAS-II, the first entry in 'Phases'
        """
        import GSASII
        import GSASIIspc

        SGData = phase_data['General']['SGData']
        use_list = phase_data['Histograms']
        NShkl = len(GSASIIspc.MustrainNames(SGData))
        NDij = len(GSASIIspc.HStrainNames(SGData))
        # like 'PWDR ENGINX_ceria_1000_spectrum-0.txt'
        histo_name = 'PWDR ' + powder_histo_name
        # 'Reflection Lists' is not defined at this point:
        # item_id = GSASIIgrid.GetPatternTreeItemId(gs2, gs2.root, histo_name)
        # refList = gs2.PatternTree.GetItemPyData(
        #     GSASIIgrid.GetPatternTreeItemId(gs2, item_id, 'Reflection Lists'))
        # refList[general_phase_data['Name']] = {}
        use_list[histo_name] = GSASII.SetDefaultDData('PWDR', histo_name, NShkl=NShkl, NDij=NDij)

    def _produce_outputs(self, gof_estimates, lattice_params, parm_dict):
        (result_rwp, result_gof, _result_chisq) = gof_estimates
        self.setProperty(self.PROP_OUT_RWP, result_rwp)
        self.setProperty(self.PROP_OUT_GOF, result_gof)

        self._build_output_table(parm_dict, self.PROP_OUT_FITTED_PARAMS)
        self._build_output_lattice_table(lattice_params, self.PROP_OUT_LATTICE_PARAMS)

    def _build_output_table(self, parm_dict, tbl_prop_name):
        tbl_name = self.getPropertyValue(tbl_prop_name)
        if not tbl_name:
            return

        par_names = ['Center', 'Intensity', 'Alpha', 'Beta', 'Sigma', 'Gamma']
        par_prefixes = ['pos','int','alp','bet','sig','gam']
        table = msapi.CreateEmptyTableWorkspace(OutputWorkspace=tbl_name)

        num_peaks = 0
        while par_prefixes[0] + str(num_peaks) in parm_dict:
            num_peaks += 1

        for name in par_names:
            table.addColumn('double', name)

        for idx in range(0, num_peaks):
            par_values = [ parm_dict[par_prefix + str(idx)] for par_prefix in par_prefixes]
            print("par_values: ", par_values)
            table.addRow(par_values)

        for parm in parm_dict:
            self.log().debug("Parameters for output table: {0}".format(parm))

    def _build_output_lattice_table(self, lattice_params, tbl_prop_name):
        tbl_name = self.getPropertyValue(tbl_prop_name)
        if not tbl_name:
            return

        alg = self.createChildAlgorithm('CreateEmptyTableWorkspace')
        alg.execute()
        table = alg.getProperty('OutputWorkspace').value
        self.setProperty(tbl_prop_name, table)

        table.addColumn('double', 'a')
        table.addColumn('double', 'b')
        table.addColumn('double', 'c')
        table.addColumn('double', 'alpha')
        table.addColumn('double', 'beta')
        table.addColumn('double', 'gamma')
        table.addColumn('double', 'volume')

        table.addRow([float(par) for par in lattice_params])

    def _save_project_read_lattice(self, gs2, gs2_rd):
        """
        To save the project at the very end, and parse lattice params from the output .lst
        file from GSAS.

        @param gs2 :: the main GSAS-II object
        @param gs2_rd :: the GSAS-II "rd" object with powder data in it
        """
        out_proj_file = self.getProperty(self.PROP_OUT_PROJECT_FILE).value
        if out_proj_file:
            # Not totally sure if this save will leave less information in the output
            # file as compared with the save done from the core refine routines.
            # those routines save information in the project file that is apparently not
            # updated in the project tree (other than loading the saved project file).
            self._save_gsas2_project(gs2, gs2_rd, out_proj_file)
            try:
                file_lattice_params = self._parse_lattice_params_refined(out_proj_file)
                self.log().notice("Lattice parameters found in output file: {0}".format(file_lattice_params))
            except IOError:
                self.log().notice("The output project lst file was not found for this project: {0}".
                                  format(out_proj_file))

    def _parse_lattice_params_refined(self, out_proj_file):
        """
        Parses lattice parameters from the output .lst file (refinement results)
        corresponding to the project file given as input

        @param out_proj_file : GSAS-II project file name

        @Returns a tuple with the lattice parameter values refined. 7 parameters:
        (a, b, c, alpha, beta, gamma, Volume)
        """
        import os
        import re
        lst_filename = os.path.splitext(out_proj_file)[0] + '.lst'
        with open(lst_filename) as results_file:
            results_lst = results_file.read()

            re_lattice_params = (r"Unit\s+cell:\s+a\s+=\s+(\d+.\d+)\s+b\s+=\s+(\d+.\d+)\s+c\s+=\s+(\d+.\d+)"
                                 r"\s+alpha\s+=\s+(\d+.\d+)\s+beta\s+=\s+(\d+.\d+)\s+gamma\s+=\s+(\d+.\d+)"
                                 r"\s+volume\s+=\s+(\d+.\d+)")
            pattern = re.compile(re_lattice_params)
            lines_match = pattern.findall(results_lst)

            # Depending on what refinement options are enabled the cell info is produced in different
            # places and formats. Alternatively look for something like
            # values:    2.470000    2.470000    6.790000     90.0000     90.0000    120.0000      35.875
            if not lines_match:
                more_re_lattice_params = (r"\s+values:\s+(\d+.\d+)\s+(\d+.\d+)\s+(\d+.\d+)\s+(\d+.\d+)"
                                          r"\s+(\d+.\d+)\s+(\d+.\d+)\s+(\d+.\d+)")
                pattern = re.compile(more_re_lattice_params)
                lines_match = pattern.findall(results_lst)

            params = lines_match[0]

        return params

    def _save_lattice_params_file(self, _gs2, out_lattice_file):
        (latt_a, latt_b, latt_c, latt_alpha, latt_beta, latt_gamma) = 6*[0]
        # To grab parameters from the gs2 object:
        # G2gd.GetPatternTreeItemId(self,self.root,'Phases')
        with open(out_lattice_file, 'w') as lattice_txt:
            print("a, b, c, alpha, beta, gamma", file=lattice_txt)
            print(("{0}, {1}, {2}, {3}, {4}, {5}".
                   format(latt_a, latt_b, latt_c, latt_alpha, latt_beta, latt_gamma)), file=lattice_txt)

    def _prepare_save_gsas2_project(self, gs2, gs2_rd):
        """
        GSAS-II projects are saved/loaded from/to the tree of the main window.
        This populates the GSAS-II GUI tree, getting it ready for saving.

        It needs to save at least all the elements save here even if we are not
        using effectively (for example 'Sample Parameters'). Otherwise the code
        will fail in various places (for example at the end of DoPeakFit).
        This is based on GSASII.OnDummyPowder, GSASII.GetPWDRdatafromTree,
        GSASII.OnImportPowder

        Assumes that the (two) instrument parameter objects have been added in
        gs2_rd.pwdparms['Instrument Parameters']
        that the limits have been initialized in
        gs2_rd.pwdparms['Limits']
        and that the background has been initialized in
        gs2_rd.pwdparms['Background']
        as it would be done in GSAS-II (sometimes).

        @param gs2 :: the main GSAS-II object
        @param gs2_rd :: the GSAS-II "rd" object with powder data
        """

        import random
        import sys
        import GSASIIgrid

        # set GUI tree items
        histo_name = 'PWDR ' + gs2_rd.idstring
        tree_id = gs2.PatternTree.AppendItem(parent=gs2.root, text=histo_name)
        valuesdict = {
            'wtFactor':1.0,
            'Dummy':True,
            'ranId':random.randint(0,sys.maxsize),
            'Offset':[0.0,0.0],'delOffset':0.02,'refOffset':-1.0,'refDelt':0.01,
            'qPlot':False,'dPlot':False,'sqrtPlot':False
        }
        # Warning: with the comment "this should be removed someday"
        gs2_rd.Sample['ranId'] = valuesdict['ranId']
        gs2.PatternTree.SetItemPyData(tree_id, [valuesdict, gs2_rd.powderdata])

        gs2.PatternTree.SetItemPyData(
            gs2.PatternTree.AppendItem(tree_id, text='Comments'),
            gs2_rd.comments)

        gs2.PatternTree.SetItemPyData(
            gs2.PatternTree.AppendItem(tree_id, text='Limits'),
            [gs2_rd.pwdparms['Limits'], [gs2_rd.powderdata[0].min(),
                                         gs2_rd.powderdata[0].max()]])
        gs2.PatternId = GSASIIgrid.GetPatternTreeItemId(gs2, tree_id, 'Limits')

        gs2.PatternTree.SetItemPyData(
            gs2.PatternTree.AppendItem(tree_id, text='Background'),
            gs2_rd.pwdparms['Background'])

        gs2.PatternTree.SetItemPyData(
            gs2.PatternTree.AppendItem(tree_id, text='Instrument Parameters'),
            gs2_rd.pwdparms['Instrument Parameters'])

        gs2.PatternTree.SetItemPyData(
            gs2.PatternTree.AppendItem(tree_id, text='Sample Parameters'),
            gs2_rd.Sample)

        gs2.PatternTree.SetItemPyData(
            gs2.PatternTree.AppendItem(tree_id, text='Peak List')
            ,{'peaks':[],'sigDict':{}})

        gs2.PatternTree.SetItemPyData(
            gs2.PatternTree.AppendItem(tree_id, text='Index Peak List'),
            [[],[]])
        gs2.PatternTree.SetItemPyData(
            gs2.PatternTree.AppendItem(tree_id, text='Unit Cells List'),
            [])
        gs2.PatternTree.SetItemPyData(
            gs2.PatternTree.AppendItem(tree_id, text='Reflection Lists'),
            {})

    def _save_gsas2_project(self, gsas2, gs2_rd, proj_filename):
        """
        Saves all the information loaded into a GSAS-II project file that can be loaded
        in the GSAS-II GUI (.gpx files).

        @param gsas2 :: the main GSAS-II object
        @param gs2_rd :: the GSAS-II "rd" object with powder data
        @param proj_filename :: name of the output project file
        """
        self.log().notice("Preparing GSAS-II project tree to save into: {0}".format(proj_filename))
        self._prepare_save_gsas2_project(gsas2, gs2_rd)

        import GSASIIIO
        self.log().debug("Saving GSAS-II project: {0}".format(gsas2))
        gsas2.GSASprojectfile = proj_filename
        gsas2.CheckNotebook()
        GSASIIIO.ProjFileSave(gsas2)

# Need GSAS-II _init_Imports()
#pylint: disable=protected-access
    def _init_histo_data_readers(self, gs2):
        gs2._init_Imports()
        readers_list = gs2.ImportPowderReaderlist
        return readers_list

AlgorithmFactory.subscribe(GSASIIRefineFitPeaks)
