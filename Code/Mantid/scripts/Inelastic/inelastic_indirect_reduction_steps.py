#pylint: disable=invalid-name,no-init
from reduction.reducer import ReductionStep

import mantid
from mantid import config
from mantid.simpleapi import *
from mantid.api import IEventWorkspace

import string
import os


class LoadData(ReductionStep):
    """Handles the loading of the data for Indirect instruments. The summing
    of input workspaces is handled in this routine, as well as the identifying
    of detectors that require masking.

    This step will use the following parameters from the Instrument's parameter
    file:

    * Workflow.ChopDataIfGreaterThan - if this parameter is specified on the
        instrument, then the raw data will be split into multiple frames if
        the largest TOF (X) value in the workspace is greater than the provided
        value.
    """

    _multiple_frames = False
    _sum = False
    _load_logs = False
    _monitor_index = None
    _detector_range_start = None
    _detector_range_end = None
    _masking_detectors = []
    _parameter_file = None
    _data_files = {}
    _extra_load_opts = {}
    _contains_event_data = False

    def __init__(self):
        """Initialise the ReductionStep. Constructor should set the initial
        parameters for the step.
        """
        super(LoadData, self).__init__()
        self._sum = False
        self._load_logs = False
        self._multiple_frames = False
        self._monitor_index = None
        self._detector_range_start = None
        self._detector_range_end = None
        self._parameter_file = None
        self._data_files = {}

    def execute(self, reducer, file_ws):
        """Loads the data.
        """
        self._reducer = reducer
        wsname = ''

        for output_ws, filename in self._data_files.iteritems():
            try:
                self._load_single_file(filename,output_ws)
                if wsname == "":
                    wsname = output_ws
            except RuntimeError, exc:
                logger.warning("Error loading '%s': %s. File skipped" % (filename, str(exc)))
                continue

        if ( self._sum ) and ( len(self._data_files) > 1 ):
            ## Sum files
            merges = []
            if  self._multiple_frames :
                self._sum_chopped(wsname)
            else:
                self._sum_regular(wsname)
            ## Need to adjust the reducer's list of workspaces
            self._data_files = {}
            self._data_files[wsname] = wsname

    def set_load_logs(self, value):
        self._load_logs = value

    def set_sum(self, value):
        self._sum = value

    def set_parameter_file(self, value):
        self._parameter_file = value

    def set_detector_range(self, start, end):
        self._detector_range_start = start
        self._detector_range_end = end

    def set_extra_load_opts(self, opts):
        self._extra_load_opts = opts

    def set_ws_list(self, value):
        self._data_files = value

    def get_ws_list(self):
        return self._data_files

    def contains_event_data(self):
        return self._contains_event_data

    def _load_single_file(self, filename, output_ws):
        logger.notice("Loading file %s" % filename)

        self._load_data(filename, output_ws)

        if type(mtd[output_ws]) is IEventWorkspace:
            self._contains_event_data = True

        inst_name = mtd[output_ws].getInstrument().getName()
        if inst_name == 'BASIS':
            ModeratorTzeroLinear(InputWorkspace=output_ws,OutputWorkspace= output_ws)
            basis_mask = mtd[output_ws].getInstrument().getStringParameter(
                'Workflow.MaskFile')[0]
            # Quick hack for older BASIS files that only have one side
            #if (mtd[file].getRun()['run_number'] < 16693):
            #        basis_mask = "BASIS_Mask_before_16693.xml"
            basis_mask_filename = os.path.join(config.getString('maskFiles.directory')\
                    , basis_mask)
            if os.path.isfile(basis_mask_filename):
                LoadMask(Instrument="BASIS", OutputWorkspace="__basis_mask",\
                             InputFile=basis_mask_filename)
                MaskDetectors(Workspace=output_ws, MaskedWorkspace="__basis_mask")
            else:
                logger.notice("Couldn't find specified mask file : " + str(basis_mask_filename))

        if self._parameter_file != None:
            LoadParameterFile(Workspace=output_ws,Filename= self._parameter_file)

        self._monitor_index = self._reducer._get_monitor_index(mtd[output_ws])

        if self._require_chop_data(output_ws):
            ChopData(InputWorkspace=output_ws,OutputWorkspace= output_ws,Step= 20000.0,NChops= 5, IntegrationRangeLower=5000.0,\
                IntegrationRangeUpper=10000.0,\
                MonitorWorkspaceIndex=self._monitor_index)
            self._multiple_frames = True
        else:
            self._multiple_frames = False

        if  self._multiple_frames :
            workspaces = mtd[output_ws].getNames()
        else:
            workspaces = [output_ws]

        logger.debug('self._monitor_index = ' + str(self._monitor_index))

        for ws in workspaces:
            if isinstance(mtd[ws],mantid.api.IEventWorkspace):
                LoadNexusMonitors(Filename=self._data_files[output_ws],
                                  OutputWorkspace= ws+'_mon')
            else:
                ## Extract Monitor Spectrum
                ExtractSingleSpectrum(InputWorkspace=ws,OutputWorkspace= ws+'_mon',WorkspaceIndex= self._monitor_index)

                if self._detector_range_start < 0 or self._detector_range_end > mtd[ws].getNumberHistograms():
                    raise ValueError("Range %d - %d is not a valid detector range." % (self._detector_range_start, self._detector_range_end))

                ## Crop the workspace to remove uninteresting detectors
                CropWorkspace(InputWorkspace=ws,OutputWorkspace= ws,\
                    StartWorkspaceIndex=self._detector_range_start,\
                    EndWorkspaceIndex=self._detector_range_end)

    def _load_data(self, filename, output_ws):
        if self._parameter_file is not None and "VESUVIO" in self._parameter_file:
            loaded_ws = LoadVesuvio(Filename=filename, OutputWorkspace=output_ws, SpectrumList="1-198", **self._extra_load_opts)
        else:
            # loaded_ws = Load(Filename=filename, OutputWorkspace=output_ws, LoadLogFiles=False, **self._extra_load_opts)
            if self._load_logs == True:
                loaded_ws = Load(Filename=filename, OutputWorkspace=output_ws, LoadLogFiles=True, **self._extra_load_opts)
                logger.notice("Loaded sample logs")
            else:
                loaded_ws = Load(Filename=filename, OutputWorkspace=output_ws, LoadLogFiles=False, **self._extra_load_opts)

    def _sum_regular(self, wsname):
        merges = [[], []]
        run_numbers = []
        for ws in self._data_files:
            merges[0].append(ws)
            merges[1].append(ws + '_mon')
            run_numbers.append(str(mtd[ws].getRunNumber()))

        MergeRuns(InputWorkspaces=','.join(merges[0]), OutputWorkspace=wsname)
        MergeRuns(InputWorkspaces=','.join(merges[1]), OutputWorkspace=wsname + '_mon')

        AddSampleLog(Workspace=wsname, LogName='multi_run_numbers', LogType='String',
                     LogText=','.join(run_numbers))

        for n in range(1, len(merges[0])):
            DeleteWorkspace(Workspace=merges[0][n])
            DeleteWorkspace(Workspace=merges[1][n])

        factor = 1.0 / len(self._data_files)
        Scale(InputWorkspace=wsname, OutputWorkspace=wsname, Factor=factor)
        Scale(InputWorkspace=wsname + '_mon', OutputWorkspace=wsname + '_mon', Factor=factor)

    def _sum_chopped(self, wsname):
        merges = []
        nmerges = len(mtd[wsname].getNames())

        for n in range(0, nmerges):
            merges.append([])
            merges.append([])

            for file in self._data_files:
                try:
                    merges[2 * n].append(mtd[file].getNames()[n])
                    merges[2 * n + 1].append(mtd[file].getNames()[n] + '_mon')
                except AttributeError:
                    if n == 0:
                        merges[0].append(file)
                        merges[1].append(file + '_mon')

        for merge in merges:
            MergeRuns(InputWorkspaces=','.join(merge), OutputWorkspace=merge[0])
            factor = 1.0 / len(merge)
            Scale(InputWorkspace=merge[0], OutputWorkspace=merge[0], Factor=factor)

            for n in range(1, len(merge)):
                DeleteWorkspace(Workspace=merge[n])

    def _require_chop_data(self, ws):
        try:
            cdigt = mtd[ws].getInstrument().getNumberParameter(
                'Workflow.ChopDataIfGreaterThan')[0]
        except IndexError:
            return False
        if  mtd[ws].readX(0)[mtd[ws].blocksize()] > cdigt :
            return True
        else:
            return False

    def is_multiple_frames(self):
        return self._multiple_frames

