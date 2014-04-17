"""*WIKI* 
USANS Reduction
*WIKI*"""
from mantid.simpleapi import *
from mantid.api import *
from mantid.kernel import *
from reduction_workflow.find_data import find_data
import math
import numpy
import sys

class USANSReduction(PythonAlgorithm):

    def category(self):
        return "SANS"

    def name(self):
        return "USANSReduction"

    def PyInit(self):
        arrvalidator = IntArrayBoundedValidator()
        arrvalidator.setLower(0)
        self.declareProperty(IntArrayProperty("RunNumbers", values=[0], validator=arrvalidator,
                             direction=Direction.Input), "Runs to reduce")
        self.declareProperty("EmptyRun", '', "Run number for the empty run")

        #TODO: Mask workspace
        
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", Direction.Output), "Output workspace")
        
    class DataFile(object):
        def __init__(self, workspace, monitor, empty, empty_monitor, is_scan=False, max_index=1):
            self.workspace = workspace
            self.monitor = monitor
            self.empty = empty
            self.empty_monitor = empty_monitor
            self.is_scan = is_scan
            self.max_index = max_index
        
    def _load_data(self):
        """
            Load data and go through each file to determine how many points 
            will have to be dealt with.
        """
        # Load the empty run
        empty_run = self.getProperty("EmptyRun").value
        data_file = find_data(empty_run, instrument='USANS')
        Load(Filename=data_file, OutputWorkspace='__empty')
        mon_file = data_file.replace('.nxs', '_monitors.nxs')
        Load(Filename=mon_file, OutputWorkspace='__empty_monitors')
        
        # Get the runs to reduce
        run_list = self.getProperty("RunNumbers").value
        
        # Total number of measurements per wavelength peak
        total_points = 0
        # Load all files so we can determine how many points we have
        self.data_files = []
        for item in run_list:
            data_file = find_data(item, instrument='USANS')
            ws_name = '__sample_%s' % item
            Load(Filename=data_file, OutputWorkspace=ws_name)
            
            # A simple Load doesn't load the instrument properties correctly with our test file
            # Reload the instrument for now
            LoadInstrument(Workspace=ws_name, InstrumentName='USANS', RewriteSpectraMap=False)
            mon_file = data_file.replace('.nxs', '_monitors.nxs')
            Load(Filename=mon_file, OutputWorkspace=ws_name+'_monitors')
            
            # If we don't have the wavelength peaks, get them now
            if self.wl_list is None:
                wl_str = mtd[ws_name].getInstrument().getStringParameter("wavelength_peaks")[0]
                self.wl_list = wl_str.split(',')
           
                #TODO: make this part of the instrument parameters
                self.peaks = ((4800,6000), (6200, 7525), (8225, 9975), (12775, 14438), (26425, 28175))
            
            # Determine whether we are putting together multiple files or whether
            # we will be looking for scan_index markers.
            is_scan = False
            max_index = 1
            if mtd[ws_name].getRun().hasProperty('scan_index'):
                scan_index = mtd[ws_name].getRun().getProperty("scan_index").value
                if len(scan_index)>0:
                    _max_index = scan_index.getStatistics().maximum
                    if _max_index>0:
                        max_index = _max_index
                        is_scan = True
                
            # Append the info for when we do the reduction
            self.data_files.append(self.DataFile(workspace=ws_name,
                                            monitor=ws_name+'_monitors',
                                            empty='__empty',
                                            empty_monitor='__empty_monitors',
                                            is_scan=is_scan,
                                            max_index=max_index))
            total_points += max_index
             
        return total_points
        
    def _process_data_file(self, file_info, index_offset):
        # Go through each point
        Logger.get("USANSReduction").warning("Reduction %s: %s points (offset=%s)" % (file_info.workspace, file_info.max_index, index_offset))
        for point in range(file_info.max_index):
            # If we are in a scan, select the current scan point
            if file_info.is_scan:
                ws=FilterByLogValue(InputWorkspace=mtd[file_info.workspace],
                                    LogName='scan_index',
                                    MinimumValue=point,
                                    MaximumValue=point,
                                    LogBoundary='Left')
            else:
                ws = mtd[file_info.workspace]
                
            # Get the two-theta value for this point
            if ws.getRun().getProperty("two_theta").type=='number':
                two_theta = ws.getRun().getProperty("two_theta").value
            else:
                two_theta = ws.getRun().getProperty("two_theta").timeAverageValue()
        
            # Loop through the wavelength peaks for this point
            for i_wl in range(len(self.wl_list)):
                if len(self.wl_list[i_wl].strip())==0:
                    continue
                wl = float(self.wl_list[i_wl].strip())
                tof = 30.0/0.0039560*wl
                q = 6.28*math.sin(two_theta)/wl
                
                # Get I(q) for each wavelength peak
                i_q = None
                for peak in self.peaks:
                    if tof>peak[0] and tof<peak[1]:
                        i_q = self._get_intensity(mtd[file_info.workspace],
                                                  mtd[file_info.empty],
                                                  mtd[file_info.monitor],
                                                  mtd[file_info.empty_monitor],
                                                  tof_min=peak[0], tof_max=peak[1])
                if i_q is None:
                    Logger.get("USANSReduction").error("Unable to find TOF peak for wl=%s" % wl)
                    continue
                    
                # Store the reduced data  
                try:
                    self.q_output[i_wl][point+index_offset] = q
                    self.iq_output[i_wl][point+index_offset] = i_q.dataY(0)[0]
                    self.iq_err_output[i_wl][point+index_offset] = i_q.dataE(0)[0]
                except:
                    Logger.get("USANSReduction").error("Exception caught for %s on peak %s, point %s. Offset=%s" % (file_info.workspace, i_wl, point, index_offset))
                    Logger.get("USANSReduction").error("Array: %s x %s    Data: %s" % (len(self.wl_list), self.total_points, file_info.max_index))
                    Logger.get("USANSReduction").error(sys.exc_value)
                
        return file_info.max_index
        
    def PyExec(self):
        # Placeholder for the list of wavelength peaks
        self.wl_list = None
        
        # Placeholder for the data file information
        self.data_files = []
        
        # Total number of measurements per wavelength peak
        self.total_points = self._load_data()

        # Create an array to store the I(q) points
        n_wl = len(self.wl_list)
        Logger.get("USANSReduction").notice("USANS reduction for %g peaks with %g point(s) each" % (n_wl, self.total_points))
        self.q_output = numpy.zeros(shape=(n_wl, self.total_points))
        self.iq_output = numpy.zeros(shape=(n_wl, self.total_points))
        self.iq_err_output = numpy.zeros(shape=(n_wl, self.total_points))
        
        index_offset = 0
        for file in self.data_files:
            index_offset += self._process_data_file(file, index_offset)
        
        # Create a workspace for each peak
        self._aggregate()

    def _aggregate(self):
        """
            Create a workspace for each peak
            #TODO: stitch the data instead of just dumping them in a workspace
        """
        x_all = []
        y_all = []
        e_all = []
        for i_wl in range(len(self.wl_list)):
            x_all.extend(self.q_output[i_wl])
            y_all.extend(self.iq_output[i_wl])
            e_all.extend(self.iq_err_output[i_wl])
            x = self.q_output[i_wl]
            y = self.iq_output[i_wl]
            e = self.iq_err_output[i_wl]
            
            # Sort the I(q) point just in case we got them in the wrong order
            zipped = zip(x,y,e)
            def cmp(p1,p2):
                if p2[0]==p1[0]:
                    return 0
                return -1 if p2[0]>p1[0] else 1
            combined = sorted(zipped, cmp)
            x,y,e = zip(*combined)
            
            wl = float(self.wl_list[i_wl].strip())
            CreateWorkspace(DataX=x,
                            DataY=y,
                            DataE=e,
                            NSpec=1, UnitX='MomentumTransfer',
                            OutputWorkspace='iq_%1.2f' % wl)

        # Sort the I(q) point just in case we got them in the wrong order
        zipped = zip(x_all,y_all,e_all)
        def cmp(p1,p2):
            if p2[0]==p1[0]:
                return 0
            return -1 if p2[0]>p1[0] else 1
        combined = sorted(zipped, cmp)
        x,y,e = zip(*combined)
        
        # Create the combined output workspace
        output_ws_name = self.getPropertyValue("OutputWorkspace")
        out_ws = CreateWorkspace(DataX=x,
                                 DataY=y,
                                 DataE=e,
                                 NSpec=1, UnitX='MomentumTransfer',
                                 OutputWorkspace=output_ws_name)
        self.setProperty("OutputWorkspace", out_ws)

    def _get_intensity(self, sample, empty, sample_monitor, empty_monitor, tof_min, tof_max):
        # Number of pixels we are dealing with
        nspecs = sample.getNumberHistograms()
        
        # Apply mask
        
        # Get the normalized empty run counts in the transmission detector
        __empty_summed = SumSpectra(InputWorkspace=empty, 
                                    StartWorkspaceIndex=nspecs/2, 
                                    EndWorkspaceIndex=nspecs-1)        
        __point = CropWorkspace(InputWorkspace=__empty_summed, XMin=tof_min, XMax=tof_max)
        __empty_count = Integration(InputWorkspace=__point)
        
        __point = CropWorkspace(InputWorkspace=empty_monitor, XMin=tof_min, XMax=tof_max)
        __empty_monitor_count = Integration(InputWorkspace=__point)
        
        __normalized_empty = __empty_count/__empty_monitor_count

        # Get the normalized sample counts in the transmission detector
        __trans_summed = SumSpectra(InputWorkspace=sample, 
                                    StartWorkspaceIndex=nspecs/2, 
                                    EndWorkspaceIndex=nspecs-1)
        __point = CropWorkspace(InputWorkspace=__trans_summed, XMin=tof_min, XMax=tof_max)
        __trans_count = Integration(InputWorkspace=__point)

        __point = CropWorkspace(InputWorkspace=sample_monitor, XMin=tof_min, XMax=tof_max)
        __monitor_count = Integration(InputWorkspace=__point)

        # The monitor count normalization cancels out when doing the transmission correction
        # of the scattering signal below
        __normalized_sample_trans = __trans_count#/__monitor_count
        
        # Transmission workspace
        transmission = __normalized_sample_trans/__normalized_empty
        
        # Scattering signal
        __signal_summed = SumSpectra(InputWorkspace=sample, 
                                     StartWorkspaceIndex=0, 
                                     EndWorkspaceIndex=nspecs/2)
        __point = CropWorkspace(InputWorkspace=__signal_summed, XMin=tof_min, XMax=tof_max)
        __signal_count = Integration(InputWorkspace=__point)
        # The monitor count normalization cancels out when doing the transmission correction
        __signal = __signal_count#/__monitor_count
        
        intensity = __signal/transmission
        return intensity

#############################################################################################
AlgorithmFactory.subscribe(USANSReduction())
