import os
import copy
import math
from mantid.simpleapi import *
from mantid.kernel import Logger

IS_IN_MANTIDPLOT = True
try:
    import mantidplot
    from PyQt4 import QtGui, QtCore
except:
    IS_IN_MANTIDPLOT = False

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
            if IS_IN_MANTIDPLOT:
                mantidplot.app.disconnect(mantidplot.app.mantidUI,
                                    QtCore.SIGNAL("x_range_update(double,double)"),
                                    self._call_back)

        def connect(self, ws, call_back, xmin=None, xmax=None,
                    range_min=None, range_max=None, x_title=None,
                    log_scale=False,
                    ws_output_base=None):
            if not IS_IN_MANTIDPLOT:
                print "RangeSelector cannot be used output MantidPlot"
                return

            self._call_back = call_back
            self._ws_output_base = ws_output_base

            mantidplot.app.connect(mantidplot.app.mantidUI,
                             QtCore.SIGNAL("x_range_update(double,double)"),
                             self._call_back)
            g = mantidplot.graph(self._graph)

            if g is not None:
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
        return str(self._ws_name)

    def get_number_of_points(self):
        return self._npts

    def get_scaled_ws(self):
        """
           Get the name of the scaled workspace, if it exists
        """
        if mtd.doesExist(self._ws_scaled):
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
        return mtd.doesExist(self._ws_name)

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

    def apply_scale(self, xmin=None, xmax=None):
        """
            Apply the scaling factor to the unmodified data set.
            If xmin and xmax are both set, only the data in the
            defined range will be scaled.
            @param xmin: minimum q-value
            @param xmax: maximum q-value
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

        if xmin is not None and xmax is not None:
            # Make sure we have point data
            ConvertToPointData(InputWorkspace=self._ws_scaled,
                               OutputWorkspace=self._ws_scaled)
            x = mtd[self._ws_scaled].readX(0)
            dx = dq_scaled
            y = mtd[self._ws_scaled].readY(0)
            e = mtd[self._ws_scaled].readE(0)

            x_trim = []
            dx_trim = []
            y_trim = []
            e_trim = []
            for i in range(len(y)):
                if x[i]>=xmin and x[i]<=xmax:
                    x_trim.append(x[i])
                    dx_trim.append(dx[i])
                    y_trim.append(y[i])
                    e_trim.append(e[i])

            CreateWorkspace(DataX=x_trim, DataY=y_trim, DataE=e_trim,
                           OutputWorkspace=self._ws_scaled,
                           UnitX="MomentumTransfer",
                           ParentWorkspace=self._ws_name)

            dq_scaled = mtd[self._ws_scaled].dataDx(0)
            for i in range(len(dq_scaled)):
                dq_scaled[i] = dx_trim[i]
        else:
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
            Scale(InputWorkspace=self._ws_scaled,
                  OutputWorkspace=self._ws_scaled,
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
        elif mtd.doesExist(self._file_path):
            self._ws_name = self._file_path
        else:
            raise RuntimeError, "Specified file doesn't exist: %s" % self._file_path

        if mtd.doesExist(self._ws_name):
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
        if mtd.doesExist(self._ws_name):
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

    def get_data_set(self, id):
        """
            Returns a particular data set
            @param id: position of the data set in the list
        """
        if id<0 or id>len(self._data_sets)-1:
            raise "Stitcher has not data set number %s" % str(id)
        return self._data_sets[id]

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
            Logger("data_stitching").error("No overlap between %s and %s" % (str(data_ref), str(data_to_scale)))
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

    def save_combined(self, file_path=None, as_canSAS=True, workspace=None):
        """
            Save the resulting scaled I(Q) curves in one data file
            @param file_path: file to save data in
        """
        iq = self.get_scaled_data(workspace=workspace)
        if file_path is not None:
            if as_canSAS:
                SaveCanSAS1D(Filename=file_path, InputWorkspace=iq)
            else:
                SaveAscii(Filename=file_path, InputWorkspace=iq,
                          Separator="Tab", CommentIndicator="# ",
                          WriteXError=True, WriteSpectrumID=False)

    def trim_zeros(self, x, y, e, dx):
        zipped = zip(x,y,e,dx)
        trimmed = []

        data_started = False
        # First the zeros at the beginning
        for i in range(len(zipped)):
            if data_started or zipped[i][1]!=0:
                data_started = True
                trimmed.append(zipped[i])

        # Then the trailing zeros
        zipped = []
        data_started = False
        for i in range(len(trimmed)-1,-1,-1):
            if data_started or trimmed[i][1]!=0:
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

        x = []
        dx = []
        y = []
        e = []
        for d in self._data_sets:
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

def stitch(data_list=[], q_min=None, q_max=None, output_workspace=None,
           scale=None, save_output=False):
    """
        @param data_list: list of N data files or workspaces to stitch
        @param q_min: list of N-1 Qmin values of overlap regions
        @param q_max: list of N-1 Qmax values of overlap regions
        @param output_workspace: name of the output workspace for the combined data
        @param scale: single overall scaling factor, of N scaling factors (one for each data set)
        @param save_output: If True, the combined output will be saved as XML
    """

    # Sanity check: q_min and q_max can either both be None or both be
    # of length N-1 where N is the length of data_list
    if (q_min is not None and q_max is None) or \
       (q_max is not None and q_min is None):
        error_msg = "Both q_min and q_max parameters should be provided, not just one"
        Logger("data_stitching").error(error_msg)
        raise RuntimeError, error_msg

    if not type(data_list)==list:
        error_msg = "The data_list parameter should be a list"
        Logger("data_stitching").error(error_msg)
        raise RuntimeError, error_msg

    n_data_sets = len(data_list)
    if n_data_sets<2:
        error_msg = "The data_list parameter should contain at least two data sets"
        Logger("data_stitching").error(error_msg)
        raise RuntimeError, error_msg

    # Check whether we just need to scale the data sets using the provided
    # scaling factors
    has_scale_factors = False
    if type(scale)==list:
        if len(scale)==n_data_sets:
            has_scale_factors = True
        else:
            error_msg = "If the scale parameter is provided as a list, it should have the same length as data_list"
            Logger("data_stitching").error(error_msg)
            raise RuntimeError, error_msg

    is_q_range_limited = False
    if q_min is not None and q_max is not None:
        is_q_range_limited = True
        if type(q_min) in [int, float]:
            q_min = [q_min]
        if type(q_max) in [int, float]:
            q_max = [q_max]

        if not type(q_min)==list or not type(q_max)==list:
            error_msg = "The q_min and q_max parameters must be lists"
            Logger("data_stitching").error(error_msg)
            raise RuntimeError, error_msg

        if not len(q_min)==n_data_sets-1:
            error_msg = "The length of q_min must be 1 shorter than the length of data_list: q_min=%s" % str(q_min)
            Logger("data_stitching").error(error_msg)
            raise RuntimeError, error_msg
        if not len(q_max)==n_data_sets-1:
            error_msg = "The length of q_max must be 1 shorter than the length of data_list: q_max=%s" % str(q_max)
            Logger("data_stitching").error(error_msg)
            raise RuntimeError, error_msg

        # Sanity check
        for i in range(n_data_sets-1):
            try:
                q_min[i] = float(q_min[i])
                q_max[i] = float(q_max[i])
            except:
                error_msg = "The Q range parameters are invalid: q_min=%s   q_max=%s" % (str(q_min), str(q_max))
                Logger("data_stitching").error(error_msg)
                raise RuntimeError, error_msg
    else:
        q_min = (n_data_sets-1)*[None]
        q_max = (n_data_sets-1)*[None]

    # Prepare the data sets
    s = Stitcher()

    for i in range(n_data_sets):
        d = DataSet(data_list[i])
        d.load(True)
        # Set the Q range to be used to stitch
        xmin, xmax = d.get_range()
        if is_q_range_limited:
            if i==0:
                xmax = q_max[i]
            elif i<n_data_sets-1:
                xmin = q_min[i-1]
                xmax = q_max[i]
            elif i==n_data_sets-1:
                xmin = q_min[i-1]

        d.set_range(xmin, xmax)

        # Set the scale of the reference data as needed
        if has_scale_factors:
            d.set_scale(float(scale[i]))
        elif i==0 and type(scale) in [int, float]:
            d.set_scale(scale)

        s.append(d)

    # Set the reference data (index of the data set in the workspace list)
    s.set_reference(0)
    if not has_scale_factors:
        s.compute()

    # Now that we have the scaling factors computed, simply apply them (not very pretty...)
    for i in range(n_data_sets):
        d = s.get_data_set(i)
        xmin, xmax = d.get_range()
        if i>0:
            xmin = q_min[i-1]
        if i<n_data_sets-1:
            xmax = q_max[i]

        d.apply_scale(xmin, xmax)

    # Create combined output
    if output_workspace is not None:
        s.get_scaled_data(workspace=output_workspace)

    # Save output to a file
    if save_output:
        if output_workspace is None:
            output_workspace = "combined_scaled_Iq"
        s.save_combined(output_workspace+".xml", as_canSAS=True, workspace=output_workspace)