#--------------------------------------------------------------------------------------------------

class IdentifyBadDetectors(ReductionStep):
    """ Identifies bad detectors in a workspace and creates a list of
    detectors to mask. This step will set the masking detectors property on
    the reducer object passed to execute. This uses the IdentifyNoisyDetectors algorithm.

    The step will use the following parameters on the workspace:

    * Workflow.Masking - identifies the method (if any) on which detectors that
        are to be masked should be identified.
    """

    _masking_detectors = []

    def __init__(self, MultipleFrames=False):
        super(IdentifyBadDetectors, self).__init__()
        self._multiple_frames = MultipleFrames
        self._background_start = None
        self._background_end = None

    def execute(self, reducer, file_ws):

        if self._multiple_frames:
            try:
                workspaces = mtd[file_ws].getNames()
            except AttributeError:
                workspaces = [file_ws]
        else:
            workspaces = [file_ws]

        try:
            msk = mtd[workspaces[0]].getInstrument().getStringParameter('Workflow.Masking')[0]
        except IndexError:
            msk = 'None'

        if msk != 'IdentifyNoisyDetectors':
            return

        temp_ws_mask = '__temp_ws_mask'
        IdentifyNoisyDetectors(InputWorkspace=workspaces[0], OutputWorkspace=temp_ws_mask)
        ws = mtd[temp_ws_mask]
        nhist = ws.getNumberHistograms()

        for i in range(0, nhist):
            if ws.readY(i)[0] == 0.0:
                self._masking_detectors.append(i)
        DeleteWorkspace(Workspace=temp_ws_mask)

        #set the detector masks for the workspace
        reducer._masking_detectors[file_ws] = self._masking_detectors

    def get_mask_list(self):
        return self._masking_detectors

