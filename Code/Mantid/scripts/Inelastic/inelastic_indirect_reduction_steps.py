from reducer import ReductionStep
from mantidsimple import *

class LoadData(ReductionStep):
    """Handles the loading of the data for Indirect instruments. The summing
    of input workspaces is handled in this routine, as well as the identifying
    of detectors that require masking.
    
    This step will use the following parameters from the Instrument's parameter
    file:
    
    * Workflow.Masking - identifies the method (if any) on which detectors that
        are to be masked should be identified.
    * Workflow.ChopDataIfGreaterThan - if this parameter is specified on the
        instrument, then the raw data will be split into multiple frames if
        the largest TOF (X) value in the workspace is greater than the provided
        value.
    """

    _multiple_frames = False
    _sum = False
    _monitor_index = None
    _detector_range_start = None
    _detector_range_end = None
    _masking_detectors = []
    _parameter_file = None
    _data_files = {}

    def __init__(self):
        """Initialise the ReductionStep. Constructor should set the initial
        parameters for the step.
        """
        super(LoadData, self).__init__()
        self._sum = False
        self._multiple_frames = False
        self._monitor_index = None
        self._detector_range_start = None
        self._detector_range_end = None
        self._parameter_file = None
        self._data_files = {}

    def execute(self, reducer, file_ws):
        """Loads the data.
        """
        wsname = ''

        for file in self._data_files:
            Load(file, file, LoadLogFiles=False)

            if self._parameter_file != None:
                LoadParameterFile(file, self._parameter_file)

            try:
                msk = mtd[file].getInstrument().getStringParameter(
                    'Workflow.Masking')[0]
            except IndexError:
                msk = 'None'
            if ( msk == 'IdentifyNoisyDetectors' ):
                self._identify_bad_detectors(file)

            if ( wsname == '' ):
                wsname = file

            if self._require_chop_data(file):
                ChopData(file, file, 20000.0, 5, IntegrationRangeLower=5000.0,
                    IntegrationRangeUpper=10000.0, 
                    MonitorWorkspaceIndex=self._monitor_index)
                self._multiple_frames = True
            else:
                self._multiple_frames = False

            if ( self._multiple_frames ):
                workspaces = mtd[file].getNames()
            else:
                workspaces = [file]

            for ws in workspaces:
                ## Extract Monitor Spectrum
                ExtractSingleSpectrum(ws, ws+'_mon', self._monitor_index)
                ## Crop the workspace to remove uninteresting detectors
                CropWorkspace(ws, ws, 
                    StartWorkspaceIndex=self._detector_range_start,
                    EndWorkspaceIndex=self._detector_range_end)

        if ( self._sum ) and ( len(self._data_files) > 1 ):
            ## Sum files
            merges = []
            if ( self._multiple_frames ):
                self._sum_chopped(wsname)
            else:
                self._sum_regular(wsname)
            ## Need to adjust the reducer's list of workspaces
            self._data_files = {}
            self._data_files[wsname] = wsname

    def set_sum(self, value):
        self._sum = value

    def set_parameter_file(self, value):
        self._parameter_file = value

    def set_monitor_index(self, index):
        self._monitor_index = index

    def set_detector_range(self, start, end):
        self._detector_range_start = start
        self._detector_range_end = end

    def get_mask_list(self):
        return self._masking_detectors

    def set_ws_list(self, value):
        self._data_files = value

    def get_ws_list(self):
        return self._data_files

    def _sum_regular(self, wsname):
        merges = [[], []]
        for ws in self._data_files:
            merges[0].append(ws)
            merges[1].append(ws+'_mon')
        MergeRuns(','.join(merges[0]), wsname)
        MergeRuns(','.join(merges[1]), wsname+'_mon')
        for n in range(1, len(merges[0])):
            DeleteWorkspace(merges[0][n])
            DeleteWorkspace(merges[1][n])
        factor = 1.0 / len(self._data_files)
        Scale(wsname, wsname, factor)
        Scale(wsname+'_mon', wsname+'_mon', factor)

    def _sum_chopped(self, wsname):
        merges = []
        nmerges = len(mtd[wsname].getNames())
        for n in range(0, nmerges):
            merges.append([])
            merges.append([])
            for file in self._data_files:
                merges[2*n].append(mtd[file].getNames()[n])
                merges[2*n+1].append(mtd[file].getNames()[n]+'_mon')
        for merge in merges:
            MergeRuns(','.join(merge), merge[0])
            factor = 1.0 / len(merge)
            Scale(merge[0], merge[0], factor)
            for n in range(1, len(merge)):
                DeleteWorkspace(merge[n])

    def _identify_bad_detectors(self, workspace):
        IdentifyNoisyDetectors(workspace, '__temp_tsc_noise')
        ws = mtd['__temp_tsc_noise']
        nhist = ws.getNumberHistograms()
        self._masking_detectors = []
        for i in range(0, nhist):
            if ( ws.readY(i)[0] == 0.0 ):
                self._masking_detectors.append(i)
        DeleteWorkspace('__temp_tsc_noise')
        return self._masking_detectors

    def _require_chop_data(self, ws):
        try:
            cdigt = mtd[ws].getInstrument().getNumberParameter(
                'Workflow.ChopDataIfGreaterThan')[0]
        except IndexError:
            return False
        if ( mtd[ws].readX(0)[mtd[ws].getNumberBins()] > cdigt ):
            return True
        else:
            return False

    def is_multiple_frames(self):
        return self._multiple_frames

