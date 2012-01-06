import os
import copy
from MantidFramework import *
mtd.initialise(False)
from mantidsimple import *

from PyQt4 import QtGui, QtCore

class RangeSelector(object):
    """
        Brings up range selector window and connects the user selection to
        a call-back function.
    """
    __instance=None

    class _Selector(object):
        def __init__(self):
            self._call_back = None
            self._graph = "StitcherRangeSelector"
            
        def disconnect(self):
            qti.app.disconnect(qti.app.mantidUI, QtCore.SIGNAL("x_range_update(double,double)"), self._call_back)
            
        def connect(self, ws, call_back):
            self._call_back = call_back
            qti.app.connect(qti.app.mantidUI, QtCore.SIGNAL("x_range_update(double,double)"), self._call_back)
            g = qti.app.graph(self._graph)
            if g is None:
                g = qti.app.mantidUI.pyPlotSpectraList(ws,[0],True)
                g.setName(self._graph)        
            qti.app.selectMultiPeak(False)
            qti.app.selectMultiPeak(False)
    
    @classmethod
    def connect(cls, ws, call_back):
        if RangeSelector.__instance is not None:
            RangeSelector.__instance.disconnect()
        else:
            RangeSelector.__instance = RangeSelector._Selector()
        RangeSelector.__instance.connect(ws, call_back)            
    
class DataSet(object):
    """
        Data set class for stitcher
    """
    def __init__(self, file_path=""):
        self._file_path = file_path
        self._xmin = None
        self._xmax = None
        self._ws_name = None
        self._ws_scaled = None
        self._scale = 1.0
        self._last_applied_scale = 1.0
        self._skip_last = 0
        self._skip_first = 0
        self._npts = None
        
    def __str__(self):
        return self._ws_name
    
    def get_number_of_points(self):
        return self._npts
    
    def get_scaled_ws(self):
        """
           Get the name of the scaled workspace, if it exists 
        """
        if mtd.workspaceExists(self._ws_scaled):
            return self._ws_scaled
        return None
    
    def set_skipped_points(self, first, last):
        """
            Set the number of points to skip at the beginning and
            end of the distribution
            @param first: number of points to skip at the beginning of distribution
            @param last: number of points to skip at the end of distribution
        """
        self._skip_last = last
        self._skip_first = first
        
    def is_loaded(self):
        """
            Return True is this data set has been loaded
        """
        return mtd.workspaceExists(self._ws_name)
    
    def set_scale(self, scale=1.0):
        """
            Set the scaling factor for this data set
            @param scale: scaling factor
        """
        self._scale = scale
    
    def get_scale(self):
        """
            Get the current scaling factor for this data set
        """
        return self._scale
    
    def set_range(self, xmin, xmax):
        """
            Set the Q range for this data set
            @param xmin: minimum Q value
            @param xmax: maximum Q value
        """
        self._xmin = xmin
        self._xmax = xmax
        
    def get_range(self):
        """
            Return the Q range for this data set
        """
        return self._xmin, self._xmax
    
    def apply_scale(self):
        """
            Apply the scaling factor to the unmodified data set
        """
        self.load()
        
        # Keep track of dQ
        dq = mtd[self._ws_name].readDx(0)
        
        Scale(InputWorkspace=self._ws_name, OutputWorkspace=self._ws_scaled,
              Operation="Multiply", Factor=self._scale)
        
        # Put back dQ
        dq_scaled = mtd[self._ws_scaled].dataDx(0)
        for i in range(len(dq)):
            dq_scaled[i] = dq[i]
            
        y_scaled = mtd[self._ws_scaled].dataY(0)
        e_scaled = mtd[self._ws_scaled].dataE(0)
        for i in range(self._skip_first):
            y_scaled[i] = 0
            e_scaled[i] = 0
            
        first_index = max(len(y_scaled)-self._skip_last, 0)
        for i in range(first_index, len(y_scaled)):
            y_scaled[i] = 0
            e_scaled[i] = 0
        
        # Dummy operation to update the plot
        Scale(InputWorkspace=self._ws_scaled, OutputWorkspace=self._ws_scaled,
              Operation="Multiply", Factor=1.0)
        
    def load(self, update_range=False):
        """
            Load a data set from file
            @param upate_range: if True, the Q range of the data set will be udpated
        """
        if os.path.isfile(self._file_path):
            self._ws_name = os.path.basename(self._file_path)
            Load(Filename=self._file_path, OutputWorkspace=self._ws_name)
        elif mtd.workspaceExists(self._file_path):
            self._ws_name = self._file_path
        
        if mtd.workspaceExists(self._ws_name):
            self._ws_scaled = self._ws_name+"_scaled"
            if update_range:
                self._xmin = min(mtd[self._ws_name].readX(0))
                self._xmax = max(mtd[self._ws_name].readX(0))
            self._npts = len(mtd[self._ws_name].readY(0))
            self._last_applied_scale = 1.0
        
    def integrate(self, xmin=None, xmax=None):
        """
            Integrate a distribution between the given boundaries
            @param xmin: minimum Q value
            @param xmax: maximum Q value
        """
        self.load()
        
        if xmin is None:
            xmin = self._xmin
        if xmax is None:
            xmax = self._xmax
            
        x = mtd[self._ws_name].readX(0)
        y = mtd[self._ws_name].readY(0)
        
        is_histo = len(x)==len(y)+1
        if not is_histo and len(x)!=len(y):
            raise RuntimeError, "Corrupted I(q) %s" % self._ws_name
        
        sum = 0.0
        for i in range(len(y)-1):
            if x[i]>=xmin and x[i+1]<=xmax:
                sum += (y[i]+y[i+1])*(x[i+1]-x[i])/2.0
            elif x[i]<xmin and x[i+1]>xmin:
                sum += (y[i+1]+(y[i]-y[i+1])/(x[i]-x[i+1])*(x[i]-xmin)/2.0) * (x[i+1]-xmin)
            elif x[i]<xmax and x[i+1]>xmax:
                sum += (y[i]+(y[i+1]-y[i])/(x[i+1]-x[i])*(xmax-x[i])/2.0) * (xmax-x[i])
        
        return sum
            
    def select_range(self, call_back=None):
        if mtd.workspaceExists(self._ws_name):
            if call_back is None:
                call_back = self.set_range
            RangeSelector.connect([self._ws_name], call_back=call_back)
                 