#--------------------------------------------------------------------------------------------------

class BackgroundOperations(ReductionStep):
    """Removes, if requested, a background from the detectors data in TOF
    units. Currently only uses the CalculateFlatBackground algorithm, more options
    to cover SNS use to be added at a later point.
    """
    _multiple_frames = False
    _background_start = None
    _background_end = None

    def __init__(self, MultipleFrames=False):
        super(BackgroundOperations, self).__init__()
        self._multiple_frames = MultipleFrames
        self._background_start = None
        self._background_end = None

    def execute(self, reducer, file_ws):
        if  self._multiple_frames :
            try:
                workspaces = mtd[file_ws].getNames()
            except AttributeError:
                workspaces = [file_ws]
        else:
            workspaces = [file_ws]

        for ws in workspaces:
            ConvertToDistribution(Workspace=ws)
            CalculateFlatBackground(InputWorkspace=ws,OutputWorkspace= ws,StartX= self._background_start,\
                           EndX=self._background_end, Mode='Mean')
            ConvertFromDistribution(Workspace=ws)

    def set_range(self, start, end):
        self._background_start = start
        self._background_end = end

class ApplyCalibration(ReductionStep):
    """Applies a calibration workspace to the data.
    """

    _multiple_frames = False
    _calib_workspace = None

    def __init__(self):
        super(ApplyCalibration, self).__init__()
        self._multiple_frames = False
        self._calib_workspace = None

    def execute(self, reducer, file_ws):
        if self._calib_workspace is None: # No calibration workspace set
            return
        if  self._multiple_frames :
            try:
                workspaces = mtd[file_ws].getNames()
            except AttributeError:
                workspaces = [file_ws]
        else:
            workspaces = [file_ws]

        for ws in workspaces:
            Divide(LHSWorkspace=ws,RHSWorkspace= self._calib_workspace,OutputWorkspace= ws)

    def set_is_multiple_frames(self, value):
        self._multiple_frames = value

    def set_calib_workspace(self, value):
        self._calib_workspace = value

