from mantidsimple import *
from reducer import Reducer, ReductionStep

class OsirisDiffractionReducer(Reducer):
    """Reducer class for OSIRIS Diffraction.
    """
    
    _cal = None
    _mapping = {} # Mapping of sample:vanadium
    _verbose = False
    _sample_files = {}
    _vanadium_files = []
    _result_workspace = None

    def __init__(self, Verbose=False, OutputWorkspace=None):
        """Constructor function.
        """
        super(OsirisDiffractionReducer, self).__init__()
        self._cal = None
        self._verbose = Verbose
        self._result_workspace = OutputWorkspace

    def pre_process(self):
        self._reduction_steps = []
        if self._cal is None:
            raise RuntimeError("No .cal file specified.")
        step = DRangeIdentifier(self._cal, Vanadium=True)
        for file in self._vanadium_files:
            step.execute(self, file)
        step.set_vanadium(False)
        self.append_step(step)

    def post_process(self):
        sam = self._reduction_steps[0]._sam_files
        van = self._reduction_steps[0]._van_files
        if self._verbose:
            print sam
            print van
        if ( len(sam) != len(van) ):
            raise RuntimeError("Something weird be happening.")
        wsl = []
        for i in range(1,6):
            # Delete Vanadium workspaces
            DeleteWorkspace(van[i])
            wsl.append(sam[i])
        if self._result_workspace is None:
            self._result_workspace = wsl[0] + '-to-' + wsl[4][3:]
        MergeRuns(','.join(wsl), self._result_workspace)
        for i in range(1,6):
            DeleteWorkspace(sam[i])
        # Divide/scale _result_workspace
        scale = self._create_scalar()
        Divide(self._result_workspace, scale, self._result_workspace)
        DeleteWorkspace(scale)

    def set_cal_file(self, cal_file):
        self._cal = cal_file

    def append_vanadium_file(self, file):
        self._vanadium_files.append(file)
        
    def result_workspace(self):
        return self._result_workspace
        
    def _create_scalar(self):
        ws = mtd[self._result_workspace]
        dataX = ws.dataX(0)
        dataY = []; dataE = []
        for i in range(0, len(dataX)-1):
            x = ( dataX[i] + dataX[i+1] ) / 2.0
            if x < 2.1:
                dataY.append(1); dataE.append(1)
            elif x < 2.5:
                dataY.append(2); dataE.append(2)
            elif x < 3.1:
                dataY.append(1); dataE.append(1)
            elif x < 3.3:
                dataY.append(2); dataE.append(2)
            elif x < 4.1:
                dataY.append(1); dataE.append(1)
            elif x < 4.3:
                dataY.append(2); dataE.append(2)
            elif x < 5.2:
                dataY.append(1); dataE.append(1)
            elif x < 5.3:
                dataY.append(2); dataE.append(2)
            else:
                dataY.append(1); dataE.append(1)
        CreateWorkspace("scaling", dataX, dataY, dataE, UnitX="dSpacing")
        return "scaling"

class DRangeIdentifier(ReductionStep):
    """This step creates a map linking each sample file to the corresponing
    vanadium file.
    """
    _timeRegimeToD = {}
    _drange_mask = {}
    _cal = None
    _van = False
    
    _sam_files = {}
    _van_files = {}
    
    def execute(self, reducer, file_ws):
        Load(file_ws, file_ws, SpectrumMin=3, SpectrumMax=962)
        
        td = self._range(file_ws)
        if not self._van:
            self._sam_files[td] = file_ws
        else:
            self._van_files[td] = file_ws
        
        NormaliseByCurrent(file_ws, file_ws)
        AlignDetectors(file_ws, file_ws, self._cal)
        DiffractionFocussing(file_ws, file_ws, self._cal)
        
        mask = self._drange_mask[td]
        CropWorkspace(file_ws, file_ws, XMin=mask[0], XMax=mask[1])
        
        if not self._van:
            Divide(file_ws, self._van_files[td], file_ws)
            ReplaceSpecialValues(file_ws, file_ws, 
                NaNValue=0.0, InfinityValue=0.0)
    
    def __init__(self, CalFile, Vanadium=False):
        super(DRangeIdentifier, self).__init__()
        self._cal = CalFile
        self._van = Vanadium
        
    def _range(self, workspace):
        if ( self._timeRegimeToD == {} ):
            self._load_settings()
        tb = mtd[workspace].dataX(0)[0]
        td = self._timeRegimeToD[tb]
        return td
        
    def _load_settings(self):
        """Information/settings in this function taken from: "The OSIRIS User
        Guide", 3rd Edition, Table 2, page 32, 'Standard Diffraction Settings
        at 25Hz'
        """
        self._timeRegimeToD = {1.17e4: 1, 2.94e4: 2, 4.71e4: 3, 6.48e4: 4,
            8.25e4: 5}
        self._drange_mask = {1: [0.7, 2.5], 2: [2.1, 3.3], 3: [3.1, 4.3],
            4: [4.1, 5.3], 5: [5.2, 6.2] }

    def set_vanadium(self, Value):
        self._van = Value