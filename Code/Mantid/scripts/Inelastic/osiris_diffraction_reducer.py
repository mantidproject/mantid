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
        sams = self._reduction_steps[0]._sam_files
        vans = self._reduction_steps[0]._van_files
        if self._verbose:
            print sams
            print vans
        assert len(sams) == len(vans), "There are a different number of sample workspaces to vanadium workspaces."
        wsl = []
        # zip together to allow looping over both maps, but in lock-step
        for sam, van in zip(sams.itervalues(), vans.itervalues()):
            # Delete Vanadium workspaces
            DeleteWorkspace(van)
            # Append sample workspace
            wsl.append(sam)
        if self._result_workspace is None:
            self._result_workspace = wsl[0] + '-to-' + wsl[len(wsl) - 1][3:]
        MergeRuns(','.join(wsl), self._result_workspace)
        for sam in sams.itervalues():
            DeleteWorkspace(sam)
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
            elif x < 9.4:
                dataY.append(1); dataE.append(1)
            elif x < 9.5:
                dataY.append(2); dataE.append(2)
            elif x < 10.4:
                dataY.append(1); dataE.append(1)
            elif x < 10.6:
                dataY.append(2); dataE.append(2)
            elif x < 11:
                dataY.append(1); dataE.append(1)
            elif x < 11.6:
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
        self._timeRegimeToD = {
            1.17e4:  1, 
            2.94e4:  2, 
            4.71e4:  3, 
            6.48e4:  4,
            8.25e4:  5,
           10.02e4:  6,
           11.79e4:  7,
           13.55e4:  8,
           15.32e4:  9,
           17.09e4: 10,
           18.86e4: 11}
        self._drange_mask = {
            1: [ 0.7,  2.5],
            2: [ 2.1,  3.3],
            3: [ 3.1,  4.3],
            4: [ 4.1,  5.3],
            5: [ 5.2,  6.2],
            6: [ 6.2,  7.3],
            7: [ 7.3,  8.3],
            8: [ 8.3,  9.5],
            9: [ 9.4, 10.6],
           10: [10.4, 11.6],
           11: [11.0, 12.5]}

    def set_vanadium(self, Value):
        self._van = Value