class HandleMonitor(ReductionStep):
    """Handles the montior for the reduction of inelastic indirect data.

    This uses the following parameters from the instrument:
    * Workflow.Monitor1-Area
    * Workflow.Monitor1-Thickness
    * Workflow.Monitor1-ScalingFactor
    * Workflow.UnwrapMonitor
    """
    _multiple_frames = False

    def __init__(self, MultipleFrames=False):
        """Constructor for HandleMonitor routine.
        """
        super(HandleMonitor, self).__init__()
        self._multiple_frames = MultipleFrames

    def execute(self, reducer, file_ws):
        """Does everything we want to with the Monitor.
        """
        if  self._multiple_frames :
            try:
                workspaces = mtd[file_ws].getNames()
            except AttributeError:
                workspaces = [file_ws]
        else:
            workspaces = [file_ws]

        for ws in workspaces:
            monitor = ws+'_mon'
            self._rebin_monitor(ws)
            if self._need_to_unwrap(ws):
                self._unwrap_monitor(ws)
            else:
                ConvertUnits(InputWorkspace=monitor,OutputWorkspace= monitor,Target= 'Wavelength')
            self._monitor_efficiency(monitor)
            self._scale_monitor(monitor)

    def _rebin_monitor(self, ws):
        """For some instruments (e.g. BASIS) the monitor binning is too
    fine and needs to be rebinned. This is controlled
        by the 'Workflow.Monitor.RebinStep' parameter set on the
        instrument.  If no parameter is present, no rebinning will occur.
        """
        try:
            stepsize = mtd[ws].getInstrument().getNumberParameter('Workflow.Monitor.RebinStep')[0]
        except IndexError:
            logger.notice("Monitor is not being rebinned.")
        else:
            Rebin(InputWorkspace=ws+'_mon',OutputWorkspace= ws+'_mon',Params= stepsize)

    def _need_to_unwrap(self, ws):
        try:
            unwrap = mtd[ws].getInstrument().getStringParameter(
                'Workflow.UnwrapMonitor')[0]
        except IndexError:
            return False # Default it to not unwrap
        if  unwrap == 'Never' :
            return False
        elif  unwrap == 'Always' :
            return True
        elif  unwrap == 'BaseOnTimeRegime' :
            SpecMon = mtd[ws+'_mon'].readX(0)[0]
            SpecDet = mtd[ws].readX(0)[0]
            if  SpecMon == SpecDet :
                return True
            else:
                return False
        else:
            return False

    def _unwrap_monitor(self, ws):
        l_ref = self._get_reference_length(ws, 0)
        monitor = ws+'_mon'
        unwrapped_ws, join = UnwrapMonitor(InputWorkspace=monitor, OutputWorkspace=monitor, LRef=l_ref)
        RemoveBins(InputWorkspace=monitor,OutputWorkspace= monitor,XMin= join-0.001,XMax= join+0.001,\
            Interpolation='Linear')
        try:
            FFTSmooth(InputWorkspace=monitor,OutputWorkspace=monitor,WorkspaceIndex=0)
        except ValueError:
            raise ValueError("Indirect Energy Conversion does not support uneven bin widths.")

    def _get_reference_length(self, ws, index):
        workspace = mtd[ws]
        instrument = workspace.getInstrument()
        sample = instrument.getSample()
        source = instrument.getSource()
        detector = workspace.getDetector(index)
        sample_to_source = sample.getPos() - source.getPos()
        r = detector.getDistance(sample)
        x = sample_to_source.getZ()
        result = x + r
        return result

    def _monitor_efficiency(self, monitor):
        inst = mtd[monitor].getInstrument()
        try:
            montiorStr = 'Workflow.Monitor1'
            area = inst.getNumberParameter(montiorStr+'-Area')[0]
            thickness = inst.getNumberParameter(montiorStr+'-Thickness')[0]
            attenuation= inst.getNumberParameter(montiorStr+'-Attenuation')[0]
        except IndexError:
            raise ValueError('Unable to retrieve monitor thickness, area and '\
                'attenuation from Instrument Parameter file.')
        else:
            if  area == -1 or thickness == -1 or attenuation == -1:
                return
            OneMinusExponentialCor(InputWorkspace=monitor,OutputWorkspace= monitor,C= (attenuation * thickness),C1= area)

    def _scale_monitor(self, monitor):
        """Some instruments wish to scale their data. Doing this at the
        monitor is the most efficient way to do this. This is controlled
        by the 'Workflow.MonitorScalingFactor' parameter set on the
        instrument.
        """
        try:
            factor = mtd[monitor].getInstrument().getNumberParameter(
                'Workflow.Monitor1-ScalingFactor')[0]
        except IndexError:
            print "Monitor is not being scaled."
        else:
            if factor != 1.0:
                Scale(InputWorkspace=monitor,OutputWorkspace= monitor,Factor= ( 1.0 / factor ),Operation= 'Multiply')

class CorrectByMonitor(ReductionStep):
    """
    """

    _multiple_frames = False
    _emode = "Indirect"

    def __init__(self, MultipleFrames=False, EMode="Indirect"):
        super(CorrectByMonitor, self).__init__()
        self._multiple_frames = MultipleFrames
        self._emode = EMode

    def execute(self, reducer, file_ws):
        if  self._multiple_frames :
            try:
                workspaces = mtd[file_ws].getNames()
            except AttributeError:
                workspaces = [file_ws]
        else:
            workspaces = [file_ws]

        for ws in workspaces:
            ConvertUnits(InputWorkspace=ws,OutputWorkspace= ws,Target= "Wavelength",EMode= self._emode)
            RebinToWorkspace(WorkspaceToRebin=ws,WorkspaceToMatch= ws+'_mon',OutputWorkspace= ws)
            Divide(LHSWorkspace=ws,RHSWorkspace= ws+'_mon',OutputWorkspace= ws)
            DeleteWorkspace(Workspace=ws+'_mon')

    def set_emode(self, emode):
        """
        """
        self._emode = emode

