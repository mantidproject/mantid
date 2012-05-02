import os
import copy
import math
from MantidFramework import *
mtd.initialise(False)
from mantidsimple import *
import _qti
import mantidplot
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
            self._graph = "Range Selector"
            
        def disconnect(self):
            _qti.app.disconnect(_qti.app.mantidUI, 
                                QtCore.SIGNAL("x_range_update(double,double)"),
                                self._call_back)
            
        def connect(self, ws, call_back, xmin=None, xmax=None, 
                    range_min=None, range_max=None, x_title=None,
                    log_scale=False,
                    ws_output_base=None):
            
            self._call_back = call_back
            self._ws_output_base = ws_output_base
            
            _qti.app.connect(_qti.app.mantidUI,
                             QtCore.SIGNAL("x_range_update(double,double)"),
                             self._call_back)
            g = mantidplot.graph(self._graph)
            
            if g._getHeldObject is not None:
                g.close()
                
            g = mantidplot.plotSpectrum(ws, [0], True)
            g.setName(self._graph)        
            l=g.activeLayer()
            try:
                title = ws[0].replace("_"," ")
                title.strip()
            except:
                title = " "
            l.setTitle(" ")
            l.setCurveTitle(0, title)
            if log_scale:
                l.logYlinX()
            if x_title is not None:
                l.setXTitle(x_title)
            if xmin is not None and xmax is not None:
                l.setScale(2,xmin,xmax)
                
            if range_min is not None and range_max is not None:
                mantidplot.selectMultiPeak(g, False, range_min, range_max)            
            else:
                mantidplot.selectMultiPeak(g, False)
    
    @classmethod
    def connect(cls, ws, call_back, xmin=None, xmax=None,
                range_min=None, range_max=None, x_title=None,
                log_scale=False, ws_output_base=None):
        
        if RangeSelector.__instance is not None:
            RangeSelector.__instance.disconnect()
        else:
            RangeSelector.__instance = RangeSelector._Selector()

        RangeSelector.__instance.connect(ws, call_back, xmin=xmin, xmax=xmax,
                                         range_min=range_min, range_max=range_max,
                                         x_title=x_title, log_scale=log_scale)            
    
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
        self._restricted_range = False
        
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
        
    def get_skipped_range(self):
        """
            Get the non-zero x range of the data, excluding the skipped
            points 
        """
        if self.is_loaded():
            x = mtd[self._ws_name].readX(0)
            y = mtd[self._ws_name].readY(0)
            xmin = x[0]
            xmax = x[len(x)-1]
            
            for i in range(len(y)):
                if y[i]!=0.0:
                    xmin = x[i+self._skip_first]
                    break

            for i in range(len(y)-1,-1,-1):
                if y[i]!=0.0:
                    xmax = x[i-self._skip_last]
                    break
            
            return xmin, xmax
        else:
            return self.get_range()
        
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
        
        # Get rid of points with an error greater than the intensity
        if self._restricted_range:
            for i in range(len(y_scaled)):
                if y_scaled[i]<=e_scaled[i]:
                    y_scaled[i] = 0
                    e_scaled[i] = 0   
        
        # Dummy operation to update the plot
        Scale(InputWorkspace=self._ws_scaled, OutputWorkspace=self._ws_scaled,
              Operation="Multiply", Factor=1.0)
        
    def load(self, update_range=False, restricted_range=False):
        """
            Load a data set from file
            @param upate_range: if True, the Q range of the data set will be udpated
            @param restricted_range: if True, zeros at the beginning and end will be stripped
        """
        if os.path.isfile(self._file_path):
            self._ws_name = os.path.basename(self._file_path)
            Load(Filename=self._file_path, OutputWorkspace=self._ws_name)
        elif mtd.workspaceExists(self._file_path):
            self._ws_name = self._file_path
        
        if mtd.workspaceExists(self._ws_name):
            self._ws_scaled = self._ws_name+"_scaled"
            if update_range:
                self._restricted_range = restricted_range        
                self._xmin = min(mtd[self._ws_name].readX(0))
                self._xmax = max(mtd[self._ws_name].readX(0))
                if restricted_range:
                    y = mtd[self._ws_name].readY(0)
                    x = mtd[self._ws_name].readX(0)
                    
                    for i in range(len(y)):
                        if y[i]!=0.0:
                            self._xmin = x[i]
                            break
                    for i in range(len(y)-1,-1,-1):
                        if y[i]!=0.0:
                            self._xmax = x[i]
                            break
                        
            self._npts = len(mtd[self._ws_name].readY(0))
            self._last_applied_scale = 1.0
        
    def scale_to_unity(self, xmin=None, xmax=None):
        """
            Compute a scaling factor for which the average of the 
            data is 1 in the specified region
        """
        x = mtd[self._ws_name].readX(0)
        y = mtd[self._ws_name].readY(0)
        e = mtd[self._ws_name].readE(0)
        sum = 0.0
        sum_err = 0.0
        for i in range(len(y)):
            upper_bound = x[i]
            if len(x)==len(y)+1:
                upper_bound = x[i+1]
            if x[i]>=xmin and upper_bound<=xmax:
                sum += y[i]/(e[i]*e[i])
                sum_err += 1.0/(e[i]*e[i])

        return sum_err/sum
        
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
        e = mtd[self._ws_name].readE(0)
        
        is_histo = len(x)==len(y)+1
        if not is_histo and len(x)!=len(y):
            raise RuntimeError, "Corrupted I(q) %s" % self._ws_name
        
        sum = 0.0
        for i in range(len(y)-1):
            # Skip points compatible with zero within error
            if self._restricted_range and y[i]<=e[i]:
                continue
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
        
        for i in range(self._reference-1,-1,-1):
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
                          Separator="Tab", CommentIndicator="# ", WriteXError=True)
        
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
        
        if len(zipped)>0:
            x,y,e,dx = zip(*zipped)
        else:
            return [],[],[],[]
        return list(x), list(y), list(e), list(dx)            
        
    def get_scaled_data(self, workspace=None):
        """
            Return the data points for the scaled data set
        """
        if len(self._data_sets)==0:
            return
        
        ws_combined = "combined_Iq"
        if workspace is not None:
            ws_combined = workspace
        
        first_ws = self._data_sets[0].get_scaled_ws()
        if first_ws is None:
            return

        x = mtd[first_ws].dataX(0)
        dx = mtd[first_ws].dataDx(0)
        y = mtd[first_ws].dataY(0)
        e = mtd[first_ws].dataE(0)
        if len(x)==len(y)+1:
            xtmp = [(x[i]+x[i+1])/2.0 for i in range(len(y))]
            dxtmp = [(dx[i]+dx[i+1])/2.0 for i in range(len(y))]
            x = xtmp
            dx = dxtmp
        x, y, e, dx = self.trim_zeros(x, y, e, dx)
        
        for d in self._data_sets[1:]:
            ws = d.get_scaled_ws()
            if ws is not None:
                _x = mtd[ws].dataX(0)
                _y = mtd[ws].dataY(0)
                _e = mtd[ws].dataE(0)
                _dx = mtd[ws].dataDx(0)
                if len(_x)==len(_y)+1:
                    xtmp = [(_x[i]+_x[i+1])/2.0 for i in range(len(_y))]
                    dxtmp = [(_dx[i]+_dx[i+1])/2.0 for i in range(len(_y))]
                    _x = xtmp
                    _dx = dxtmp
                    
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
        
        CreateWorkspace(DataX=x, DataY=y, DataE=e,
                       OutputWorkspace=ws_combined,
                       UnitX="MomentumTransfer",
                       ParentWorkspace=first_ws)
        
        dxtmp = mtd[ws_combined].dataDx(0)
        
        # Fill out dQ
        npts = len(x)
        for i in range(npts):
            dxtmp[i] = dx[i]
            
        return ws_combined
        