class BackgroundOperations(ReductionStep):
    """Removes, if requested, a background from the detectors data in TOF
    units. Currently only uses the FlatBackground algorithm, more options
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
        if ( self._multiple_frames ):
            workspaces = mtd[file_ws].getNames()
        else:
            workspaces = [file_ws]

        for ws in workspaces:
            ConvertToDistribution(ws)
            FlatBackground(ws, ws, self._background_start, 
                self._background_end, Mode='Mean')
            ConvertFromDistribution(ws)

    def set_range(self, start, end):
        self._background_start = start
        self._background_end = end

class CreateCalibrationWorkspace(ReductionStep):
    """Creates a calibration workspace from a White-Beam Vanadium run.
    """

    _back_min = None
    _back_max = None
    _peak_min = None
    _peak_max = None
    _detector_range_start = None
    _detector_range_end = None
    _calib_raw_files = []
    _calib_workspace = None

    def __init__(self):
        super(CreateCalibrationWorkspace, self).__init__()
        self._back_min = None
        self._back_max = None
        self._peak_min = None
        self._peak_max = None
        self._detector_range_start = None
        self._detector_range_end = None
        self._calib_raw_files = []
        self._calib_workspace = None

    def execute(self, reducer, file_ws):
        """The information we use here is not from the main reducer object
        (ie, we are not looking at one of the data files.)
        
        The ApplyCalibration step is related to this.
        """
        rawfiles = self._calib_raw_files
        if ( len(rawfiles) == 0 ):
            print "Indirect: No calibration run specified."
            return

        backMin, backMax, peakMin, peakMax = self._get_calib_details()
        specMin = self._detector_range_start + 1
        specMax = self._detector_range_end + 1

        runs = []
        for file in rawfiles:
            (direct, filename) = os.path.split(file)
            (root, ext) = os.path.splitext(filename)
            try:
                Load(file, root, SpectrumMin=specMin, SpectrumMax=specMax,
                    LoadLogFiles=False)
                runs.append(root)
            except:
                sys.exit('Indirect: Could not load raw file: ' + file)
        cwsn = 'calibration'
        if ( len(runs) > 1 ):
            MergeRuns(",".join(runs), cwsn)
            factor = 1.0 / len(runs)
            Scale(cwsn, cwsn, factor)
        else:
            cwsn = runs[0]
        FlatBackground(cwsn, cwsn, backMin, backMax, Mode='Mean')
        Integration(cwsn, cwsn, peakMin, peakMax)
        cal_ws = mtd[cwsn]
        sum = 0
        for i in range(0, cal_ws.getNumberHistograms()):
            sum += cal_ws.readY(i)[0]

        runNo = cal_ws.getRun().getLogData("run_number").value
        outWS_n = runs[0][:3] + runNo + '_cal'

        value = 1.0 / ( sum / cal_ws.getNumberHistograms() )
        Scale(cwsn, cwsn, value, 'Multiply')

        RenameWorkspace(cwsn, outWS_n)
        self._calib_workspace = outWS_n # Set result workspace value

    def set_parameters(self, back_min, back_max, peak_min, peak_max):
        self._back_min = back_min
        self._back_max = back_max
        self._peak_min = peak_min
        self._peak_max = peak_max
        
    def set_detector_range(self, start, end):
        self._detector_range_start = start
        self._detector_range_end = end
        
    def set_instrument_workspace(self, workspace):
        self._instrument_workspace = workspace
        
    def set_files(self, files):
        if len(files) > 0:
            self._calib_raw_files = files
        else:
            raise ValueError("Indirect: Can't set calib files if you don't "
                "specify a calib file.")
                
    def result_workspace(self):
        return self._calib_workspace
        
    def _get_calib_details(self):    
        if ( self._back_min is None and
                self._back_max is None and
                self._peak_min is None and
                self._peak_max is None ):
            instrument = mtd[self._instrument_workspace].getInstrument()
            try:
                backMin = instrument.getNumberParameter('back-start')[0]
                backMax = instrument.getNumberParameter('back-end')[0]
                peakMin = instrument.getNumberParameter('peak-start')[0]
                peakMax = instrument.getNumberParameter('peak-end')[0]
            except IndexError:
                sys.exit("Indirect: Unable to retrieve calibration details "
                    "from instrument of workspace.")
            else:
                return backMin, backMax, peakMin, peakMax
        else:
            return ( self._back_min, self._back_max, self._peak_min,
                self._peak_max )

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
        if ( self._multiple_frames ):
            workspaces = mtd[file_ws].getNames()
        else:
            workspaces = [file_ws]

        for ws in workspaces:
            Divide(ws, self._calib_workspace, ws)

    def set_is_multiple_frames(self, value):
        self._multiple_frames = value

    def set_calib_workspace(self, value):
        self._calib_workspace = value

class HandleMonitor(ReductionStep):
    """Handles the montior for the reduction of inelastic indirect data.
    
    This uses the following parameters from the instrument:
    * Workflow.MonitorArea
    * Workflow.MonitorThickness
    * Workflow.MonitorScalingFactor
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
        if ( self._multiple_frames ):
            workspaces = mtd[file_ws].getNames()
        else:
            workspaces = [file_ws]

        for ws in workspaces:
            monitor = ws+'_mon'
            if self._need_to_unwrap(ws):
                self._unwrap_monitor(ws)
            else:
                ConvertUnits(monitor, monitor, 'Wavelength')
            self._monitor_efficiency(monitor)
            self._scale_monitor(monitor)

    def _need_to_unwrap(self, ws):
        try:
            unwrap = mtd[ws].getInstrument().getStringParameter(
                'Workflow.UnwrapMonitor')[0]
        except IndexError:
            return False # Default it to not unwrap
        if ( unwrap == 'Never' ):
            return False
        elif ( unwrap == 'Always' ):
            return True
        elif ( unwrap == 'BaseOnTimeRegime' ):
            SpecMon = mtd[ws+'_mon'].readX(0)[0]
            SpecDet = mtd[ws].readX(0)[0]
            if ( SpecMon == SpecDet ):
                return True
            else:
                return False
        else:
            return False

    def _unwrap_monitor(self, ws):
        l_ref = self._get_reference_length(ws, 0)
        monitor = ws+'_mon'
        alg = UnwrapMonitor(monitor, monitor, LRef=l_ref)
        join = float(alg.getPropertyValue('JoinWavelength'))
        RemoveBins(monitor, monitor, join-0.001, join+0.001, 
            Interpolation='Linear')
        FFTSmooth(monitor, monitor, 0)

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
            area = inst.getNumberParameter('Workflow.MonitorArea')[0]
            thickness = inst.getNumberParameter('Workflow.MonitorThickness')[0]
        except IndexError:
            raise ValueError('Unable to retrieve monitor thickness and '
                'area from Instrument Parameter file.')
        else:
            OneMinusExponentialCor(monitor, monitor, (8.3 * thickness), area)

    def _scale_monitor(self, monitor):
        """Some instruments wish to scale their data. Doing this at the
        monitor is the most efficient way to do this. This is controlled
        by the 'Workflow.MonitorScalingFactor' parameter set on the 
        instrument.
        """
        try:
            factor = mtd[monitor].getInstrument().getNumberParameter(
                'Workflow.MonitorScalingFactor')[0]
        except IndexError:
            print "Monitor is not being scaled."
        else:
            Scale(monitor, monitor, ( 1.0 / factor ), 'Multiply')

