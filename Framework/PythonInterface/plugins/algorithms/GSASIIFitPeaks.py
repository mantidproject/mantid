from mantid.api import AlgorithmFactory, ITableWorkspaceProperty, FileAction, FileProperty, \
    MatrixWorkspaceProperty, Progress, PropertyMode, PythonAlgorithm, WorkspaceProperty
from mantid.kernel import Direction, FloatArrayProperty, StringListValidator, \
    StringMandatoryValidator

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
        PROP_SPECTRUM_LIST = 'SpectrumList'
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

        self.declareProperty(PROP_SPECTRUM_LIST, "1", StringMandatoryValidator(),
                             doc = 'List of spectra to fit. Leave empty (default) to fit all the '
                             'spectra.', direction = Direction.Input)

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
        

    def PyExec(self):
        gs2 = self._init_gs2()

        gs2_rd = self.get_histo_data_reader(gs2)


    def validateInputs(self):
        path_gsas2 = getPropertyValue("PathToGSASII")

        try:
            import GSASIIpwd
            import GSASII
        except ImportError as ierr:
            raise ImportError("Failed to import the GSASII and GSASIIpwd modules "
                              "from GSAS-II. Please check that it is available in the "
                              "Python path and/or the path to GSAS-II given in the "
                              "input properties: " + path_gsas2 + ". More error "
                              "details: " +ierr)

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
        if 0 == len(rd_list):
            raise RuntimeError('Empty list of readers. Cannot continue')
        # work only with the first one
        gs2_rd = gs2_rd_list[0]

# Need GSAS-II _init_Imports()
#pylint: disable=protected-access
    def _init_histo_data_readers(self, gs2):
        gs2._init_Imports()
        readers_list = gs2.ImportPowderReaderlist

AlgorithmFactory.subscribe(GSASIIFitPeaks)
