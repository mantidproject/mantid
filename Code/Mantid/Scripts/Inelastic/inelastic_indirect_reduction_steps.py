from reducer import ReductionStep
from mantidsimple import *

class LoadData(ReductionStep):
    """Handles the loading of the data for Indirect instruments.
    Options to Sum, etc should be included here.
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
            LoadRaw(file, file, LoadLogFiles=False)
            
            if self._parameter_file != None:
                LoadParameterFile(file, self._parameter_file)
            
            if ( mtd[file].getInstrument().getName() == 'TOSCA' ):
                self._identify_bad_detectors(file)

            if ( wsname == '' ):
                wsname = file

            if self._require_chop_data(file):
                ChopData(file, file)
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

    def _require_chop_data(self, workspace):
        if ( mtd[workspace].getInstrument().getName() != 'TOSCA' ):
            return False
        if ( mtd[workspace].readX(0)[mtd[workspace].getNumberBins()] > 40000 ):
            return True
        return False
        
    def is_multiple_frames(self):
        return self._multiple_frames

class BackgroundOperations(ReductionStep):
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
                LoadRaw(file, root, SpectrumMin=specMin, SpectrumMax=specMax,
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
        if ( mtd[ws].getInstrument().getName() == 'TOSCA' ):
            return False
        SpecMon = mtd[ws+'_mon'].readX(0)[0]
        SpecDet = mtd[ws].readX(0)[0]
        if ( SpecMon == SpecDet ):
            return True
        else:
            return False

    def _unwrap_monitor(self, ws):
        l_ref = self._get_reference_length(ws, 0)
        monitor = ws+'_mon'
        alg = Unwrap(monitor, monitor, LRef=l_ref)
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

    def __init__(self):
        super(FoldData, self).__init__()
        
    def execute(self, reducer, file_ws):
        """Folds data back into a single workspace if it has been "chopped".
        """
        wsgroup = mtd[file_ws].getNames()        
        workspaces = ','.join(wsgroup)
        ws = file_ws+'_merged'
        MergeRuns(workspaces, ws)
        scaling = self._create_scaling_workspace(wsgroup, ws)
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
        CreateWorkspace(wsname, dataX, dataY, dataE, UnitX='Wavelength')
        return wsname

class ConvertToEnergy(ReductionStep):
    """
    """
    
    _rebin_string = None
    
    def __init__(self):
        super(ConvertToEnergy, self).__init__()
        self._rebin_string = None
        
    def execute(self, reducer, file_ws):
        ConvertUnits(file_ws, file_ws, 'DeltaE', 'Indirect')
        CorrectKiKf(file_ws, file_ws, 'Indirect')
        if self._rebin_string is not None:
            Rebin(file_ws, file_ws, reducer._rebin_string)
            
    def set_rebin_string(self, value):
        if value is not None:
            self._rebin_string = value

class DetailedBalance(ReductionStep):

    _temp = None
    
    def __init__(self):
        super(DetailedBalance, self).__init__()
        self._temp = None
        
    def execute(self, reducer, file_ws):
        if self._temp is None:
            return        
        correction = 11.606 / ( 2 * self._temp )
        ExponentialCorrection(file_ws, file_ws, 1.0, correction)
        
    def set_temperature(self, temp):
        self._temp = temp
            
class Grouping(ReductionStep):

    _grouping_policy = None
    _masking_detectors = []
    
    def __init__(self):
        super(Grouping, self).__init__()
        self._grouping_policy = None
        self._masking_detectors = []
        
    def execute(self, reducer, file_ws):
        if ( mtd[file_ws].getInstrument().getName() == 'TOSCA' ):
            self._group_tosca(file_ws)
        else:
            self._group_data(file_ws)
            
    def set_grouping_policy(self, value):
        self._grouping_policy = value
        
    def set_mask_list(self, value):
        self._masking_detectors = value
            
    def _group_tosca(self, workspace):
        wsname = self._get_run_title(workspace)
        grp = range(0,70)
        for i in self._masking_detectors:
            try:
                grp.remove(i)
            except ValueError:
                pass
        GroupDetectors(workspace, wsname, WorkspaceIndexList=grp,
            Behaviour='Average')
        grp = range(70,140)
        for i in self._masking_detectors:
            try:
                grp.remove(i)
            except ValueError:
                pass
        GroupDetectors(workspace, '__front', WorkspaceIndexList=grp,
            Behaviour='Average')
        DeleteWorkspace(workspace)
        ConjoinWorkspaces(wsname, '__front')
        return wsname

    def _group_data(self, workspace):
        grouping = self._grouping_policy
        name = self._get_run_title(workspace)
        if ( grouping == 'Individual' ) or ( grouping is None ):
            if ( workspace != name ):
                RenameWorkspace(workspace, name)
            return name
        elif ( grouping == 'All' ):
            nhist = mtd[workspace].getNumberHistograms()
            GroupDetectors(workspace, name, WorkspaceIndexList=range(0,nhist),
                Behaviour='Average')
        else:
            GroupDetectors(workspace, name, MapFile=grouping,
                Behaviour='Average')
        if ( workspace != name ):
            DeleteWorkspace(workspace)
        return name

    def _get_run_title(self, workspace):
        ws = mtd[workspace]
        title = ws.getRun()['run_title'].value.strip()
        runNo = ws.getRun()['run_number'].value
        inst = ws.getInstrument().getName()
        isn = ConfigService().facility().instrument(inst).shortName().upper()
        valid = "-_.() %s%s" % (string.ascii_letters, string.digits)
        title = ''.join(ch for ch in title if ch in valid)
        title = isn + runNo + '-' + title
        return title