class Stitcher(object):
    """
        Data set stitcher
    """
    def __init__(self):
        ## Reference ID (int)
        self._reference = None
        ## List of data sets to process
        self._data_sets = []
        
    def size(self):
        """
            Return the number of data sets
        """
        return len(self._data_sets)
    
    def append(self, data_set):
        """
            Append a data set to the list of data sets to process
            @param data_set: DataSet object
        """
        self._data_sets.append(data_set)
        return len(self._data_sets)-1
        
    @classmethod
    def normalize(cls, data_ref, data_to_scale):
        """
            Scale a data set relative to a reference
            @param data_ref: reference data set
            @param data_to_scale: data set to rescale
        """
        if data_ref == data_to_scale:
            return
        
        # Get ranges
        ref_min, ref_max = data_ref.get_range()
        d_min, d_max = data_to_scale.get_range()
        
        # Check that we have an overlap
        if ref_max<d_min or ref_min>d_max:
            mtd.sendLogMessage("No overlap between %s and %s" % (str(data_ref), str(data_to_scale)))
            return
        
        # Get overlap
        xmin = max(ref_min, d_min)
        xmax = min(ref_max, d_max)
            
        # Compute integrals
        sum_ref = data_ref.integrate(xmin, xmax)
        sum_d = data_to_scale.integrate(xmin, xmax)
        
        if sum_ref!=0 and sum_d!=0:
            ref_scale = data_ref.get_scale()
            data_to_scale.set_scale(ref_scale*sum_ref/sum_d)
    
    def compute(self):
        """
            Compute scaling factors relative to reference data set
        """
        if len(self._data_sets)<2:
            return
        
        for i in range(self._reference):
            Stitcher.normalize(self._data_sets[i+1], self._data_sets[i])
        for i in range(self._reference,len(self._data_sets)-1):
            Stitcher.normalize(self._data_sets[i], self._data_sets[i+1])
        
    def set_reference(self, id):
        """
            Select which data set is the reference to normalize to
            @param id: index of the reference in the internal file list.
        """
        if id>=len(self._data_sets):
            raise RuntimeError, "Stitcher: invalid reference ID"
        self._reference = id

    def save_combined(self, file_path=None, as_canSAS=True):
        """
            Save the resulting scaled I(Q) curves in one data file
            @param file_path: file to save data in
        """
        iq = self.get_scaled_data()        
        if file_path is not None:
            if as_canSAS:
                SaveCanSAS1D(Filename=file_path, InputWorkspace=iq)
            else:
                SaveAscii(Filename=file_path, InputWorkspace=iq, 
                          Separator="\t", CommentIndicator="# ", WriteXError=True)
        
    def trim_zeros(self, x, y, e, dx):
        zipped = zip(x,y,e,dx)
        trimmed = []
        
        data_started = False        
        # First the zeros at the beginning
        for i in range(len(zipped)):
            if data_started or zipped[i][1]>0:
                data_started = True
                trimmed.append(zipped[i])
        
        # Then the trailing zeros
        zipped = []
        data_started = False
        for i in range(len(trimmed)-1,-1,-1):
            if data_started or trimmed[i][1]>0:
                data_started = True
                zipped.append(trimmed[i])
        
        x,y,e,dx = zip(*zipped)
        return list(x), list(y), list(e), list(dx)
        
    def get_scaled_data(self):
        """
            Return the data points for the scaled data set
        """
        if len(self._data_sets)==0:
            return
        
        ws_combined = "combined_Iq"
        
        # Copy over dQ
        CloneWorkspace(InputWorkspace=self._data_sets[0].get_scaled_ws(),
                       OutputWorkspace=ws_combined)

        x = mtd[ws_combined].dataX(0)
        dx = mtd[ws_combined].dataDx(0)
        y = mtd[ws_combined].dataY(0)
        e = mtd[ws_combined].dataE(0)
        if len(x)!=len(y) and len(x)!=len(e):
            raise RuntimeError, "Stitcher expected distributions but got histo"
        x, y, e, dx = self.trim_zeros(x, y, e, dx)
        
        for d in self._data_sets[1:]:
            ws = d.get_scaled_ws()
            if ws is not None:
                _x = mtd[ws].dataX(0)
                _y = mtd[ws].dataY(0)
                _e = mtd[ws].dataE(0)
                _dx = mtd[ws].dataDx(0)
                if len(_x)!=len(_y) and len(_x)!=len(_e):
                    raise RuntimeError, "Stitcher expected distributions but got histo"
                _x, _y, _e, _dx = self.trim_zeros(_x, _y, _e, _dx)
                x.extend(_x)
                y.extend(_y)
                e.extend(_e)
                dx.extend(_dx)
        
        zipped = zip(x,y,e,dx)
        def cmp(p1,p2):
            if p2[0]==p1[0]:
                return 0
            return -1 if p2[0]>p1[0] else 1
        combined = sorted(zipped, cmp)
        x,y,e,dx = zip(*combined)
        
        xtmp = mtd[ws_combined].dataX(0)
        ytmp = mtd[ws_combined].dataY(0)
        etmp = mtd[ws_combined].dataE(0)
        dxtmp = mtd[ws_combined].dataDx(0)
        
        # Use the space we have in the current data vectors
        npts = len(ytmp)
        if len(x)>=npts:
            for i in range(npts):
                xtmp[i] = x[i]
                ytmp[i] = y[i]
                etmp[i] = e[i]
                dxtmp[i] = dx[i]
            if len(x)>npts:
                xtmp.extend(x[npts:])
                ytmp.extend(y[npts:])
                etmp.extend(e[npts:])
                dxtmp.extend(dx[npts:])
        else:
            for i in range(len(x)):
                xtmp[i] = x[i]
                ytmp[i] = y[i]
                etmp[i] = e[i]
                dxtmp[i] = dx[i]
            for i in range(len(x),npts):
                xtmp[i] = xtmp[len(x)-1]+1.0
                ytmp[i] = 0.0
                etmp[i] = 0.0
                dxtmp[i] = 0.0
            CropWorkspace(InputWorkspace=ws_combined, OutputWorkspace=ws_combined, XMin=0.0, XMax=xtmp[len(x)-1])
        return ws_combined
        
                    
                    
            
        
        
        