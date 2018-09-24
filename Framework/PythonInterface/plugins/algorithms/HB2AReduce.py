from __future__ import (absolute_import, division, print_function)
from mantid.api import (PythonAlgorithm, AlgorithmFactory, PropertyMode, WorkspaceProperty, FileProperty,
                        FileAction, MultipleFileProperty)
from mantid.kernel import (Direction, IntArrayProperty, FloatTimeSeriesProperty, FloatBoundedValidator,
                           EnabledWhenProperty, PropertyCriterion, Property)
from mantid import logger
import numpy as np
import datetime
import os
import re
import warnings


class HB2AReduce(PythonAlgorithm):
    _gaps = np.array([0.   ,    2.641,    5.287,    8.042,   10.775,   13.488,
                      16.129,   18.814,   21.551,   24.236,   26.988,   29.616,
                      32.312,   34.956,   37.749,   40.4  ,   43.111,   45.839,
                      48.542,   51.207,   53.938,   56.62 ,   59.286,   61.994,
                      64.651,   67.352,   70.11 ,   72.765,   75.492,   78.204,
                      80.917,   83.563,   86.279,   88.929,   91.657,   94.326,
                      97.074,   99.784,  102.494,  105.174,  107.813,  110.551,
                      113.25 ,  115.915])

    def category(self):
        return 'Diffraction\\Reduction'

    def seeAlso(self):
        return []

    def name(self):
        return 'HB2AReduce'

    def summary(self):
        return 'Performs data reduction for HB-2A POWDER at HFIR'

    def PyInit(self):
        self.declareProperty(MultipleFileProperty(name="Filename", action=FileAction.OptionalLoad,
                                                  extensions=[".dat"]), "Data files to load")
        condition = EnabledWhenProperty("Filename", PropertyCriterion.IsDefault)
        self.declareProperty('IPTS', Property.EMPTY_INT, "IPTS number to load from")
        self.setPropertySettings("IPTS", condition)
        self.declareProperty('Exp', Property.EMPTY_INT, "Experiment number to load from")
        self.setPropertySettings("Exp", condition)
        self.declareProperty(IntArrayProperty("ScanNumbers", []), 'Scan numbers to load')
        self.setPropertySettings("ScanNumbers", condition)
        self.declareProperty(FileProperty(name="Vanadium", defaultValue="", action=FileAction.OptionalLoad, extensions=[".dat", ".txt"]),
                             doc="Vanadium file, can be either the vanadium scan file or the reduced vcorr file. "
                             "If not provided the vcorr file adjacent to the data file will be used")
        self.declareProperty('Normalise', True, "If False vanadium normalisation will not be performed")
        self.declareProperty(IntArrayProperty("ExcludeDetectors", []),
                             doc="Detectors to exclude. If not provided the HB2A_exp???__exclude_detectors.txt adjacent "
                             "to the data file will be used if it exist")
        self.declareProperty('DefX', '',
                             "By default the def_x (x-axis) from the file will be used, it can be overridden by setting it here")
        self.declareProperty('IndividualDetectors', False,
                             "If True the workspace will include each anode as a separate spectrum, useful for debugging issues")
        condition = EnabledWhenProperty("IndividualDetectors", PropertyCriterion.IsDefault)
        self.declareProperty('BinData', True, "Data will be binned using BinWidth. If False then all data will be unbinned")
        self.setPropertySettings("BinData", condition)
        positiveFloat = FloatBoundedValidator(lower=0., exclusive=True)
        self.declareProperty('BinWidth', 0.05, positiveFloat, "Bin size of the output workspace")
        self.setPropertySettings("BinWidth", condition)
        self.declareProperty('Scale', 1.0, positiveFloat, "The output will be scaled by this value")
        self.declareProperty(WorkspaceProperty("OutputWorkspace", "",
                                               optional=PropertyMode.Mandatory,
                                               direction=Direction.Output),
                             "Output Workspace")

    def validateInputs(self):
        issues = dict()

        if not self.getProperty("Filename").value:
            ipts = self.getProperty("IPTS").value

            if ((ipts == Property.EMPTY_INT) or len(self.getProperty("ScanNumbers").value) is 0):
                issues["Filename"] = 'Must specify either Filename or IPTS AND ScanNumbers'

            if self.getProperty("Exp").value == Property.EMPTY_INT:
                exp_list = sorted(e for e in os.listdir('/HFIR/HB2A/IPTS-{0}'.format(ipts)) if 'exp' in e)
                if len(exp_list)>1:
                    exps = ','.join(e.replace('exp','') for e in exp_list)
                    issues["Exp"] = 'Multiple experiments found in IPTS-{}. You must set Exp to one of {}'.format(ipts, exps)

        return issues

    def PyExec(self):
        scale = self.getProperty("Scale").value
        filenames = self.getProperty("Filename").value

        if not filenames:
            ipts = self.getProperty("IPTS").value
            exp = self.getProperty("Exp").value
            if self.getProperty("Exp").value == Property.EMPTY_INT:
                exp = int([e for e in os.listdir('/HFIR/HB2A/IPTS-{0}'.format(ipts)) if 'exp' in e][0].replace('exp',''))
            filenames = ['/HFIR/HB2A/IPTS-{0}/exp{1}/Datafiles/HB2A_exp{1:04}_scan{2:04}.dat'.format(ipts, exp, scan)
                         for scan in self.getProperty("ScanNumbers").value]

        metadata = None
        data = None

        # Read in data array and append all files
        for filename in filenames:
            # Read in all lines once
            with open(filename) as f:
                lines = f.readlines()

            if metadata is None:
                # Read in metadata from first file only file
                metadata = dict([np.char.strip(re.split('#(.*?)=(.*)', line, flags=re.U)[1:3])
                                 for line in lines if re.match('^#.*=', line)])
                # Get indir and exp from first file
                indir, data_filename = os.path.split(filename)
                _, exp, _ = data_filename.replace(".dat", "").split('_')

            # Find size of header, the size changes
            header = np.argmax([bool(re.match('(?!^#)', line)) for line in lines])-1
            if header < 0:
                raise RuntimeError("{} has no data in it".format(filename))
            names = lines[header].split()[1:]

            try:
                d = np.loadtxt(lines[header:], ndmin=1, dtype={'names': names, 'formats':[float]*len(names)})
            except (ValueError, IndexError):
                raise RuntimeError("Could not read {}, file likely malformed".format(filename))

            # Accumulate data
            data = d if data is None else np.append(data, d)

        # Get any masked detectors
        detector_mask = self.get_detector_mask(exp, indir)

        counts = np.array([data['anode{}'.format(n)] for n in range(1,45)])[detector_mask]
        twotheta = data['2theta']
        monitor = data['monitor']

        # Remove points with zero monitor count
        monitor_mask = np.nonzero(monitor)[0]
        if len(monitor_mask) == 0:
            raise RuntimeError("{} has all zero monitor counts".format(filename))
        monitor = monitor[monitor_mask]
        counts = counts[:, monitor_mask]
        twotheta = twotheta[monitor_mask]

        # Get either vcorr file or vanadium data
        vanadium_count, vanadium_monitor, vcorr = self.get_vanadium(detector_mask,
                                                                    data['m1'][0], data['colltrans'][0],
                                                                    exp, indir)

        def_x = self.getProperty("DefX").value
        if not def_x:
            def_x = metadata['def_x']

        if def_x not in data.dtype.names:
            logger.warning("Could not find {} property in datafile, using 2theta instead".format(def_x))
            def_x = '2theta'

        if def_x == '2theta':
            x = twotheta+self._gaps[:, np.newaxis][detector_mask]
            UnitX='Degrees'
        else:
            x = np.tile(data[def_x], (44,1))[detector_mask][:, monitor_mask]
            UnitX=def_x

        if self.getProperty("IndividualDetectors").value:
            # Separate spectrum per anode
            y, e = self.process(counts, scale, monitor, vanadium_count, vanadium_monitor, vcorr)
            NSpec=len(x)
        else:
            if self.getProperty("BinData").value:
                # Data binned with bin
                x, y, e = self.process_binned(counts, x.ravel(), scale, monitor, vanadium_count, vanadium_monitor, vcorr)
            else:
                y, e = self.process(counts, scale, monitor, vanadium_count, vanadium_monitor, vcorr)
            NSpec=1

        createWS_alg = self.createChildAlgorithm("CreateWorkspace", enableLogging=False)
        createWS_alg.setProperty("DataX", x)
        createWS_alg.setProperty("DataY", y)
        createWS_alg.setProperty("DataE", e)
        createWS_alg.setProperty("NSpec", NSpec)
        createWS_alg.setProperty("UnitX", UnitX)
        createWS_alg.setProperty("YUnitLabel", "Counts")
        createWS_alg.setProperty("WorkspaceTitle", str(metadata['scan_title']))
        createWS_alg.execute()
        outWS = createWS_alg.getProperty("OutputWorkspace").value

        self.setProperty("OutputWorkspace", outWS)

        self.add_metadata(outWS, metadata, data)

    def get_detector_mask(self, exp, indir):
        """Returns an anode mask"""
        detector_mask = np.ones(44, dtype=bool)
        if len(self.getProperty("ExcludeDetectors").value) == 0:
            exclude_filename = os.path.join(indir, 'HB2A_{}__exclude_detectors.txt'.format(exp))
            if os.path.isfile(exclude_filename):
                with warnings.catch_warnings():
                    warnings.simplefilter("ignore")
                    exclude_detectors = np.loadtxt(exclude_filename, ndmin=1, dtype=int)
            else:
                exclude_detectors=np.empty(0, dtype=int)
        else:
            exclude_detectors = np.array(self.getProperty("ExcludeDetectors").value)
        if len(exclude_detectors) > 0:
            logger.notice("Excluding anodes: {}".format(exclude_detectors))
        detector_mask[exclude_detectors-1] = False
        return detector_mask

    def get_vanadium(self, detector_mask, m1, colltrans, exp, indir):
        """
        This function returns either (vanadium_count, vanadium_monitor, None) or
        (None, None, vcorr) depending what type of file is provided by getProperty("Vanadium")
        """
        if not self.getProperty("Normalise").value:
            return None, None, np.ones(44)[detector_mask]

        vanadium_filename = self.getProperty("Vanadium").value
        if vanadium_filename:
            if vanadium_filename.split('.')[-1] == 'dat':
                vanadium = np.genfromtxt(vanadium_filename)
                vanadium_count = vanadium[:, 5:49].sum(axis=0)[detector_mask]
                vanadium_monitor = vanadium[:, 3].sum()
                logger.notice("Using vanadium data file: {}".format(vanadium_filename))
                return vanadium_count, vanadium_monitor, None
            else:
                vcorr_filename = vanadium_filename
        else: # Find adjacent vcorr file
            # m1 is the monochromator angle
            # m1 = 0 -> Ge 115, 1.54A
            # m1 = 9.45 -> Ge 113, 2.41A
            # colltrans is the collimator position, whether in or out of the beam
            # colltrans = 0 -> IN
            # colltrans = +/-80 -> OUT
            vcorr_filename = 'HB2A_{}__Ge_{}_{}_vcorr.txt'.format(exp,
                                                                  115 if np.isclose(m1, 0, atol=0.1) else 113,
                                                                  "IN" if np.isclose(colltrans, 0, atol=0.1) else "OUT")
        vcorr_filename = os.path.join(indir, vcorr_filename)
        logger.notice("Using vcorr file: {}".format(vcorr_filename))
        if not os.path.isfile(vcorr_filename):
            raise RuntimeError("Vanadium file {} does not exist".format(vcorr_filename))

        return None, None, np.genfromtxt(vcorr_filename)[detector_mask]

    def process(self, counts, scale, monitor, vanadium_count=None, vanadium_monitor=None, vcorr=None):
        """Reduce data not binning"""
        old_settings = np.seterr(all='ignore') # otherwise it will complain about divide by zero
        if vcorr is not None:
            y = counts/vcorr[:, np.newaxis]/monitor
            e = np.sqrt(counts)/vcorr[:, np.newaxis]/monitor
        else:
            y = counts/vanadium_count[:, np.newaxis]*vanadium_monitor/monitor
            e = np.sqrt(1/counts + 1/vanadium_count[:, np.newaxis] + 1/vanadium_monitor + 1/monitor)*y
        np.seterr(**old_settings)
        return np.nan_to_num(y*scale), np.nan_to_num(e*scale)

    def process_binned(self, counts, x, scale, monitor, vanadium_count=None, vanadium_monitor=None, vcorr=None):
        """Bin the data"""
        binWidth = self.getProperty("BinWidth").value
        bins = np.arange(x.min(), x.max()+binWidth, binWidth) # calculate bin boundaries
        inds = np.digitize(x, bins) # get bin indices

        if vcorr is not None:
            vcorr = np.tile(vcorr, (counts.shape[1], 1)).T
            vcorr_binned = np.bincount(inds, weights=vcorr.ravel(), minlength=len(bins))
        else:
            vanadium_count = np.tile(vanadium_count, (counts.shape[1], 1)).T
            vanadium_binned = np.bincount(inds, weights=vanadium_count.ravel(), minlength=len(bins))
            vanadium_monitor_binned = np.bincount(inds, minlength=len(bins))*vanadium_monitor

        monitor = np.tile(monitor, (counts.shape[0], 1))

        counts_binned = np.bincount(inds, weights=counts.ravel(), minlength=len(bins))
        monitor_binned = np.bincount(inds, weights=monitor.ravel(), minlength=len(bins))
        number_binned = np.bincount(inds, minlength=len(bins))

        old_settings = np.seterr(all='ignore') # otherwise it will complain about divide by zero
        if vcorr is not None:
            y = (counts_binned/vcorr_binned*number_binned/monitor_binned)[1:]
            e = (np.sqrt(1/counts_binned)[1:])*y
        else:
            y = (counts_binned/vanadium_binned*vanadium_monitor_binned/monitor_binned)[1:]
            e = (np.sqrt(1/counts_binned + 1/vanadium_binned + 1/vanadium_monitor + 1/monitor_binned)[1:])*y
        np.seterr(**old_settings)
        x = bins

        return x, np.nan_to_num(y*scale), np.nan_to_num(e*scale)

    def add_metadata(self, ws, metadata, data):
        """Adds metadata to the workspace"""
        run = ws.getRun()

        # Just copy all metadata in the file
        for key in metadata.keys():
            run.addProperty(key, str(metadata[key]), True)

        # Add correct start and end time
        start_time = np.datetime64(datetime.datetime.strptime(metadata['time']+' '+metadata['date'], '%I:%M:%S %p %m/%d/%Y'))
        run.addProperty('start_time', str(start_time), True)

        # Create time array for time series logs
        time_array = start_time + np.cumsum(data['time'], dtype=np.int64)*np.timedelta64(1,'s')
        run.addProperty('end_time', str(time_array[-1]), True)
        run.addProperty('duration', float((time_array[-1]-time_array[0])/np.timedelta64(1, 's')), True)

        # Create time series logs for the scan variables
        for name in data.dtype.names:
            if 'anode' not in name:
                log = FloatTimeSeriesProperty(name)
                for t, v in zip(time_array, data[name]):
                    log.addValue(t, v)
                run[name]=log


AlgorithmFactory.subscribe(HB2AReduce)
