from mantid.api import AlgorithmFactory, ITableWorkspaceProperty, FileAction, FileProperty, \
    MatrixWorkspaceProperty, Progress, PropertyMode, PythonAlgorithm
from mantid.kernel import Direction, FloatArrayProperty, Property, StringListValidator

class GSASIIFitPeaks(PythonAlgorithm):
    """
    Mantid algorithm to use the powder diffraction and related data
    from the powder diffraction module of GSAS-II
    (https://subversion.xray.aps.anl.gov/trac/pyGSAS)
    """

    def category(self):
        """
        Override required for Mantid algorithms
        """
        return "Diffraction\\Engineering"

    def name(self):
        """
        Override required for Mantid algorithms
        """
        return "GSASIIFitPeaks"

    def summary(self):
        """
        Override required for Mantid algorithms
        """
        return ("Uses GSAS-II (powder diffraction module) to fit peaks to one "
                "or more spectra from a workspace")

    # Too many properties!
    #pylint: disable=too-many-locals
    def PyInit(self):
        PROP_INPUT_WORKSPACE = 'InputWorkspace'
        PROP_WORKSPACE_INDEX = 'WorkspaceIndex'
        PROP_INSTR_FILE = 'InstrumentFile'
        PROP_PHASE_INFO_FILE = 'PhaseInfoFile'
        PROP_PATH_TO_GSASII = 'PathToGSASII'
        PROP_BACKGROUND_TYPE = 'BackgroundType'
        PROP_MINX = 'MinX'
        PROP_MAXX = 'MaxX'
        PROP_EXPECTED_PEAKS = "ExpectedPeaks"
        PROP_EXPECTED_PEAKS_FROM_FILE = "ExpectedPeaksFromFile"
        PROP_OUT_LATTICE_PARAMS = 'LatticeParameters'
        PROP_OUT_FITTED_PARAMS = 'FittedPeakParameters'
        PROP_OUT_PROJECT_FILE = 'SaveGSASIIProjectFile'
        PROP_OUT_GOF = 'GoF'
        PROP_OUT_RWP = 'Rwp'
        PROP_REFINE_CENTER = 'RefineCenter'
        PROP_REFINE_INTENSITY = 'RefineIntensity'
        PROP_REFINE_ALPHA = 'RefineAlpha'
        PROP_REFINE_BETA = 'RefineBeta'
        PROP_REFINE_SIGMA = 'RefineSigma'
        PROP_REFINE_GAMMA = 'RefineGamma'

        PROP_METHOD = "Method"
        refine_methods = ["Rietveld/Pawley refinement", "Peak fitting"]
        self.declareProperty(PROP_METHOD, defaultValue = refine_methods[0],
                             validator = StringListValidator(refine_methods),
                             doc = 'Rietveld corresponds to the Calculate/Refine option of the '
                             'GSAS-II GUI. Peak fitting is single peak (does not use phase '
                             'information  and corresponds to the option '
                             'Peaks List/Peak Fitting/PeakFitType of the GSAS-II GUI. This '
                             'second alternative requires a list of peaks which can be bassed in '
                             'the properties ' + PROP_EXPECTED_PEAKS + ' and ' +
                             PROP_EXPECTED_PEAKS_FROM_FILES + '.')

        self.declareProperty(MatrixWorkspaceProperty(PROP_INPUT_WORKSPACE, '',
                                                     optional = PropertyMode.Mandatory,
                                                     direction = Direction.Input),
                             doc = 'Workspace with spectra to fit peaks. ToF is expected X unit.')

        self.declareProperty(PROP_WORKSPACE_INDEX, 0,
                             doc = 'Index of the workspace for the spectrum to fit. By default '
                             'the first spectrum will be processed (that is, the only spectrum '
                             'for focussed data workspaces.', direction = Direction.Input)


        self.declareProperty(FileProperty(name = PROP_INSTR_FILE, defaultValue = '',
                                          action = FileAction.Load,
                                          extensions = [".par", ".prm", ".ipar", ".iparm"]),
                             doc = 'File with instrument parameters (in GSAS format).')

        # Phase information: TODO
        self.declareProperty(FileProperty(name = PROP_PHASE_INFO_FILE, defaultValue = '',
                                          action = FileAction.OptionalLoad, extensions = [".cif"]),
                             doc = 'File with phase information for the material.')

        self.declareProperty(FileProperty(name = PROP_PATH_TO_GSASII, defaultValue = '',
                                          action = FileAction.OptionalDirectory),
                             doc = 'Optional path to GSAS-II software installation. '
                             'This will be used to import several Python modules from GSAS-II.')

        GRP_FITTING_OPTS = "Fitting options"
        background_types = ["Chebyshev", "None"]
        self.declareProperty(PROP_BACKGROUND_TYPE, defaultValue = background_types[0],
                             validator = StringListValidator(background_types),
                             doc = 'Type of background for the peak fitting. Currently only the '
                             'default option of GSAS-II (chebyshev) is supported.')

        self.declareProperty(PROP_MINX, Property.EMPTY_DBL,
                             direction = Direction.Input,
                             doc = "Minimum x value for the fitting, in the same units as the input "
                             "workspace (TOF). Defines the range or domain of fitting together "
                             "with the property {0}. ".format(PROP_MAXX))

        self.declareProperty(PROP_MAXX, Property.EMPTY_DBL,
                             direction = Direction.Input,
                             doc = "Maximum x value for the fitting, in the same units as the input "
                             "workspace (TOF). Defines the range or domain of fitting together "
                             "with the property {0}.".format(PROP_MINX))

        self.setPropertyGroup(PROP_BACKGROUND_TYPE, GRP_FITTING_OPTS)
        self.setPropertyGroup(PROP_MINX, GRP_FITTING_OPTS)
        self.setPropertyGroup(PROP_MAXX, GRP_FITTING_OPTS)

        GRP_PEAKS = "Expected peaks (phase information takes precedence)"

        self.declareProperty(FloatArrayProperty(PROP_EXPECTED_PEAKS, [],
                                                direction = Direction.Input),
                             "A list of dSpacing values for the peak centers. These will be "
                             "converted into TOF to find expected peaks.")

        self.declareProperty(FileProperty(name = PROP_EXPECTED_PEAKS_FROM_FILE, defaultValue = "",
                                          action = FileAction.OptionalLoad, extensions = [".csv"],
                                          direction = Direction.Input),
                             doc = "Load from this file a list of dSpacing values to be converted "
                             "into TOF . This takes precedence over '" + PROP_EXPECTED_PEAKS + "' "
                             "when both options are given.")

        self.setPropertyGroup(PROP_EXPECTED_PEAKS, GRP_PEAKS)
        self.setPropertyGroup(PROP_EXPECTED_PEAKS_FROM_FILE, GRP_PEAKS)


        GRP_RESULTS = "RESULTS"

        self.declareProperty('GoF', 0.0, direction = Direction.Output,
                             doc = 'Goodness of fit value (Chi squared).')

        self.declareProperty('Rwp', 0.0, direction = Direction.Output,
                             doc = 'Weighted profile R-factor (Rwp) discrepancy index for the '
                             'goodness of fit.')

        self.declareProperty(ITableWorkspaceProperty(PROP_OUT_LATTICE_PARAMS, Direction.Output),
                             doc = 'Table to output the the lattice parameters (refined).')

        self.declareProperty(ITableWorkspaceProperty(PROP_OUT_FITTED_PARAMS, "", Direction.Output),
                             doc = "Fitted parameters. One row per peak found.")

        self.declareProperty(FileProperty(name = PROP_OUT_PROJECT_FILE, defaultValue = '',
                                          direction = Direction.Input,
                                          action = FileAction.OptionalSave, extensions = [".gpx"]),
                             doc = 'GSAS-II project file (that can be openened in the GSAS-II GUI).')

        self.setPropertyGroup(PROP_OUT_GOF, GRP_RESULTS)
        self.setPropertyGroup(PROP_OUT_RWP, GRP_RESULTS)
        self.setPropertyGroup(PROP_OUT_LATTICE_PARAMS, GRP_RESULTS)
        self.setPropertyGroup(PROP_OUT_FITTED_PARAMS, GRP_RESULTS)
        self.setPropertyGroup(PROP_OUT_PROJECT_FILE, GRP_RESULTS)

        self.declareProperty(name = PROP_REFINE_CENTER, defaultValue = False,
                             doc = 'Whether to refine the peak centers.')

        self.declareProperty(name = PROP_REFINE_INTENSITY, defaultValue = False,
                             doc = 'Whether to refine the peak function intensity parameters '
                             '(assuming a shape of type back-to-back exponential convoluted '
                             'with pseudo-voigt (BackToBackExponentialPV).')

        self.declareProperty(name = PROP_REFINE_ALPHA, defaultValue = False,
                             doc = 'Whether to refine the peak function beta parameters '
                             '(assuming a BackToBackExponentialPV peak shape.')

        self.declareProperty(name = PROP_REFINE_BETA, defaultValue = False,
                             doc = 'Whether to refine the peak function beta parameters '
                             '(assuming a BackToBackExponentialPV peak shape.')

        self.declareProperty(name = PROP_REFINE_SIGMA, defaultValue = True,
                             doc = 'Whether to refine the peak function sigma parameters '
                             '(assuming a BackToBackExponentialPV peak shape.')

        self.declareProperty(name = PROP_REFINE_GAMMA, defaultValue = True,
                             doc = 'Whether to refine the peak function gamma parameters '
                             '(assuming a BackToBackExponentialPV peak shape.')

        GRP_PARAMS = "Refinement of peak parameters"
        self.setPropertyGroup(PROP_REFINE_CENTER, GRP_PARAMS)
        self.setPropertyGroup(PROP_REFINE_INTENSITY, GRP_PARAMS)
        self.setPropertyGroup(PROP_REFINE_ALPHA, GRP_PARAMS)
        self.setPropertyGroup(PROP_REFINE_BETA, GRP_PARAMS)
        self.setPropertyGroup(PROP_REFINE_SIGMA, GRP_PARAMS)
        self.setPropertyGroup(PROP_REFINE_GAMMA, GRP_PARAMS)

    def validateInputs(self):
        # This could check if the required inputs for different methods have been provided
        pass

    def PyExec(self):
        prog = Progress(self, start=0, end=1, nreports=5)

        prog.report('Importing GSAS-II ')
        self._import_gsas2(PROP_PATH_TO_GSASII)

        prog.report('Initializing GSAS-II ')
        gs2 = self._init_gs2()


        prog.report('Loading and preparing input data')
        in_wks = self.getProperty(PROP_INPUT_WORKSPACE).value
        in_idx = self.getProperty(PROP_WORKSPACE_INDEX)
        inst_file = self.getProperty(PROP_INSTR_FILE)

        if in_wks.getNumberHistograms() > 1 or 0 != in_idx:
            gs2_focused_wks = sapi.ExtractSpectra(InputWorkspace = in_wks, StartWorkspaceIndex = in_idx,
                                                  EndworkspaceIndex = in_idx)
        else:
            gs2_focused_wks = in_wks

        (gs2_rd, limits, peaks_init, background_def) = self._load_prepare_data_for_fit(gs2_focused_wks,
                                                                                       inst_file)

        # No obvious way to provide proper progress report from inside the fitting routines
        prog.report('Running refinement. This may take some times')
        (gof_estimates, parm_dict) = self._run_peak_fit(gs2_rd.powderdata, limits, peaks_init,
                                                        background_def)

        prog.report('Producing outputs')
        out_proj_file = self.getProperty(PROP_OUT_PROJECT_FILE)
        lattice_params = []
        if proj_file:
            self._save_gsas2_project(gs2, gs2_rd, out_proj_file)
            lattice_params = self._parse_lattice_params_refined(out_proj_file)
            self.log().notice("Lattice parameters fitted: {0}".format(lattice_params))

        self._produce_outputs(gof_estimates, lattice_params, parm_dict)

    def _load_prepare_data_for_fit(self, gs2_focused_wks, inst_file):
        """
        Loads the data into an "rd" object as used in the GSAS-II python modules, and
        prepares an initia peaks list and a background function definition.
        Importantly, this reads the rd.powderdata list with the histogram data,
        and the rd.pwdparms tuple with instrument parameter lists

        @param gs2_focused_wks :: a focused (single spectrum) workspace
        @param inst_file :: GSAS instrument parameters file (.par / .prm / .iparm, etc.)

        @returns a tuple with: 1) 3) GSAS-II "rd" object. 2) limits for fitting,
        3) list of peaks, 4) background definition, These are ready to be passed to the
        peak fitting functions
        """
        gs2_rd = self._build_gsas2_reader_with_data(gs2_focused_wks)
        self.log().information("Loaded histogram data in GSAS-II data object: {0}".
                               format(gs2_rd.powderdata))

        gs2_rd = self._add_instrument_info(gs2_rd, inst_file)
        self.log().information("Parameters from instrument file: {0}".format(gs2_rd.pwdparms))

        background_def = self._build_add_background_def(gs2_rd)
        self.log().information("Using background function: {0}".format(background_def))

        limits = self._build_add_limits(gs2_rd)
        self.log().information("Fitting loaded histogram data, with limits: {0}".format(limits))

        # PROP_PHASE_INFO_FILE - phase information into rd
        # As in GSASII.GetPhaseData - TODO

        # Assumes peaks of type back-to-back exponential convoluted with pseudo-Voigt
        # That is true in ENGIN-X instrument parameters file
        peaks_init = self._init_peaks_list()
        self.log().information("Peaks parameters initialized as: {0}".format(peaks_init))

        return (gs2_rd, limits, peaks_init, background_def)

    def _run_peak_fit(self, powderdata, limits, peaks_list, background_def):
        """
        This performs peak fitting as in GSAS-II "Peaks List/Peak Fitting/PeakFitType".
        Does not require/use phase information. Requires histogram data, instrument parameters
        and background.

        @param powderdata :: histogram data as used in GSAS-II. a list of vectors (X, Y vectors)
        @param limits :: tuple with the min X and max X values for the fitting
        @param peaks_list :: list of peaks to fit
        @param background_def :: background function as defined in GSAS-II, a list of parameters

        @returns a tuple with: 1) a tuple with the Rwp and GoF values (weighted profile
        R-factor, goodness of fit), 2) the parameters dictionary
        """
        # peaks: ['pos','int','alp','bet','sig','gam'] / with the refine flag each
        sig_dict, result, sig, Rvals, vary_list, parm_dict, full_vary_list, bad_vary = \
            GSASIIpwd.DoPeakFit(FitPgm = 'LSQ', Peaks = peaks_list,
                                Background = background_def,
                                Limits = limits,
                                Inst = inst_parm1, Inst2 = inst_parm2,
                                data = powderdata,
                                prevVaryList = None
                                # OneCycle = False, controls = None, dlg = None
                               )
        self.log().information("Result: : {0}".format(result))
        self.log().information("Rwp: : {0}".format(Rvals[0]))
        self.log().information("GoF: : {0}".format(Rvals[1]))
        self.log().information("'Sig': {0}".format(sig))
        self.log().information("'Sig', values: : {0}".format(sig_dict))
        self.log().information("List of parameters fitted: : {0}".format(vary_list))
        self.log().information("Parameters fitted, values: {0}".format(parm_dict))
        self.log().information("Full list of parameters: {0}".format(full_vary_list))
        self.log().information("Parameters for which issues were found when refining: {0}".
                               format(bad_vary))

        return (Rvals, parm_dict)

    def _import_gsas2(self, additional_path_prop):
        try:
            import GSASII
            # It will be needed for save project
            import GSASIIIO
            # For powder diffraction data fitting routines
            import GSASIIpwd
        except ImportError as ierr:
            raise ImportError("Failed to import the GSASII and its required sub-modules "
                              "from GSAS-II. Please make sure that it is available in the "
                              "Mantid Python path and/or the path to GSAS-II given in the "
                              "input property " + getProperty(additional_path_prop) +
                              ". More error details: " + ierr)

    def _init_gs2(self):
        _gsas2_app = GSASII.GSASIImain(0)
        gs2 = GSASII.GSASII(None)
        return gs2

    def _build_gsas2_reader_with_data(self, gs2_focused_wks):
        """
        Build an "rd" GSAS-II data structure with reader, and importantly the rd.powderdata
        list with the histogram data.

        @param gs2_focused_wks :: a workspace with a histogram from focused data

        @return an "rd" object as used in GSAS-II, with histogram data in the 'powderdata'
        data member
        """

        # produce histo_file, to use the reader from GSAS-II which will initialize the
        # "rd" object
        histo_data_filename = 'tmp_gsasii_histo_data.xye'
        sapi.SaveFocusedXYE(InputWorkspace=gs2_focused_wks, Filename=histo_data_filename)

        gs2_rd = self._get_histo_data_reader(gs2, histo_file)

        gs2_rd.powderdata = self._transform_to_centers_bins(gs2_rd.powderdata)

        if not isinstance(gs2_rd.powderdata, list):
            raise ValueError('rd.powderdata is not a list of array as expected')

        return gs2_rd

    def _add_instrument_info(self, gs2_rd, inst_file):
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

        return gs2_rd

    def _build_add_background_def(self, gs2_rd):
        # Note: blatantly ignores self.getProperty(PROP_BACKGROUND_TYPE)
        backg_def = [['chebyschev', True, 3, 1.0, 0.0, 0.0],
                     {'peaksList': [], 'debyeTerms': [], 'nPeaks': 0, 'nDebye': 0}]

        gs2_rd.pwdparms['Background'] = backg_def

        return backg_def

    def _build_add_limits(self, gs2_rd):

        min_x = getProperty(PROP_MINX)
        if Property.EMPTY_DBL == minx:
            min_x = powderdata[0].min()
        max_x = getProperty(PROP_MAXX)
        if Property.EMPTY_DBL == max_x:
            max_x = powderdata[0].max()

        limits = [min_x, max_x]
        gs2_rd.pwdparms['Limits'] = limits

        return limits

    def _get_histo_data_reader(self, gs2, histo_data_file):
        readers_list = self._init_histo_data_readers(gs2)

        # 5 is a: 'G2pwd_xye.xye_ReaderClass object'
        reader_xye = [readers_list[5]]
        if not isinstance(reader, gs2.G2pwd_xye.xye_ReaderClass):
            raise RuntimeError("Could not find the reader of type G2pwd_xye.xye_ReaderClass")

        success, gs2_rd_list, err_msg = gs2.ImportDataGeneric(histo_data_file, reader_xye, [])
        if not success or 0 == len(rd_list):
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
        powderdata[0] = _calc_centers_bins(powderdata[0])
        for pdi in range(1, len(powderdata)):
            powderdata[pdi] = powderdata[pdi][:-1]

        return powderdata

    def calc_centers_bins(self, data):
        """
        Assuming that data is a vector of bin limits, changes it to the centers
        of the bins.

        @param data :: one-dimensional array
        """
        return (data[0:-1]+data[1:])/2.0

    def _init_peaks_list(self):
        # Bring the auto-search code out of that file! - TODO
        import GSASIIpwdGUI

        peaks_init = GSASIIpwdGUI.DoPeaksAutoSearch(gs2_rd.powderdata, inst_parm1, inst_parm2)
        # Note this sets as default: refine intensity, and no other parameters
        for peak in peaks_init:
            peak[1] = getProperty(PROP_REFINE_CENTER)
            peak[3] = getProperty(PROP_REFINE_INTENSITY)
            peak[5] = getProperty(PROP_REFINE_ALPHA)
            peak[7] = getProperty(PROP_REFINE_BETA)
            # sigma (Gaussian)
            peak[9] = getProperty(PROP_REFINE_SIGMA)
            # gamma (Lorentzian)
            peak[11] = getProperty(PROP_REFINE_GAMMA)

        # Just to have the same sequence as GSAS-II in its tables/standard output
        peaks_init.sort()
        peaks_init.reverse()

        return peaks_init

    def _produce_outputs(self, gof_estimates, lattice_params, parm_dict):
        (result_rwp, result_gof) = gof_estimates
        self.setProperty(PROP_OUT_RWP, result_rwp)
        self.setProperty(PROP_OUT_GOF, result_gof)

        self._build_output_table(parm_dict)
        self._build_output_lattice_table(lattice_params)

    def _build_output_table(self, parm_dict):
        # build the table property - TODO
        par_names = ['Center', 'Intensity', 'Alpha', 'Beta', 'Sigma', 'Gamma']
        par_prefixes = ['pos','int','alp','bet','sig','gam']

        tbl_name = self.getProperty(PROP_OUT_FITTED_PARAMS)
        alg = self.createChildAlgorithm('CreateEmptyTableWorkspace')
        alg.setProperty('OutputWorkspace', tbl_name)
        alg.execute()
        table = alg.getProperty('OutputWorkspace').value

        num_peaks = 0
        while par_prefixes[0] + str(num_peaks+1) in parm_dict:
            num_peaks += 1

        for name in par_names:
            table.addColumn('str', name)
        for i in range(1, num_peaks+1):
            par_values = [ parm_dict[par_prefix + str(i)] for par_prefix in par_prefixes]
            table.addRow(par_values)

        for parm in parm_dict:
            self.log().debug("Parameters for output table: {0}".format(parm))

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
        lst_filename = os.path.splitext(out_proj_file)[0] + 'lst'
        with open(lst_filename) as results_file:
            results_lst = results_file.read()
            re_lattice_params = (r"Unit\s+cell:\s+a\s+=\s+(\d+.\d+)\s+b\s+=\s+(\d+.\d+)\s+c\s+=\s+(\d+.\d+)"
                                 r"\s+alpha\s+=\s+(\d+.\d+)\s+beta\s+=\s+(\d+.\d+)\s+gamma\s+=\s+(\d+.\d+)"
                                 r"\s+volume\s+=\s+(\d+.\d+)")
            pattern = re.compile(re_lattice_params)
            params = pattern.findall(results_lst)[0]

        return params

    def _build_output_lattice_table(self, lattice_params):
        tbl_name = self.getProperty(PROP_OUT_LATTICE_PARAMS)
        alg = self.createChildAlgorithm('CreateEmptyTableWorkspace')
        alg.setProperty('OutputWorkspace', tbl_name)
        alg.execute()
        table = alg.getProperty('OutputWorkspace').value

        table.addColumn('double', 'a')
        table.addColumn('double', 'b')
        table.addColumn('double', 'c')
        table.addColumn('double', 'alpha')
        table.addColumn('double', 'beta')
        table.addColumn('double', 'gamma')
        table.addColumn('double', 'volume')

        table.addRow([double(par) for par in lattice_params])

    def _save_lattice_params_file(self, _gs2, out_lattice_file):
        (latt_a, latt_b, latt_c, latt_alpha, latt_beta, latt_gamma) = 6*[0]
        # grab parameters from gs2 - TODO
        # G2gd.GetPatternTreeItemId(self,self.root,'Phases')
        with open(out_lattice_file, 'wt') as lattice_txt:
            print >>lattice_txt, "a, b, c, alpha, beta, gamma"
            print >>lattice_txt, ("{0}, {1}, {2}, {3}, {4}, {5}".
                                  format(latt_a, latt_b, latt_c, latt_alpha, latt_beta, latt_gamma))

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
            'ranId':random.randint(0,sys.maxint),
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

    def _save_gsas2_project(self, gsas2, gs2_rd, filename):
        """
        Saves all the information loaded into a GSAS-II project file that can be loaded
        in the GSAS-II GUI (.gpx files).

        @param gsas2 :: the main GSAS-II object
        @param gs2_rd :: the GSAS-II "rd" object with powder data
        @param filename :: name of the output project file
        """
        self.notice("Preparing GSAS-II project tree to save into: {0}".format(filename))
        self._prepare_save_gsas2_project(gs2, gs2_rd)

        import GSASIIIO
        self.debug("Saving GSAS-II project: {0}".format(gsas2))
        gs2.GSASprojectfile = proj_filename
        gs2.CheckNotebook()
        GSASIIIO.ProjFileSave(gs2)

# Need GSAS-II _init_Imports()
#pylint: disable=protected-access
    def _init_histo_data_readers(self, gs2):
        gs2._init_Imports()
        readers_list = gs2.ImportPowderReaderlist
        return readers_list

AlgorithmFactory.subscribe(GSASIIFitPeaks)
