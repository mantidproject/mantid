#pylint: disable=no-init,too-many-instance-attributes
from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as api
from mantid.api import *
from mantid.kernel import *
from mantid import config
import os

DEPRECATION_NOTICE = "BASISReduction311 is deprecated (on 2017-03-11). Use BASISReduction instead."
DEFAULT_BINS = [-740, 1.6, 740]
DEFAULT_QBINS = [0.4, 0.2, 3.8]
DEFAULT_WRANGE = [6.24, 6.30]
DEFAULT_MASK_GROUP_DIR = "/SNS/BSS/shared/autoreduce/new_masks_08_12_2015"
DEFAULT_MASK_FILE = "BASIS_Mask_OneQuarterRemains_SouthBottom.xml"
DEFAULT_CONFIG_DIR = config["instrumentDefinition.directory"]

DEFAULT_ENERGY = 7.6368


class BASISReduction311(PythonAlgorithm):

    _short_inst = None
    _long_inst = None
    _extension = None
    _doIndiv = None
    _etBins = None
    _qBins = None
    _noMonNorm = None
    _maskFile = None
    _groupDetOpt = None
    _overrideMask = None
    _dMask = None
    _doNorm = None
    _normRange = None
    _norm_run_list = None
    _normWs = None
    _normMonWs = None
    _run_list = None
    _samWs = None
    _samMonWs = None
    _samWsRun = None
    _samSqwWs = None

    def category(self):
        return "Inelastic\\Reduction"

    def seeAlso(self):
        return [ "BASISReduction" ]

    def name(self):
        return "BASISReduction311"

    def summary(self):
        return DEPRECATION_NOTICE

    def PyInit(self):
        self._short_inst = "BSS"
        self._long_inst = "BASIS"
        self._extension = "_event.nxs"

        self.declareProperty("RunNumbers", "", "Sample run numbers")
        self.declareProperty("DoIndividual", False, "Do each run individually")
        self.declareProperty("NoMonitorNorm", False,
                             "Stop monitor normalization")
        self.declareProperty("NormRunNumbers", "", "Normalization run numbers")
        arrVal = FloatArrayLengthValidator(2)
        self.declareProperty(FloatArrayProperty("NormWavelengthRange", DEFAULT_WRANGE,
                                                arrVal, direction=Direction.Input),
                             "Wavelength range for normalization. default:(6.24A, 6.30A)")
        self.declareProperty(FloatArrayProperty("EnergyBins", DEFAULT_BINS,
                                                direction=Direction.Input),
                             "Energy transfer binning scheme (in ueV)")
        self.declareProperty(FloatArrayProperty("MomentumTransferBins",
                                                DEFAULT_QBINS,
                                                direction=Direction.Input),
                             "Momentum transfer binning scheme")
        self.declareProperty(FileProperty(name="MaskFile", defaultValue="",
                                          action=FileAction.OptionalLoad, extensions=['.xml']),
                             "Directory location for standard masking and grouping files.")
        grouping_type = ["None", "Low-Resolution", "By-Tube"]
        self.declareProperty("GroupDetectors", "None",
                             StringListValidator(grouping_type),
                             "Switch for grouping detectors")

    def PyExec(self):
        self.log().error(DEPRECATION_NOTICE)
        config['default.facility'] = "SNS"
        config['default.instrument'] = self._long_inst
        self._doIndiv = self.getProperty("DoIndividual").value
        self._etBins = 1.E-03 * self.getProperty("EnergyBins").value  # micro-eV to mili-eV
        self._qBins = self.getProperty("MomentumTransferBins").value
        self._noMonNorm = self.getProperty("NoMonitorNorm").value
        self._maskFile = self.getProperty("MaskFile").value
        self._groupDetOpt = self.getProperty("GroupDetectors").value

        datasearch = config["datasearch.searcharchive"]
        if datasearch != "On":
            config["datasearch.searcharchive"] = "On"

        # Handle masking file override if necessary
        self._overrideMask = bool(self._maskFile)
        if not self._overrideMask:
            config.appendDataSearchDir(DEFAULT_MASK_GROUP_DIR)
            self._maskFile = DEFAULT_MASK_FILE

        api.LoadMask(Instrument='BASIS', OutputWorkspace='BASIS_MASK',
                     InputFile=self._maskFile)

        # Work around length issue
        _dMask = api.ExtractMask('BASIS_MASK')
        self._dMask = _dMask[1]
        api.DeleteWorkspace(_dMask[0])

        # Do normalization if run numbers are present
        norm_runs = self.getProperty("NormRunNumbers").value
        self._doNorm = bool(norm_runs)
        self.log().information("Do Norm: " + str(self._doNorm))
        if self._doNorm:
            if ";" in norm_runs:
                raise SyntaxError("Normalization does not support run groups")
            # Setup the integration (rebin) parameters
            normRange = self.getProperty("NormWavelengthRange").value
            self._normRange = [normRange[0], normRange[1]-normRange[0], normRange[1]]

            # Process normalization runs
            self._norm_run_list = self._getRuns(norm_runs)
            for norm_set in self._norm_run_list:
                extra_extension = "_norm"
                self._normWs = self._makeRunName(norm_set[0])
                self._normWs += extra_extension
                self._normMonWs = self._normWs + "_monitors"
                self._sumRuns(norm_set, self._normWs, self._normMonWs, extra_extension)
                self._calibData(self._normWs, self._normMonWs)

            api.Rebin(InputWorkspace=self._normWs, OutputWorkspace=self._normWs,
                      Params=self._normRange)
            api.FindDetectorsOutsideLimits(InputWorkspace=self._normWs,
                                           OutputWorkspace="BASIS_NORM_MASK")

        self._run_list = self._getRuns(self.getProperty("RunNumbers").value)
        for run_set in self._run_list:
            self._samWs = self._makeRunName(run_set[0])
            self._samMonWs = self._samWs + "_monitors"
            self._samWsRun = str(run_set[0])
            self._sumRuns(run_set, self._samWs, self._samMonWs)
            # After files are all added, run the reduction

            self._calibData(self._samWs, self._samMonWs)

            if self._doNorm:
                api.MaskDetectors(Workspace=self._samWs,
                                  MaskedWorkspace='BASIS_NORM_MASK')
                api.Divide(LHSWorkspace=self._samWs, RHSWorkspace=self._normWs,
                           OutputWorkspace=self._samWs)

            api.ConvertUnits(InputWorkspace=self._samWs,
                             OutputWorkspace=self._samWs,
                             Target='DeltaE', EMode='Indirect')
            api.CorrectKiKf(InputWorkspace=self._samWs,
                            OutputWorkspace=self._samWs,
                            EMode='Indirect')

            api.Rebin(InputWorkspace=self._samWs,
                      OutputWorkspace=self._samWs,
                      Params=self._etBins)
            if self._groupDetOpt != "None":
                if self._groupDetOpt == "Low-Resolution":
                    grp_file = "BASIS_Grouping_LR.xml"
                else:
                    grp_file = "BASIS_Grouping.xml"
                # If mask override used, we need to add default grouping file location to
                # search paths
                if self._overrideMask:
                    config.appendDataSearchDir(DEFAULT_MASK_GROUP_DIR)

                api.GroupDetectors(InputWorkspace=self._samWs,
                                   OutputWorkspace=self._samWs,
                                   MapFile=grp_file, Behaviour="Sum")

            self._samSqwWs = self._samWs+'_sqw'
            api.SofQW3(InputWorkspace=self._samWs,
                       OutputWorkspace=self._samSqwWs,
                       QAxisBinning=self._qBins, EMode='Indirect',
                       EFixed=DEFAULT_ENERGY)

            dave_grp_filename = self._makeRunName(self._samWsRun,
                                                  useShort=False) + ".dat"
            api.SaveDaveGrp(Filename=dave_grp_filename,
                            InputWorkspace=self._samSqwWs,
                            ToMicroEV=True)
            processed_filename = self._makeRunName(self._samWsRun,
                                                   useShort=False) + "_sqw.nxs"
            api.SaveNexus(Filename=processed_filename,
                          InputWorkspace=self._samSqwWs)

    def _getRuns(self, rlist):
        """
        Create sets of run numbers for analysis. A semicolon indicates a
        separate group of runs to be processed together.
        """
        run_list = []
        rlvals = rlist.split(';')
        for rlval in rlvals:
            iap = IntArrayProperty("", rlval)
            if self._doIndiv:
                run_list.extend([[x] for x in iap.value])
            else:
                run_list.append(iap.value)
        return run_list

    def _makeRunName(self, run, useShort=True):
        """
        Make name like BSS_24234
        """
        if useShort:
            return self._short_inst + "_" + str(run)
        else:
            return self._long_inst + "_" + str(run)

    def _makeRunFile(self, run):
        """
        Make name like BSS24234
        """
        return self._short_inst + str(run)

    def _sumRuns(self, run_set, sam_ws, mon_ws, extra_ext=None):
        for run in run_set:
            ws_name = self._makeRunName(run)
            if extra_ext is not None:
                ws_name += extra_ext
            mon_ws_name = ws_name  + "_monitors"
            run_file = self._makeRunFile(run)
            # Reflection 311 is restricted to bank with name "bank2"
            api.LoadEventNexus(Filename=run_file, BankName="bank2", OutputWorkspace=ws_name)

            if not self._noMonNorm:
                api.LoadNexusMonitors(Filename=run_file,
                                      OutputWorkspace=mon_ws_name)
            if sam_ws != ws_name:
                api.Plus(LHSWorkspace=sam_ws, RHSWorkspace=ws_name,
                         OutputWorkspace=sam_ws)
                api.DeleteWorkspace(ws_name)
            if mon_ws != mon_ws_name and not self._noMonNorm:
                api.Plus(LHSWorkspace=mon_ws,
                         RHSWorkspace=mon_ws_name,
                         OutputWorkspace=mon_ws)
                api.DeleteWorkspace(mon_ws_name)

    def _calibData(self, sam_ws, mon_ws):
        api.LoadParameterFile(Workspace=sam_ws,
                              Filename=os.path.join(DEFAULT_CONFIG_DIR,
                                                    'BASIS_silicon_311_Parameters.xml'))
        api.MaskDetectors(Workspace=sam_ws,
                          DetectorList=self._dMask)
                          #MaskedWorkspace='BASIS_MASK')
        api.ModeratorTzeroLinear(InputWorkspace=sam_ws, OutputWorkspace=sam_ws)
        api.ConvertUnits(InputWorkspace=sam_ws,
                         OutputWorkspace=sam_ws,
                         Target='Wavelength', EMode='Indirect')

        if not self._noMonNorm:
            api.ModeratorTzeroLinear(InputWorkspace=mon_ws,
                                     OutputWorkspace=mon_ws)
            api.Rebin(InputWorkspace=mon_ws,
                      OutputWorkspace=mon_ws, Params='10')
            api.ConvertUnits(InputWorkspace=mon_ws,
                             OutputWorkspace=mon_ws,
                             Target='Wavelength')
            api.OneMinusExponentialCor(InputWorkspace=mon_ws,
                                       OutputWorkspace=mon_ws,
                                       C='0.20749999999999999',
                                       C1='0.001276')
            api.Scale(InputWorkspace=mon_ws,
                      OutputWorkspace=mon_ws,
                      Factor='9.9999999999999995e-07')
            api.RebinToWorkspace(WorkspaceToRebin=sam_ws,
                                 WorkspaceToMatch=mon_ws,
                                 OutputWorkspace=sam_ws)
            api.Divide(LHSWorkspace=sam_ws,
                       RHSWorkspace=mon_ws,
                       OutputWorkspace=sam_ws)


# Register algorithm with Mantid.
AlgorithmFactory.subscribe(BASISReduction311)