class FoldData(ReductionStep):
    _result_workspaces = []

    def __init__(self):
        super(FoldData, self).__init__()
        self._result_workspaces = []

    def execute(self, reducer, file_ws):
        try:
            wsgroup = mtd[file_ws].getNames()
        except AttributeError:
            return # Not a grouped workspace
        ws = file_ws+'_merged'
        MergeRuns(InputWorkspaces=','.join(wsgroup),OutputWorkspace= ws)
        scaling = self._create_scaling_workspace(wsgroup, ws)
        for workspace in wsgroup:
            DeleteWorkspace(Workspace=workspace)
        Divide(LHSWorkspace=ws,RHSWorkspace= scaling,OutputWorkspace= ws)
        DeleteWorkspace(Workspace=scaling)
        RenameWorkspace(InputWorkspace=ws,OutputWorkspace= file_ws)
        self._result_workspaces.append(file_ws)

    def get_result_workspaces(self):
        return self._result_workspaces

    def _create_scaling_workspace(self, wsgroup, merged):
        wsname = '__scaling'
        unit = ''
        ranges = []
        lowest = 0
        highest = 0
        for ws in wsgroup:
            if  unit == '' :
                unit = mtd[ws].getAxis(0).getUnit().unitID()
            low = mtd[ws].dataX(0)[0]
            high = mtd[ws].dataX(0)[mtd[ws].blocksize()-1]
            ranges.append([low, high])
            if low < lowest: lowest = low
            if high > highest: highest = high
        dataX = mtd[merged].readX(0)
        dataY = []
        dataE = []
        for i in range(0, mtd[merged].blocksize()):
            dataE.append(0.0)
            dataY.append(self._ws_in_range(ranges, dataX[i]))
        CreateWorkspace(OutputWorkspace=wsname,DataX= dataX,DataY= dataY,DataE= dataE, UnitX=unit)
        return wsname

    def _ws_in_range(self, ranges, xval):
        result = 0
        for range in ranges:
            if  xval >= range[0] and xval <= range[1] : result += 1
        return result

class ConvertToCm1(ReductionStep):
    """
    Converts the workspaces to cm-1.
    """

    _multiple_frames = False
    _save_to_cm_1 = False

    def __init__(self, MultipleFrames=False):
        super(ConvertToCm1, self).__init__()
        self._multiple_frames = MultipleFrames

    def execute(self, reducer, file_ws):

        if self._save_to_cm_1 == False:
            return

        if  self._multiple_frames :
            try:
                workspaceNames = mtd[file_ws].getNames()
            except AttributeError:
                workspaceNames = [file_ws]
        else:
            workspaceNames = [file_ws]

        for wsName in workspaceNames:
            try:
                ws = mtd[wsName]
            except:
                continue
            ConvertUnits(InputWorkspace=ws,OutputWorkspace=ws,EMode='Indirect',Target='DeltaE_inWavenumber')

    def set_save_to_cm_1(self, save_to_cm_1):
        self._save_to_cm_1 = save_to_cm_1

class ConvertToEnergy(ReductionStep):
    """
    """
    _rebin_string = None
    _multiple_frames = False

    def __init__(self, MultipleFrames=False):
        super(ConvertToEnergy, self).__init__()
        self._rebin_string = None
        self._multiple_frames = MultipleFrames

    def execute(self, reducer, file_ws):
        if  self._multiple_frames :
            try:
                workspaces = mtd[file_ws].getNames()
            except AttributeError:
                workspaces = [file_ws]
        else:
            workspaces = [file_ws]

        for ws in workspaces:
            ConvertUnits(InputWorkspace=ws,OutputWorkspace= ws,Target= 'DeltaE',EMode= 'Indirect')
            CorrectKiKf(InputWorkspace=ws,OutputWorkspace= ws,EMode= 'Indirect')
            if self._rebin_string is not None:
                if not self._multiple_frames:
                    Rebin(InputWorkspace=ws,OutputWorkspace= ws,Params= self._rebin_string)
            else:
                try:
                    # Rebin whole workspace to first spectrum to allow grouping to proceed
                    RebinToWorkspace(WorkspaceToRebin=ws,WorkspaceToMatch=ws,
                                     OutputWorkspace=ws)
                except Exception:
                    logger.information("RebinToWorkspace failed. Attempting to continue without it.")

        if self._multiple_frames and self._rebin_string is not None:
            self._rebin_mf(workspaces)

    def set_rebin_string(self, value):
        if value is not None:
            self._rebin_string = value

    def _rebin_mf(self, workspaces):
        nbin = 0
        rstwo = self._rebin_string.split(",")
        if len(rstwo) >= 5:
            rstwo = ",".join(rstwo[2:])
        else:
            rstwo = self._rebin_string
        for ws in workspaces:
            nbins = mtd[ws].blocksize()
            if nbins > nbin: nbin = nbins
        for ws in workspaces:
            if mtd[ws].blocksize() == nbin:
                Rebin(InputWorkspace=ws,OutputWorkspace= ws,Params= self._rebin_string)
            else:
                Rebin(InputWorkspace=ws,OutputWorkspace= ws,Params= rstwo)