class CorrectByMonitor(ReductionStep):
    """
    """

    _multiple_frames = False

    def __init__(self, MultipleFrames=False):
        super(CorrectByMonitor, self).__init__()
        self._multiple_frames = MultipleFrames

    def execute(self, reducer, file_ws):
        if ( self._multiple_frames ):
            workspaces = mtd[file_ws].getNames()
        else:
            workspaces = [file_ws]

        for ws in workspaces:
            ConvertUnits(ws, ws, 'Wavelength', 'Indirect')
            RebinToWorkspace(ws, ws+'_mon', ws)
            Divide(ws, ws+'_mon', ws)
            DeleteWorkspace(ws+'_mon')

class FoldData(ReductionStep):
    """
    """

    def __init__(self):
        super(FoldData, self).__init__()
        
    def execute(self, reducer, file_ws):
        """Folds data back into a single workspace if it has been "chopped".
        """
        wsgroup = mtd[file_ws].getNames()        
        ws = file_ws+'_merged'
        MergeRuns(','.join(wsgroup), ws)
        scaling = self._create_scaling_workspace(wsgroup, ws)
        for workspace in wsgroup:
            DeleteWorkspace(workspace)
        Divide(ws, scaling, ws)
        DeleteWorkspace(scaling)
        RenameWorkspace(ws, file_ws)

    def _create_scaling_workspace(self, wsgroup, merged):
        wsname = '__scaling'
        largest = 0
        nlargest = 0
        nlrgws = ''
        for ws in wsgroup:
            nbin = mtd[ws].getNumberBins()
            if ( nbin > largest ):
                nlargest = largest
                largest = nbin
            elif ( nbin > nlargest ):
                nlargest = nbin
                nlrgws = ws
        nlargestX = mtd[nlrgws].dataX(0)[nlargest]	
        largestValInNLrg = nlargestX
        binIndex = mtd[merged].binIndexOf(largestValInNLrg)	
        dataX = list(mtd[merged].readX(0))
        dataY = []
        dataE = []
        totalBins = mtd[merged].getNumberBins()
        nWS = len(wsgroup)
        for i in range(0, binIndex):
            dataE.append(0.0)
            dataY.append(nWS)    
        for i in range(binIndex, totalBins):
            dataE.append(0.0)
            dataY.append(1.0)	
        CreateWorkspace(wsname, dataX, dataY, dataE, UnitX='DeltaE')
        return wsname

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
        if ( self._multiple_frames ):
            workspaces = mtd[file_ws].getNames()
        else:
            workspaces = [file_ws]
            
        for ws in workspaces:
            ConvertUnits(ws, ws, 'DeltaE', 'Indirect')
            CorrectKiKf(ws, ws, 'Indirect')
            if self._rebin_string is not None:
                Rebin(ws, ws, self._rebin_string)
            
    def set_rebin_string(self, value):
        if value is not None:
            self._rebin_string = value

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

        if ( self._multiple_frames ):
            workspaces = mtd[file_ws].getNames()
        else:
            workspaces = [file_ws]

        for ws in workspaces:
            ExponentialCorrection(ws, ws, 1.0, correction)
        
    def set_temperature(self, temp):
        self._temp = temp
            
