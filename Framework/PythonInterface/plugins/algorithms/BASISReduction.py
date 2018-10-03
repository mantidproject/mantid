# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init
from __future__ import (absolute_import, division, print_function)

import numpy as np
import json

import mantid.simpleapi as sapi
from mantid.api import (mtd, PythonAlgorithm, AlgorithmFactory, FileProperty,
                        FileAction, AnalysisDataService)
from mantid.kernel import (IntArrayProperty, StringListValidator,
                           FloatArrayProperty, EnabledWhenProperty,
                           FloatArrayLengthValidator, Direction,
                           PropertyCriterion)
from mantid import config
from os.path import join as pjoin

TEMPERATURE_SENSOR = "SensorA"
DEFAULT_RANGE = [6.24, 6.30]
DEFAULT_MASK_GROUP_DIR = "/SNS/BSS/shared/autoreduce/new_masks_08_12_2015"
DEFAULT_CONFIG_DIR = config["instrumentDefinition.directory"]

# BASIS allows two possible reflections, with associated default properties
#pylint: disable=line-too-long
REFLECTIONS_DICT = {"silicon111": {"name": "silicon111",
                                   "energy_bins": [-150, 0.4, 500],  # micro-eV
                                   "q_bins": [0.3, 0.2, 1.9],  # inverse Angstroms
                                   "mask_file": "BASIS_Mask_default_111.xml",
                                   "parameter_file": "BASIS_silicon_111_Parameters.xml",
                                   "default_energy": 2.0826,  # mili-eV
                                   "vanadium_bins": [-0.0034, 0.068, 0.0034]  # mili-eV
                                   },
                    "silicon311": {"name": "silicon311",
                                   "energy_bins": [-740, 1.6, 740],
                                   "q_bins": [0.5, 0.2, 3.7],
                                   "mask_file": "BASIS_Mask_default_311.xml",
                                   "parameter_file": "BASIS_silicon_311_Parameters.xml",
                                   "default_energy": 7.6368,  # mili-eV
                                   "vanadium_bins": [-0.015, 0.030, 0.015]# mili-eV
                                   }
                    }

#pylint: disable=too-many-instance-attributes