class RebinToFirstSpectrum(ReductionStep):
    """
        A simple step to rebin the input workspace to match
        the first spectrum of itself
    """

    def execute(self, reducer, inputworkspace):
        RebinToWorkspace(WorkspaceToRebin=inputworkspace,WorkspaceToMatch=inputworkspace,
                         OutputWorkspace=inputworkspace)

class NormaliseToUnityStep(ReductionStep):
    """
        A simple step to normalise a workspace to a given factor
    """
    _factor = None
    _peak_min = None
    _peak_max = None

    def execute(self, reducer, ws):
        number_historgrams = mtd[ws].getNumberHistograms()
        Integration(InputWorkspace=ws, OutputWorkspace=ws, RangeLower=self._peak_min, RangeUpper= self._peak_max)
        ws_mask, num_zero_spectra = FindDetectorsOutsideLimits(InputWorkspace=ws, OutputWorkspace='__temp_ws_mask')
        DeleteWorkspace(ws_mask)

        tempSum = SumSpectra(InputWorkspace=ws, OutputWorkspace='__temp_sum')
        total = tempSum.readY(0)[0]
        DeleteWorkspace(tempSum)

        if self._factor is None:
            self._factor = 1 / ( total / (number_historgrams - num_zero_spectra) )

        Scale(InputWorkspace=ws, OutputWorkspace=ws, Factor=self._factor, Operation='Multiply')

    def set_factor(self, factor):
        self._factor = factor

    def set_peak_range(self, pmin, pmax):
        self._peak_min = pmin
        self._peak_max = pmax

class DetailedBalance(ReductionStep):
    """
    """
    _temp = None
    _multiple_frames = False

    def __init__(self, MultipleFrames=False):
        super(DetailedBalance, self).__init__()
        self._temp = None
        self._multiple_frames = MultipleFrames

    def execute(self, reducer, file_ws):
        if self._temp is None:
            return

        correction = 11.606 / ( 2 * self._temp )

        if  self._multiple_frames :
            workspaces = mtd[file_ws].getNames()
        else:
            workspaces = [file_ws]

        for ws in workspaces:
            ExponentialCorrection(InputWorkspace=ws,OutputWorkspace= ws,C0= 1.0,C1= correction, Operation="Multiply")

    def set_temperature(self, temp):
        self._temp = temp

class Scaling(ReductionStep):
    """
    """
    _scale_factor = None
    _multiple_frames = False

    def __init__(self, MultipleFrames=False):
        super(Scaling, self).__init__()
        self._scale_factor = None
        self._multiple_frames = MultipleFrames

    def execute(self, reducer, file_ws):
        if self._scale_factor is None: # Scale factor is the default value, 1.0
            return

        if  self._multiple_frames :
            workspaces = mtd[file_ws].getNames()
        else:
            workspaces = [file_ws]

        for ws in workspaces:
            Scale(InputWorkspace=ws,OutputWorkspace= ws,Factor= self._scale_factor, Operation="Multiply")

    def set_scale_factor(self, scaleFactor):
        self._scale_factor = scaleFactor