class Grouping(ReductionStep):
    """This ReductionStep handles the grouping and renaming of the final
    workspace. In most cases, this will require a Rebin on the data. The option
    to do this is given in the ConvertToEnergy step.
    
    The step will use the following parameters on the workspace:
    * 'Workflow.GroupingMethod' - if this is set to Fixed, it indicates that
        the grouping is defined at an instrument level and this can not be
        altered by the user. In this case, the value given in the function
        set_grouping_policy() is ignored and an XML grouping file is created
        based on the string in
    * 'Workflow.FixedGrouping', which is of the form: "0-69,70-139" where the
        comma seperates a group, and the hyphen indicates a range. The numbers
        given are taken to be the workspace indices.
        
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
        if ( self._multiple_frames ):
            workspaces = mtd[file_ws].getNames()
        else:
            workspaces = [file_ws]
            
        for ws in workspaces:
            try:
                group = mtd[ws].getInstrument().getStringParameter(
                    'Workflow.GroupingMethod')[0]
            except IndexError:
                group = 'User'
            if ( group == 'Fixed' ):
                self._result_workspaces.append(self._group_fixed(ws))
            else:
                self._result_workspaces.append(self._group_data(ws))
            
    def set_grouping_policy(self, value):
        self._grouping_policy = value
        
    def set_mask_list(self, value):
        self._masking_detectors = value
        
    def get_result_workspaces(self):
        return self._result_workspaces

    def _group_fixed(self, workspace):
        try:
            grps = mtd[workspace].getInstrument().getStringParameter(
                'Workflow.FixedGrouping')[0]
        except IndexError:
            raise AttributeError('Could not retrieve fixed grouping setting '
                'from the instrument parameter file.')

        groups = grps.split(",")
        group_list = []
        for group in groups:
            group_to_from = group.split("-")
            group_vals = range(int(group_to_from[0]), int(group_to_from[1])+1)
            group_list.append(group_vals)
            
        for i in self._masking_detectors:
            for grp in group_list:
                try:
                    grp.remove(i)
                except ValueError:
                    pass

        xml = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        xml += "<detector-grouping>\n"
        for grp in group_list:
            xml += "<group name=\"group\">\n"
            xml += "    <ids val=\""
            for i in grp:
                xml += str(i + 1)
                if i != ( grp[len(grp)-1] ):
                    xml += ","
            xml += "\"/>\n"
            xml += "</group>\n"
        xml += "</detector-grouping>\n"
        
        xfile = mtd.getConfigProperty('defaultsave.directory') + 'fixedGrp.xml'
        file = open(xfile, 'w')
        file.write(xml)
        file.close()
        GroupDetectors(workspace, workspace, MapFile=xfile, 
            Behaviour='Average')
        return workspace

    def _group_data(self, workspace):
        grouping = self._grouping_policy
        if ( grouping == 'Individual' ) or ( grouping is None ):
            return workspace
        elif ( grouping == 'All' ):
            nhist = mtd[workspace].getNumberHistograms()
            GroupDetectors(workspace, workspace, 
                WorkspaceIndexList=range(0,nhist), Behaviour='Average')
        else:
            GroupDetectors(workspace, workspace, MapFile=grouping,
                Behaviour='Average')
        return workspace


        
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
        
    def execute(self, reducer, file_ws):
        wsname = self._get_ws_name(file_ws)
        RenameWorkspace(file_ws, wsname)
        self._result_workspaces.append(wsname)

    def get_result_workspaces(self):
        return self._result_workspaces

    def _get_ws_name(self, workspace):
        try:
            type = mtd[workspace].getInstrument().getStringParameter(
                'Workflow.NamingConvention')[0]
        except IndexError:
            type = 'RunTitle'
            
        if ( type == 'AnalyserReflection' ):
            return self._analyser_reflection(workspace)
        elif ( type == 'RunTitle' ):
            return self._run_title(workspace)
        else:
            raise NotImplementedError('Unknown \'Workflow.NamingConvention\''
                ' parameter encountered on workspace: ' + workspace)
        
    def _run_title(self, workspace):
        ws = mtd[workspace]
        title = ws.getRun()['run_title'].value.strip()
        runNo = ws.getRun()['run_number'].value
        inst = ws.getInstrument().getName()
        isn = ConfigService().facility().instrument(inst).shortName().upper()
        valid = "-_.() %s%s" % (string.ascii_letters, string.digits)
        title = ''.join(ch for ch in title if ch in valid)
        title = isn + runNo + '-' + title
        return title
        
    def _analyser_reflection(self, workspace):
        if workspace == '':
            return ''
        ws = mtd[workspace]
        ins = ws.getInstrument().getName()
        ins = ConfigService().facility().instrument(ins).shortName().lower()
        run = ws.getRun().getLogData('run_number').value
        try:
            analyser = ws.getInstrument().getStringParameter('analyser')[0]
            reflection = ws.getInstrument().getStringParameter('reflection')[0]
        except IndexError:
            analyser = ''
            reflection = ''
        prefix = ins + run + '_' + analyser + reflection + '_red'
        return prefix        