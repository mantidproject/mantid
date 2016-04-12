from mantid.api import AlgorithmFactory, ITableWorkspaceProperty, FileAction, FileProperty, \
    MatrixWorkspaceProperty, Progress, PropertyMode, PythonAlgorithm
from mantid.kernel import Direction, FloatArrayProperty, StringListValidator

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

    def PyInit(self):
        PROP_INPUT_WORKSPACE = 'InputWorkspace'
        PROP_WORKSPACE_INDEX = 'WorkspaceIndex'
        PROP_INSTR_FILE = 'InstrumentFile'
        PROP_PHASE_INFO_FILE = 'PhaseInfoFile'
        PROP_BACKGROUND_TYPE = 'BackgroundType'
        PROP_EXPECTED_PEAKS = "ExpectedPeaks"
        PROP_EXPECTED_PEAKS_FROM_FILE = "ExpectedPeaksFromFile"
        PROP_OUT_FITTED_PARAMS = 'FittedPeakParameters'
        PROP_OUT_PROJECT_FILE = 'SaveProjectFile'
        PROP_OUT_GOF = 'GoF'
        PROP_OUT_RWP = 'Rwp'
        PROP_REFINE_CENTER = 'RefineCenter'
        PROP_REFINE_INTENSITY = 'RefineIntensity'
        PROP_REFINE_ALPHA = 'RefineAlpha'
        PROP_REFINE_BETA = 'RefineBeta'
        PROP_REFINE_SIGMA = 'RefineSigma'
        PROP_REFINE_GAMMA = 'RefineGamma'

        self.declareProperty(MatrixWorkspaceProperty(PROP_INPUT_WORKSPACE, "",
                                                     optional = PropertyMode.Mandatory,
                                                     direction = Direction.Input),
                             doc = 'Workspace with spectra to fit peaks. ToF is expected X unit.')

        self.declareProperty(PROP_WORKSPACE_INDEX, 0,
                             doc = 'Index of the workspace for the spectrum to fit. By default '
                             'the first spectrum will be processed (that is, the only spectrum '
                             'for focussed data workspaces.', direction = Direction.Input)

        self.declareProperty(FileProperty(name = PROP_INSTR_FILE, defaultValue = "",
                                          action = FileAction.Load,
                                          extensions = [".par", ".prm", ".ipar", ".iparm"]),
                             doc = 'File with instrument parameters (in GSAS format).')

        background_types = ["Chebyshev", "None"]
        self.declareProperty(name = PROP_BACKGROUND_TYPE, defaultValue = background_types[0],
                             validator = StringListValidator(background_types),
                             doc = 'Type of background for the peak fitting.')

        # Phase information: TODO
        self.declareProperty(FileProperty(name = PROP_PHASE_INFO_FILE, defaultValue = "",
                                          action = FileAction.OptionalLoad, extensions = [".cif"]),
                             doc = 'File with phase information for the material.')

        GRP_PEAKS = "Expected peaks (leave empty to auto-find)"

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

        self.declareProperty(ITableWorkspaceProperty(PROP_OUT_FITTED_PARAMS, "", Direction.Output),
                             doc = "Fitted parameters. One row per peak found.")

        self.declareProperty(FileProperty(name = PROP_OUT_PROJECT_FILE, defaultValue = "",
                                          direction = Direction.Input,
                                          action = FileAction.OptionalSave, extensions = [".gpx"]),
                             doc = 'GSAS-II project file (that can be openened in the GSAS-II GUI).')

        self.setPropertyGroup(PROP_OUT_GOF, GRP_RESULTS)
        self.setPropertyGroup(PROP_OUT_RWP, GRP_RESULTS)
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
        pass

    def PyExec(self):
        path_gsas2 = self.getPropertyValue("PathToGSASII")

        try:
            import GSASIIpwd
            import GSASII
        except ImportError as ierr:
            raise ImportError("Failed to import the GSASII and GSASIIpwd modules "
                              "from GSAS-II. Please check that it is available in the "
                              "Python path and/or the path to GSAS-II given in the "
                              "input properties: " + path_gsas2 + ". More error "
                              "details: " +ierr)

        # Split this into 3/4 calls. TODO

        # Should do much better - TODO
        prog = Progress(self, start=0, end=1, nreports=3)

        prog.report('Initializing GSAS-II ')
        gs2 = self._init_gs2()

        in_wks = self.getProperty(PROP_INPUT_WORKSPACE).value
        in_idx = self.getProperty(PROP_WORKSPACE_INDEX)

        prog.report('Loading and preparing input data')
        focused_wks = sapi.ExtractSpectra(InputWorkspace = in_wks, StartWorkspaceIndex = in_idx,
                                          EndworkspaceIndex = in_idx)

        # produce histo_file
        histo_data_file = 'tmp.xye'
        sapi.SaveFocusedXYE(InputWorkspace=focused_wks, Filename=histo_data_file)

        gs2_rd = self.get_histo_data_reader(gs2, histo_file)

        gs2_rd.powderdata = _transform_to_centers_bins(gs2_rd.powderdata)

        if not isinstance(gs2_rd.powderdata, list):
            raise ValueError('rd.powderdata is not a list of array as expected')

        inst_file = self.getProperty(PROP_INSTR_FILE)
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

        gs2_rd.pwdparms = (inst_parm1, inst_parm2)

        self.log().information("Parameters from instrument file: {0}".format(gs2_rd.pwdparms))

        # Note: blatantly ignores self.getProperty(PROP_PHASE_INFO_FILE)
        background_def = [['chebyschev', True, 3, 1.0, 0.0, 0.0],
                          {'peaksList': [], 'debyeTerms': [], 'nPeaks': 0, 'nDebye': 0}]

        self.log().information("Using background function: {0}".format(background_def))

        peaks_init = self.init_peaks_list()
        self.log().information("Peaks parameters initialized as: {0}".format(peaks_init))

        prog.report('Running refinement')
        (gof_estimates, parm_dict) = self._run_peak_fit(peaks_init, background_def, gs2_rd.powderdata)

        proj_file = self.getProperty(PROP_OUT_PROJECT_FILE)
        if proj_file:
            # Save in gpx project format- TODO
            pass

        (result_rwp, result_gof) = gof_estimates
        self.setProperty(PROP_OUT_RWP, result_rwp)
        self.setProperty(PROP_OUT_GOF, result_gof)

        _build_output_table(parm_dict)


    def _run_peak_fit(self, peaks_list, background_def, powderdata):
        """
        Explain parameters! TODO

        @returns a tuple with: 1) a tuple with the Rwp and GoF values (weighted profile
        R-factor, goodness of fit), 2) the parameters dictionary
        """
        limits = [powderdata[0].min(), powderdata[0].max()]
        self.log().notice("Loaded histogram data with limits: {0}".format(limits))

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
        self.log().information("'Sig': : {0}".format(sig))
        self.log().information("'Sig', values: : {0}".format(sig_dict))
        self.log().information("List of parameters fitted: : {0}".format(vary_list))
        self.log().information("Parameters fitted, values: {0}".format(parm_dict))
        self.log().information("Full list of parameters: {0}".format(full_vary_list))
        self.log().information("Parameters for which issues were found when refining: {0}".
                               format(bad_vary))

        return (Rvals, parm_dict)

    def _init_gs2(self):
        _gsas2_app = GSASII.GSASIImain(0)
        gs2 = GSASII.GSASII(None)
        return gs2

    def _get_histo_data_reader(self, gs2):
        readers_list = self._init_histo_data_readers(gs2)

        # 5 is a: 'G2pwd_xye.xye_ReaderClass object'
        reader_xye = [readers_list[5]]
        if not isinstance(reader, gs2.G2pwd_xye.xye_ReaderClass):
            raise RuntimeError("Could not find the reader of type G2pwd_xye.xye_ReaderClass")

        success, gs2_rd_list, err_msg = gs2.ImportDataGeneric(datafile, reader_xye, [])
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

        peaks_init = GSASIIpwdGUI.DoPeaksAutoSearch(gs2_rd.powderdata, limits, inst_parm1, inst_parm2)
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

    def _build_output_table(self, parm_dict):
        # build the table property - TODO
        for parm in parm_dict:
            self.log().debug("Parameter: {0}".format(parm))

# Need GSAS-II _init_Imports()
#pylint: disable=protected-access
    def _init_histo_data_readers(self, gs2):
        gs2._init_Imports()
        readers_list = gs2.ImportPowderReaderlist
        return readers_list

AlgorithmFactory.subscribe(GSASIIFitPeaks)