class Grouping(ReductionStep):
    """This ReductionStep handles the grouping and renaming of the final
    workspace. In most cases, this will require a Rebin on the data. The option
    to do this is given in the ConvertToEnergy step.

    The step will use the following parameters on the workspace:
    * 'Workflow.GroupingMethod' - if this is equal to 'File' then we look for a
        parameter called:
    * 'Workflow.GroupingFile' - the name of a file which contains the grouping of
        detectors for the instrument.

    If a masking list has been set using set_mask_list(), then the workspace
    indices listed will not be included in the group (if any grouping is in
    fact performed).
    """
    _grouping_policy = None
    _masking_detectors = []
    _result_workspaces = []
    _multiple_frames = False

    def __init__(self, MultipleFrames=False):
        super(Grouping, self).__init__()
        self._grouping_policy = None
        self._masking_detectors = []
        self._result_workspaces = []
        self._multiple_frames = MultipleFrames

    def execute(self, reducer, file_ws):

        if self._multiple_frames:
            try:
                workspaces = mtd[file_ws].getNames()
            except AttributeError:
                workspaces = [file_ws]
        else:
            workspaces = [file_ws]

        # Set the detector mask for this workspace
        if file_ws in reducer._masking_detectors:
            self._masking_detectors = reducer._masking_detectors[file_ws]

        for ws in workspaces:
            # If a grouping policy has not been set then try to get one from the IPF
            if self._grouping_policy is None:
                try:
                    group = mtd[ws].getInstrument().getStringParameter('Workflow.GroupingMethod')[0]
                except IndexError:
                    group = 'User'

                if group == 'File':
                    self._grouping_policy = mtd[ws].getInstrument().getStringParameter('Workflow.GroupingFile')[0]
                else:
                    self._grouping_policy = group

            self._result_workspaces.append(self._group_data(ws))

    def set_grouping_policy(self, value):
        self._grouping_policy = value

    def get_result_workspaces(self):
        return self._result_workspaces

    def _group_data(self, workspace):
        grouping = self._grouping_policy
        if grouping == 'Individual' or grouping is None:
            return workspace
        elif grouping == 'All':
            nhist = mtd[workspace].getNumberHistograms()
            wslist = []
            for i in range(0, nhist):
                if i not in self._masking_detectors:
                    wslist.append(i)
            GroupDetectors(InputWorkspace=workspace, OutputWorkspace=workspace,
                           WorkspaceIndexList=wslist, Behaviour='Average')
        else:
            # We may have either a workspace name or a mapping file name here
            grouping_workspace = None
            grouping_filename = None

            # See if it a workspace in ADS
            # If not assume it is a mapping file
            try:
                grouping_workspace = mtd[grouping]
            except KeyError:
                logger.notice("Cannot find group workspace " + grouping + ", attempting to find as file")

                # See if it is an absolute path
                # Otherwise check in the default group files directory
                if os.path.isfile(grouping):
                    grouping_filename = grouping
                else:
                    grouping_filename = os.path.join(config.getString('groupingFiles.directory'), grouping)

            # Mask detectors before grouping if we need to
            if len(self._masking_detectors) > 0:
                MaskDetectors(workspace, WorkspaceIndexList=self._masking_detectors)

            # Run GroupDetectors with a workspace if we have one
            # Otherwise try to run it with a mapping file
            if grouping_workspace is not None:
                GroupDetectors(InputWorkspace=workspace, OutputWorkspace=workspace, CopyGroupingFromWorkspace=grouping_workspace,\
                        Behaviour='Average')
            elif os.path.isfile(grouping_filename):
                GroupDetectors(InputWorkspace=workspace, OutputWorkspace=workspace, MapFile=grouping_filename,\
                        Behaviour='Average')

        return workspace