class BASISReduction(PythonAlgorithm):

    _short_inst = None
    _long_inst = None
    _extension = None
    _doIndiv = None

    _groupDetOpt = None
    _overrideMask = None
    _dMask = None
    _run_list = None  # a list of runs, or a list of sets of runs
    _samWs = None

    _samWsRun = None
    _samSqwWs = None
    _debugMode = False  # delete intermediate workspaces if False

    def __init__(self):
        PythonAlgorithm.__init__(self)
        self._normalizeToFirst = False
        self._as_json = None

        # properties related to monitor
        self._MonNorm = None

        # properties related to the chosen reflection
        self._reflection = None  # entry in the reflections dictionary
        self._etBins = None
        self._qBins = None
        self._maskFile = None

        # properties related to division by Vanadium (normalization)
        self._doNorm = None  # stores the selected item from normalization_types
        self._normalizationType = None
        self._normRange = None
        self._norm_run_list = None
        self._normWs = None
        self._normMonWs = None

        # properties related to saving NSXPE file
        self._nsxpe_do = False
        self._nxspe_psi_angle_log = None
        self._nxspe_offset = 0.0

    def category(self):
        return 'Inelastic\\Reduction'

    def name(self):
        return 'BASISReduction'

    def version(self):
        return 1

    def summary(self):
        return 'Multiple-file BASIS reduction for its two reflections.'

    def PyInit(self):
        self._short_inst = 'BSS'
        self._long_inst = 'BASIS'
        self._extension = '_event.nxs'

        self.declareProperty('RunNumbers', '', 'Sample run numbers')
        self.declareProperty('DoIndividual', False, 'Do each run individually')
        self.declareProperty('MonitorNorm', True,
                             'Normalization with wavelength-dependent monitor counts')
        self.declareProperty('ExcludeTimeSegment', '',
                             'Exclude a contigous time segment; '+
                             'Examples: "71546:0-60" filter run 71546 from '+
                             'start to 60 seconds, "71546:300-600", '+
                             '"71546:120-end" from 120s to the end of the run')
        grouping_type = ["None", "Low-Resolution", "By-Tube"]
        self.declareProperty("GroupDetectors", "None",
                             StringListValidator(grouping_type),
                             "Switch for grouping detectors")
        self.declareProperty('NormalizeToFirst', False, 'Normalize spectra '+
                             'to intensity of spectrum with lowest Q?')

        # Properties affected by the reflection selected
        titleReflection = 'Reflection Selector'
        available_reflections = sorted(REFLECTIONS_DICT.keys())
        default_reflection = REFLECTIONS_DICT['silicon111']
        self.declareProperty('ReflectionType', default_reflection['name'],
                             StringListValidator(available_reflections),
                             'Analyzer. Documentation lists typical \
                             associated property values.')
        self.setPropertyGroup('ReflectionType', titleReflection)
        self.declareProperty(FloatArrayProperty('EnergyBins',
                                                default_reflection['energy_bins'],
                                                direction=Direction.Input),
                             'Energy transfer binning scheme (in ueV)')
        self.setPropertyGroup('EnergyBins', titleReflection)
        self.declareProperty(FloatArrayProperty('MomentumTransferBins',
                                                default_reflection['q_bins'],
                                                direction=Direction.Input),
                             'Momentum transfer binning scheme')
        self.setPropertyGroup('MomentumTransferBins', titleReflection)
        self.declareProperty(FileProperty(name='MaskFile', defaultValue='',
                                          action=FileAction.OptionalLoad,
                                          extensions=['.xml']),
                             'See documentation for latest mask files.')
        self.setPropertyGroup('MaskFile', titleReflection)

        # Properties setting the division by vanadium
        titleDivideByVanadium = 'Normalization by Vanadium'
        self.declareProperty('DivideByVanadium', False, direction=Direction.Input,
                             doc='Do we normalize by the vanadium intensity?')
        self.setPropertyGroup('DivideByVanadium', titleDivideByVanadium)
        ifDivideByVanadium = EnabledWhenProperty('DivideByVanadium',
                                                 PropertyCriterion.IsNotDefault)

        normalization_types = ['by Q slice', 'by detector ID']
        self.declareProperty('NormalizationType', 'by Q slice',
                             StringListValidator(normalization_types),
                             'Select a Vanadium normalization')
        self.setPropertySettings('NormalizationType', ifDivideByVanadium)
        self.setPropertyGroup('NormalizationType', titleDivideByVanadium)

        self.declareProperty('NormRunNumbers', '', 'Normalization run numbers')
        self.setPropertySettings('NormRunNumbers', ifDivideByVanadium)
        self.setPropertyGroup('NormRunNumbers', titleDivideByVanadium)
        arrVal = FloatArrayLengthValidator(2)
        self.declareProperty(FloatArrayProperty('NormWavelengthRange',
                                                DEFAULT_RANGE,
                                                arrVal,
                                                direction=Direction.Input),
                             'Wavelength range for normalization')
        self.setPropertySettings('NormWavelengthRange', ifDivideByVanadium)
        self.setPropertyGroup('NormWavelengthRange', titleDivideByVanadium)

        # Properties setting the saving of NSXPE file
        title_nxspe= 'Save to NXSPE'
        self.declareProperty('SaveNXSPE', False, direction=Direction.Input,
                             doc='Do we save to NXSPE format?')
        nxspe_enabled = EnabledWhenProperty('SaveNXSPE',
                                            PropertyCriterion.IsNotDefault)
        self.setPropertyGroup('SaveNXSPE', title_nxspe)
        self.declareProperty('PsiAngleLog', 'SE50Rot', direction=Direction.Input,
                             doc='name of entry in the logs storing the psi \
                             angle')
        self.setPropertySettings('PsiAngleLog', nxspe_enabled)
        self.setPropertyGroup('PsiAngleLog', title_nxspe)
        self.declareProperty('PsiOffset', 0.0, direction=Direction.Input,
                             doc='add this quantity to the psi angle stored \
                             in the log')
        self.setPropertySettings('PsiOffset', nxspe_enabled)
        self.setPropertyGroup('PsiOffset', title_nxspe)

        # Aditional output properties
        titleAddionalOutput = 'Additional Output'
        self.declareProperty('OutputSusceptibility', False,
                             direction=Direction.Input,
                             doc='Output dynamic susceptibility (Xqw)')
        self.setPropertyGroup('OutputSusceptibility', titleAddionalOutput)

    #pylint: disable=too-many-branches
    def PyExec(self):
        config['default.facility'] = 'SNS'
        config['default.instrument'] = self._long_inst
        self._reflection =\
            REFLECTIONS_DICT[self.getProperty('ReflectionType').value]
        self._doIndiv = self.getProperty('DoIndividual').value
        # micro-eV to mili-eV
        self._etBins = 1.E-03 * self.getProperty('EnergyBins').value
        self._qBins = self.getProperty('MomentumTransferBins').value
        self._qBins[0] -= self._qBins[1]/2.0  # leftmost bin boundary
        self._qBins[2] += self._qBins[1]/2.0  # rightmost bin boundary
        self._MonNorm = self.getProperty('MonitorNorm').value
        self._maskFile = self.getProperty('MaskFile').value
        maskfile = self.getProperty('MaskFile').value
        self._maskFile = maskfile if maskfile else\
            pjoin(DEFAULT_MASK_GROUP_DIR, self._reflection['mask_file'])
        self._groupDetOpt = self.getProperty('GroupDetectors').value
        self._normalizeToFirst = self.getProperty('NormalizeToFirst').value
        self._doNorm = self.getProperty('DivideByVanadium').value

        # retrieve properties pertaining to saving to NXSPE file
        self._nsxpe_do = self.getProperty('SaveNXSPE').value
        if self._nsxpe_do:
            self._nxspe_psi_angle_log = self.getProperty('PsiAngleLog').value
            self._nxspe_offset = self.getProperty('PsiOffset').value

        datasearch = config["datasearch.searcharchive"]
        if datasearch != "On":
            config["datasearch.searcharchive"] = "On"

        # Apply default mask if not supplied by user
        self._overrideMask = bool(self._maskFile)
        if not self._overrideMask:
            config.appendDataSearchDir(DEFAULT_MASK_GROUP_DIR)
            self._maskFile = self._reflection["mask_file"]

        sapi.LoadMask(Instrument='BASIS',
                      OutputWorkspace='BASIS_MASK',
                      InputFile=self._maskFile)

        # Work around length issue
        _dMask = sapi.ExtractMask('BASIS_MASK')
        self._dMask = _dMask[1]
        sapi.DeleteWorkspace(_dMask[0])

        ############################
        ##  Process the Vanadium  ##
        ############################

        norm_runs = self.getProperty("NormRunNumbers").value
        if self._doNorm and bool(norm_runs):
            if ";" in norm_runs:
                raise SyntaxError("Normalization does not support run groups")
            self._normalizationType = self.getProperty("NormalizationType").value
            self.log().information("Divide by Vanadium with normalization" +
                                   self._normalizationType)

            # Following steps common to all types of Vanadium normalization

            # norm_runs encompasses a single set, thus _getRuns returns
            # a list of only one item
            norm_set = self._getRuns(norm_runs, doIndiv=False)[0]
            normWs = self._sum_and_calibrate(norm_set, extra_extension="_norm")

            normRange = self.getProperty("NormWavelengthRange").value
            bin_width = normRange[1] - normRange[0]
            # This rebin integrates counts onto a histogram of a single bin
            if self._normalizationType == "by detector ID":
                self._normRange = [normRange[0], bin_width, normRange[1]]
                sapi.Rebin(InputWorkspace=normWs,
                           OutputWorkspace=normWs,
                           Params=self._normRange)
                self._normWs = normWs
            # FindDetectorsOutsideLimits to be substituted by MedianDetectorTest
            sapi.FindDetectorsOutsideLimits(InputWorkspace=normWs,
                                            LowThreshold=1.0*bin_width,
                                            # no count events outside ranges
                                            RangeLower=normRange[0],
                                            RangeUpper=normRange[1],
                                            OutputWorkspace='BASIS_NORM_MASK')
            # additional reduction steps when normalizing by Q slice
            if self._normalizationType == "by Q slice":
                self._normWs = self._group_and_SofQW(normWs, self._etBins,
                                                     isSample=False)

        ##########################
        ##  Process the sample  ##
        ##########################
        self._run_list = self._getRuns(self.getProperty("RunNumbers").value,
                                       doIndiv=self._doIndiv)
        for run_set in self._run_list:
            self._samWs = self._sum_and_calibrate(run_set)
            self._samWsRun = str(run_set[0])
            # Divide by Vanadium detector ID, if pertinent
            if self._normalizationType == "by detector ID":
                # Mask detectors with insufficient Vanadium signal before dividing
                sapi.MaskDetectors(Workspace=self._samWs,
                                   MaskedWorkspace='BASIS_NORM_MASK')
                sapi.Divide(LHSWorkspace=self._samWs,
                            RHSWorkspace=self._normWs,
                            OutputWorkspace=self._samWs)
            # additional reduction steps
            self._samSqwWs = self._group_and_SofQW(self._samWs, self._etBins,
                                                   isSample=True)
            if not self._debugMode:
                sapi.DeleteWorkspace(self._samWs)  # delete events file
            # Divide by Vanadium Q slice, if pertinent
            if self._normalizationType == "by Q slice":
                sapi.Divide(LHSWorkspace=self._samSqwWs,
                            RHSWorkspace=self._normWs,
                            OutputWorkspace=self._samSqwWs)
            # Clear mask from reduced file. Needed for binary operations
            # involving this S(Q,w)
            sapi.ClearMaskFlag(Workspace=self._samSqwWs)
            # Scale so that elastic line has Y-values ~ 1
            if self._normalizeToFirst:
                self._ScaleY(self._samSqwWs)

            # Transform the vertical axis (Q) to point data
            # Q-values are in X-axis now
            sapi.Transpose(InputWorkspace=self._samSqwWs,
                           OutputWorkspace=self._samSqwWs)
            # from histo to point
            sapi.ConvertToPointData(InputWorkspace=self._samSqwWs,
                                    OutputWorkspace=self._samSqwWs)
            # Q-values back to vertical axis
            sapi.Transpose(InputWorkspace=self._samSqwWs,
                           OutputWorkspace=self._samSqwWs)
            self.serialize_in_log(self._samSqwWs)  # store the call
            # Output Dave and Nexus files
            extension = "_divided.dat" if self._doNorm else ".dat"
            dave_grp_filename = self._makeRunName(self._samWsRun, False) +\
                extension
            sapi.SaveDaveGrp(Filename=dave_grp_filename,
                             InputWorkspace=self._samSqwWs,
                             ToMicroEV=True)
            extension = "_divided_sqw.nxs" if self._doNorm else "_sqw.nxs"
            processed_filename = self._makeRunName(self._samWsRun, False) +\
                extension
            sapi.SaveNexus(Filename=processed_filename,
                           InputWorkspace=self._samSqwWs)

            # additional output
            if self.getProperty("OutputSusceptibility").value:
                temperature = mtd[self._samSqwWs].getRun().\
                    getProperty(TEMPERATURE_SENSOR).getStatistics().mean
                samXqsWs = self._samSqwWs.replace("sqw", "Xqw")
                sapi.ApplyDetailedBalance(InputWorkspace=self._samSqwWs,
                                          OutputWorkspace=samXqsWs,
                                          Temperature=str(temperature))
                sapi.ConvertUnits(InputWorkspace=samXqsWs,
                                  OutputWorkspace=samXqsWs,
                                  Target="DeltaE_inFrequency",
                                  Emode="Indirect")
                self.serialize_in_log(samXqsWs)
                susceptibility_filename = processed_filename.replace("sqw", "Xqw")
                sapi.SaveNexus(Filename=susceptibility_filename,
                               InputWorkspace=samXqsWs)

        if not self._debugMode:
            sapi.DeleteWorkspace("BASIS_MASK")  # delete the mask
            if self._doNorm and bool(norm_runs):
                sapi.DeleteWorkspace("BASIS_NORM_MASK")  # delete vanadium mask
                sapi.DeleteWorkspace(self._normWs)  # Delete vanadium S(Q)
                if self._normalizationType == "by Q slice":
                    sapi.DeleteWorkspace(normWs)  # Delete vanadium events file
            if self.getProperty("ExcludeTimeSegment").value:
                sapi.DeleteWorkspace('splitter')
                [sapi.DeleteWorkspace(name) for name in
                 ('splitted_unfiltered', 'TOFCorrectWS') if
                 AnalysisDataService.doesExist(name)]

    def _getRuns(self, rlist, doIndiv=True):
        """
        Create sets of run numbers for analysis. A semicolon indicates a
        separate group of runs to be processed together.
        @param rlist: string containing all the run numbers to be reduced.
        @return if _doIndiv is False, return a list of IntArrayProperty
         objects. Each item is a pseudolist containing a set of runs to
         be reduced together. if _doIndiv is True, return a list of strings,
         each string is a run number.
        """
        run_list = []
        # ";" separates the runs into substrings. Each substring
        #  represents a set of runs
        rlvals = rlist.split(';')
        for rlval in rlvals:
            iap = IntArrayProperty("", rlval)  # split the substring
            if doIndiv:
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
        Make name like BSS_24234_event.nxs
        """
        return "{0}_{1}_event.nxs".format(self._short_inst,str(run))

    def _sumRuns(self, run_set, sam_ws, mon_ws, extra_ext=None):
        """
        Aggregate the set of runs
        @param run_set: list of run numbers
        @param sam_ws:  name of aggregate workspace for the sample
        @param mon_ws:  name of aggregate workspace for the monitors
        @param extra_ext: string to be added to the temporary workspaces
        """
        for run in run_set:
            ws_name = self._makeRunName(run)
            if extra_ext is not None:
                ws_name += extra_ext
            mon_ws_name = ws_name + "_monitors"
            run_file = self._makeRunFile(run)

            # Faster loading for the 311 reflection
            if self._reflection["name"] == "silicon311":
                kwargs = {"BankName": "bank2"}  # 311 analyzers only in bank2
            else:
                kwargs = {}

            sapi.LoadEventNexus(Filename=run_file,
                                OutputWorkspace=ws_name, **kwargs)
            if str(run)+':' in self.getProperty("ExcludeTimeSegment").value:
                self._filterEvents(str(run), ws_name)

            if self._MonNorm:
                sapi.LoadNexusMonitors(Filename=run_file,
                                       OutputWorkspace=mon_ws_name)

            if sam_ws != ws_name:
                sapi.Plus(LHSWorkspace=sam_ws,
                          RHSWorkspace=ws_name,
                          OutputWorkspace=sam_ws)
                sapi.DeleteWorkspace(ws_name)
            if mon_ws != mon_ws_name and self._MonNorm:
                sapi.Plus(LHSWorkspace=mon_ws,
                          RHSWorkspace=mon_ws_name,
                          OutputWorkspace=mon_ws)
                sapi.DeleteWorkspace(mon_ws_name)

    def _calibData(self, sam_ws, mon_ws):
        sapi.MaskDetectors(Workspace=sam_ws,
                           DetectorList=self._dMask)
        sapi.ModeratorTzeroLinear(InputWorkspace=sam_ws,
                                  OutputWorkspace=sam_ws)
        sapi.LoadParameterFile(Workspace=sam_ws,
                               Filename=pjoin(DEFAULT_CONFIG_DIR,
                                              self._reflection["parameter_file"]))
        sapi.ConvertUnits(InputWorkspace=sam_ws,
                          OutputWorkspace=sam_ws,
                          Target='Wavelength',
                          EMode='Indirect')

        if self._MonNorm:
            sapi.ModeratorTzeroLinear(InputWorkspace=mon_ws,
                                      OutputWorkspace=mon_ws)
            sapi.Rebin(InputWorkspace=mon_ws,
                       OutputWorkspace=mon_ws,
                       Params='10')
            sapi.ConvertUnits(InputWorkspace=mon_ws,
                              OutputWorkspace=mon_ws,
                              Target='Wavelength')
            sapi.OneMinusExponentialCor(InputWorkspace=mon_ws,
                                        OutputWorkspace=mon_ws,
                                        C='0.20749999999999999',
                                        C1='0.001276')
            sapi.Scale(InputWorkspace=mon_ws,
                       OutputWorkspace=mon_ws,
                       Factor='1e-06')
            sapi.RebinToWorkspace(WorkspaceToRebin=sam_ws,
                                  WorkspaceToMatch=mon_ws,
                                  OutputWorkspace=sam_ws)
            sapi.Divide(LHSWorkspace=sam_ws,
                        RHSWorkspace=mon_ws,
                        OutputWorkspace=sam_ws)

    def _sum_and_calibrate(self, run_set, extra_extension=""):
        """
        Aggregate the set of runs and calibrate
        @param run_set: list of run numbers
        @param extra_extension: string to be added to the workspace names
        @return: workspace name of the aggregated and calibrated data
        """
        wsName = self._makeRunName(run_set[0])
        wsName += extra_extension
        wsName_mon = wsName + "_monitors"
        self._sumRuns(run_set, wsName, wsName_mon, extra_extension)
        self._calibData(wsName, wsName_mon)
        if not self._debugMode:
            if self._MonNorm:
                sapi.DeleteWorkspace(wsName_mon)  # delete monitors
        return wsName

    def _group_and_SofQW(self, wsName, etRebins, isSample=True):
        """ Transforms from wavelength and detector ID to S(Q,E)
        @param wsName: workspace as a function of wavelength and detector id
        @param etRebins: final energy domain and bin width
        @param isSample: discriminates between sample and vanadium
        @return: string name of S(Q,E)
        """
        sapi.ConvertUnits(InputWorkspace=wsName,
                          OutputWorkspace=wsName,
                          Target='DeltaE',
                          EMode='Indirect')
        sapi.CorrectKiKf(InputWorkspace=wsName,
                         OutputWorkspace=wsName,
                         EMode='Indirect')
        sapi.Rebin(InputWorkspace=wsName,
                   OutputWorkspace=wsName,
                   Params=etRebins)
        if self._groupDetOpt != "None":
            if self._groupDetOpt == "Low-Resolution":
                grp_file = "BASIS_Grouping_LR.xml"
            else:
                grp_file = "BASIS_Grouping.xml"
            # If mask override used, we need to add default grouping file
            # location to search paths
            if self._overrideMask:
                config.appendDataSearchDir(DEFAULT_MASK_GROUP_DIR)
                sapi.GroupDetectors(InputWorkspace=wsName,
                                    OutputWorkspace=wsName,
                                    MapFile=grp_file,
                                    Behaviour="Sum")

        # Output NXSPE file (must be done before transforming the
        # vertical axis to point data)
        if isSample and self._nsxpe_do:
            extension = '.nxspe'
            run = mtd[wsName].getRun()
            if run.hasProperty(self._nxspe_psi_angle_log):
                psi_angle_logproperty = \
                    run.getProperty(self._nxspe_psi_angle_log)
                psi_angle = np.average(psi_angle_logproperty.value)
                psi_angle += self._nxspe_offset
                nxspe_filename = wsName + extension
                sapi.SaveNXSPE(InputWorkspace=wsName,
                               Filename=nxspe_filename,
                               Efixed=self._reflection['default_energy'],
                               Psi=psi_angle,
                               KiOverKfScaling=1)
            else:
                error_message = 'Runs have no log entry named {}'\
                    .format(self._nxspe_psi_angle_log)
                self.log().error(error_message)

        wsSqwName = wsName + '_divided_sqw' \
            if isSample and self._doNorm else wsName + '_sqw'
        sapi.SofQW3(InputWorkspace=wsName,
                    QAxisBinning=self._qBins,
                    EMode='Indirect',
                    EFixed=self._reflection["default_energy"],
                    OutputWorkspace=wsSqwName)
        # Rebin the vanadium within the elastic line
        if not isSample:
            sapi.Rebin(InputWorkspace=wsSqwName,
                       OutputWorkspace=wsSqwName,
                       Params=self._reflection["vanadium_bins"])
        return wsSqwName

    def _ScaleY(self, wsName):
        """
        Scale all spectra by a number so that the maximum of the first spectra
        is rescaled to 1
        @param wsName: name of the workspace to rescale
        """
        workspace = sapi.mtd[wsName]
        maximumYvalue = workspace.dataY(0).max()
        sapi.Scale(InputWorkspace=wsName,
                   OutputWorkspace=wsName,
                   Factor=1./maximumYvalue,
                   Operation="Multiply")

    def generateSplitterWorkspace(self, fragment):
        r"""Create a table workspace with time intervals to keep

        Parameters
        ----------
        fragment: str
            a-b  start and end of time fragment to filter out
        """
        inf = 86400  # a run a full day long
        a, b = fragment.split('-')
        b = inf if 'end' in b else float(b)
        a = float(a)
        splitter = sapi.CreateEmptyTableWorkspace(OutputWorkspace='splitter')
        splitter.addColumn('double', 'start')
        splitter.addColumn('double', 'stop')
        splitter.addColumn('str', 'target') #, 'str')
        if a == 0.0:
            splitter.addRow([b, inf, '0'])
        elif b == inf:
            splitter.addRow([0, a, '0'])
        else:
            splitter.addRow([0, a, '0'])
            splitter.addRow([b, inf, '0'])

    def _filterEvents(self, run, ws_name):
        r"""Filter out ExcludeTimeSegment if applicable

        Parameters
        ----------
        run: str
            run number
        ws_name : str
            name of the workspace to filter
        """
        for run_fragment in self.getProperty("ExcludeTimeSegment").value.split(';'):
            if run+':' in run_fragment:
                self.generateSplitterWorkspace(run_fragment.split(':')[1])
                sapi.FilterEvents(InputWorkspace=ws_name,
                                  SplitterWorkspace='splitter',
                                  OutputWorkspaceBaseName='splitted',
                                  GroupWorkspaces=True,
                                  OutputWorkspaceIndexedFrom1=True,
                                  RelativeTime=True)
                sapi.UnGroupWorkspace('splitted')
                sapi.RenameWorkspace(InputWorkspace='splitted_0',
                                     OutputWorkspace=ws_name)
                break

    def serialize_in_log(self, ws_name):
        r"""Save the serialization of the algorithm in the logs.

        Parameters
        ----------
        ws_name: str
            Name of the workspace from which to retrieve and modify the logs
        """
        def jsonify(value):
            r"""Cast non-standard objects to their closest standard
            representation to enable JSON serialiation"""
            if isinstance(value, np.ndarray):
                return value.tolist()
            return value
        if self._as_json is None:
            self._as_json = json.loads(str(self))
            # Force serialization of the following properties even if having
            # their default values
            forced = {name: jsonify(self.getProperty(name).value)
                      for name in ('DoIndividual', 'MonitorNorm',
                                   'NormalizeToFirst', 'ReflectionType',
                                   'EnergyBins', 'MomentumTransferBins',
                                   'MaskFile', 'DivideByVanadium')}
            self._as_json['properties'].update(forced)
        r = mtd[ws_name].mutableRun()
        r.addProperty('asString', json.dumps(self._as_json), True)

# Register algorithm with Mantid.
AlgorithmFactory.subscribe(BASISReduction)