class SaveItem(ReductionStep):
    """This routine will save a given workspace in the selected file formats.
    The currently recognised formats are:
        * 'spe' - SPE ASCII format
        * 'nxs' - NeXus compressed file format
        * 'nxspe' - NeXus SPE file format
        * 'ascii' - Comma Seperated Values (file extension '.dat')
        * 'gss' - GSAS file format (N.B.: units will be converted to Time of
            Flight if not already in that unit for saving in this format).
        * 'davegrp' - DAVE grouped ASCII format
    """
    _formats = []
    _save_to_cm_1 = False

    def __init__(self):
        super(SaveItem, self).__init__()
        self._formats = []

    def execute(self, reducer, file_ws):
        naming = Naming()
        filename = naming.get_ws_name(file_ws, reducer)
        for format in self._formats:
            if format == 'spe':
                SaveSPE(InputWorkspace=file_ws, Filename=filename + '.spe')
            elif format == 'nxs':
                SaveNexusProcessed(InputWorkspace=file_ws, Filename=filename + '.nxs')
            elif format == 'nxspe':
                SaveNXSPE(InputWorkspace=file_ws, Filename=filename + '.nxspe')
            elif format == 'ascii':
                # Version 1 of SaveASCII produces output that works better with excel/origin
                # For some reason this has to be done with an algorithm object, using the function
                # wrapper with Version did not change the version that was run
                saveAsciiAlg = mantid.api.AlgorithmManager.createUnmanaged('SaveAscii', 1)
                saveAsciiAlg.initialize()
                saveAsciiAlg.setProperty('InputWorkspace', file_ws)
                saveAsciiAlg.setProperty('Filename', filename + '.dat')
                saveAsciiAlg.execute()

            elif format == 'gss':
                ConvertUnits(InputWorkspace=file_ws, OutputWorkspace="__save_item_temp", Target="TOF")
                SaveGSS(InputWorkspace="__save_item_temp", Filename=filename + ".gss")
                DeleteWorkspace(Workspace="__save_item_temp")
            elif format == 'aclimax':
                if self._save_to_cm_1 == False:
                    bins = '3, -0.005, 500' #meV
                else:
                    bins = '24, -0.005, 4000' #cm-1
                Rebin(InputWorkspace=file_ws,OutputWorkspace= file_ws + '_aclimax_save_temp', Params=bins)
                SaveAscii(InputWorkspace=file_ws + '_aclimax_save_temp', Filename=filename + '_aclimax.dat', Separator='Tab')
                DeleteWorkspace(Workspace=file_ws + '_aclimax_save_temp')
            elif format == 'davegrp':
                ConvertSpectrumAxis(InputWorkspace=file_ws, OutputWorkspace=file_ws + '_davegrp_save_temp', Target='ElasticQ', EMode='Indirect')
                SaveDaveGrp(InputWorkspace=file_ws + '_davegrp_save_temp', Filename=filename + '.grp')
                DeleteWorkspace(Workspace=file_ws + '_davegrp_save_temp')

    def set_formats(self, formats):
        self._formats = formats

    def set_save_to_cm_1(self, save_to_cm_1):
        self._save_to_cm_1 = save_to_cm_1

class Naming(ReductionStep):
    """Takes the responsibility of naming the results away from the Grouping
    step so that ws names are consistent right up until the last step. This
    uses the following instrument parameters:
    * 'Workflow.NamingConvention' - to decide how to name the result workspace.
        The default (when nothing is selected) is to use the run title.
    """
    _result_workspaces = []

    def __init__(self):
        super(Naming, self).__init__()
        self._result_workspaces = []
        self._multi_run = False

    def execute(self, reducer, file_ws):
        self._multi_run = reducer._sum
        wsname = self._get_ws_name(file_ws)
        RenameWorkspace(InputWorkspace=file_ws, OutputWorkspace=wsname)
        self._result_workspaces.append(wsname)

    def get_result_workspaces(self):
        return self._result_workspaces

    def get_ws_name(self, workspace, reducer):
        self._multi_run = reducer._sum
        return self._get_ws_name(workspace)

    def _get_ws_name(self, workspace):
        try:
            type = mtd[workspace].getInstrument().getStringParameter(
                'Workflow.NamingConvention')[0]
        except IndexError:
            type = 'RunTitle'

        if type == 'AnalyserReflection':
            return self._analyser_reflection(workspace)
        elif type == 'RunTitle':
            return self._run_title(workspace)
        else:
            raise NotImplementedError('Unknown \'Workflow.NamingConvention\''\
                ' parameter encountered on workspace: ' + workspace)

    def _run_title(self, workspace):
        ws = mtd[workspace]
        title = ws.getRun()['run_title'].value.strip()
        runNo = ws.getRun()['run_number'].value
        if self._multi_run:
            runNo += '_multi'
        inst = ws.getInstrument().getName()
        isn = config.getFacility().instrument(inst).shortName().upper()
        valid = "-_.() %s%s" % (string.ascii_letters, string.digits)
        title = ''.join(ch for ch in title if ch in valid)
        title = isn + runNo + '-' + title
        return title

    def _analyser_reflection(self, workspace):
        if workspace == '':
            return ''
        ws = mtd[workspace]
        inst = ws.getInstrument().getName()

        short_name = ''
        try:
            short_name = config.getFacility().instrument(inst).shortName().lower()
        except RuntimeError:
            for facility in config.getFacilities():
                try:
                    short_name = facility.instrument(inst).shortName().lower()
                except RuntimeError:
                    pass

        if short_name == '':
            raise RuntimeError('Cannot find instrument "%s" in any facility' % str(inst))

        run = ws.getRun().getLogData('run_number').value
        if self._multi_run:
            run += '_multi'
        try:
            analyser = ws.getInstrument().getStringParameter('analyser')[0]
            reflection = ws.getInstrument().getStringParameter('reflection')[0]
        except IndexError:
            analyser = ''
            reflection = ''
        prefix = short_name + run + '_' + analyser + reflection + '_red'
        return prefix
