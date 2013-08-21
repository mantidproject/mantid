"""
    Implementation of loading reduction steps for HFIR SANS
"""
import sys
from reduction import ReductionStep
from reduction import find_data
from reduction import validate_step
from sans_reduction_steps import BaseBeamFinder

# Mantid imports
from mantid.simpleapi import *

class LoadRun(ReductionStep):
    """
        Load a data file, move its detector to the right position according
        to the beam center and normalize the data.
    """
    def __init__(self, datafile=None, sample_det_dist=None, sample_det_offset=0, beam_center=None,
                 wavelength=None, wavelength_spread=None, mask_side=None):
        """
            @param datafile: file path of data to load
            @param sample_det_dist: sample-detector distance [mm] (will overwrite header info)
            @param sample_det_offset: sample-detector distance offset [mm]
            @param beam_center: [center_x, center_y] [pixels]
            @param wavelength: if provided, wavelength will be fixed at that value [A]
            @param wavelength_spread: if provided along with wavelength, it will be fixed at that value [A] 
            @param mask_side: mask the front detector side if 0, the back side if 1, or no masking if None
        """
        super(LoadRun, self).__init__()
        self._data_file = datafile
        self.set_sample_detector_distance(sample_det_dist)
        self.set_sample_detector_offset(sample_det_offset)
        self.set_beam_center(beam_center)
        self.set_wavelength(wavelength, wavelength_spread)
        self.mask_detector_side(mask_side)
        
    def clone(self, data_file=None):
        if data_file is None:
            data_file = self._data_file
        return LoadRun(datafile=data_file, sample_det_dist=self._sample_det_dist,
                       sample_det_offset=self._sample_det_offset, beam_center=self._beam_center,
                       wavelength=self._wavelength, wavelength_spread=self._wavelength_spread,
                       mask_side=self._mask_side)
        
    def mask_detector_side(self, side=None):
        """
            Select a detector side to mask
            0 = front side
            1 = back side
        """
        if side is None or side in [0, 1]:
            self._mask_side = side
        else:
            raise RuntimeError, "LoadRun.mask_detector_side expects None, 0, or 1"
        
    def set_wavelength(self, wavelength=None, wavelength_spread=None):
        if wavelength is not None and type(wavelength) != int and type(wavelength) != float:
            raise RuntimeError, "LoadRun.set_wavelength expects a float: %s" % str(wavelength)
        self._wavelength = wavelength

        if wavelength_spread is not None and type(wavelength_spread) != int and type(wavelength_spread) != float:
            raise RuntimeError, "LoadRun.set_wavelength expects a float for wavelength_spread: %s" % str(wavelength_spread)
        self._wavelength_spread = wavelength_spread
        
    def set_sample_detector_distance(self, distance):
        # Check that the distance given is either None of a float
        if distance is not None and type(distance) != int and type(distance) != float:
            raise RuntimeError, "LoadRun.set_sample_detector_distance expects a float: %s" % str(distance)
        self._sample_det_dist = distance
        
    def set_sample_detector_offset(self, offset):
        # Check that the offset given is either None of a float
        if offset is not None and type(offset) != int and type(offset) != float:
            raise RuntimeError, "LoadRun.set_sample_detector_offset expects a float: %s" % str(offset)
        self._sample_det_offset = offset
        
    def set_beam_center(self, beam_center):
        """
            Sets the beam center to be used when loading the file
            @param beam_center: [pixel_x, pixel_y]
        """
        if beam_center is None or beam_center == [None, None]:
            self._beam_center = beam_center
        
        # Check that we have pixel numbers (int)            
        elif type(beam_center) == list:
            if len(beam_center) == 2:
                try:
                    int(beam_center[0])
                    int(beam_center[1])
                    self._beam_center = [beam_center[0], beam_center[1]]
                except:
                    raise RuntimeError, "LoadRun.set_beam_center expects a list of two integers\n  %s" % sys.exc_value
            else:
                raise RuntimeError, "LoadRun.set_beam_center expects a list of two integers. Found length %d" % len(beam_center)
            
        else:
            raise RuntimeError, "LoadRun.set_beam_center expects a list of two integers. Found %s" % type(beam_center)
        
    def execute(self, reducer, inputworkspace, outputworkspace=None):
        """
            Loads a data file.
            Note: Files are ALWAYS reloaded when this method is called.
            We do this because speed is not an issue and we ensure that the data
            is always pristine. We could only load files that are not already loaded
            by using the 'dirty' flag and checking for the existence of the workspace.
        """
        output_str = ""
        if outputworkspace is not None:
            workspace = outputworkspace 
        else:
            workspace = inputworkspace
        # If we don't have a data file, look up the workspace handle
        if self._data_file is None:
            if workspace in reducer._data_files:
                data_file = reducer._data_files[workspace]
            elif workspace in reducer._extra_files:
                data_file = reducer._extra_files[workspace]
            else:
                raise RuntimeError, "SANSReductionSteps.LoadRun doesn't recognize workspace handle %s" % workspace
        else:
            data_file = self._data_file
        
        def _load_data_file(file_name, wks_name):
            filepath = find_data(file_name, instrument=reducer.instrument.name())
            beam_x = None
            beam_y = None
            if self._beam_center is not None:            
                [beam_x, beam_y] = self._beam_center
            else:
                [beam_x, beam_y] = reducer.get_beam_center()
                
            wksp, msg = HFIRLoad(Filename=filepath, OutputWorkspace=wks_name,
                         BeamCenterX = beam_x,
                         BeamCenterY = beam_y,
                         SampleDetectorDistance = self._sample_det_dist,
                         SampleDetectorDistanceOffset = self._sample_det_offset,
                         Wavelength = self._wavelength,
                         WavelengthSpread = self._wavelength_spread,
                         ReductionProperties=reducer.get_reduction_table_name())
            return msg

        # Check whether we have a list of files that need merging
        #   Make sure we process a list of files written as a string
        if type(data_file)==str:
            data_file = find_data(data_file, instrument=reducer.instrument.name(), allow_multiple=True)
        if type(data_file)==list:
            monitor = 0.0
            timer = 0.0 
            for i in range(len(data_file)):
                output_str += "Loaded %s\n" % data_file[i]
                if i==0:
                    output_str += _load_data_file(data_file[i], workspace)
                else:
                    output_str += _load_data_file(data_file[i], '__tmp_wksp')
                    Plus(LHSWorkspace=workspace,
                         RHSWorkspace='__tmp_wksp',
                         OutputWorkspace=workspace)
                    # Get the monitor and timer values
                    monitor += mtd['__tmp_wksp'].getRun().getProperty("monitor").value
                    timer += mtd['__tmp_wksp'].getRun().getProperty("timer").value
            
            # Get the monitor and timer of the first file, which haven't yet
            # been added to the total
            monitor += mtd[workspace].getRun().getProperty("monitor").value
            timer += mtd[workspace].getRun().getProperty("timer").value
                    
            # Update the timer and monitor
            mtd[workspace].mutableRun().addProperty("monitor", monitor, True)
            mtd[workspace].mutableRun().addProperty("timer", timer, True)
            
            if mtd.doesExist('__tmp_wksp'):
                DeleteWorkspace('__tmp_wksp')
        else:
            output_str += "Loaded %s\n" % data_file
            output_str += _load_data_file(data_file, workspace)
                
        if mtd[workspace].getRun().hasProperty("beam_center_x") and \
            mtd[workspace].getRun().hasProperty("beam_center_y"):
            beam_center_x = mtd[workspace].getRun().getProperty("beam_center_x").value
            beam_center_y = mtd[workspace].getRun().getProperty("beam_center_y").value
            if type(reducer._beam_finder) is BaseBeamFinder:
                reducer.set_beam_finder(BaseBeamFinder(beam_center_x, beam_center_y))
                logger.notice("No beam finding method: setting to default [%-6.1f, %-6.1f]" % (beam_center_x, beam_center_y))
        
        n_files = 1
        if type(data_file)==list:
            n_files = len(data_file)
            
        # Mask the back side or front side as needed
        # Front detector IDs
        if self._mask_side is not None and self._mask_side in [0, 1]:
            nx = int(mtd[workspace].getInstrument().getNumberParameter("number-of-x-pixels")[0])
            ny = int(mtd[workspace].getInstrument().getNumberParameter("number-of-y-pixels")[0])
            id_side = []
            
            for iy in range(ny):
                for ix in range(self._mask_side, nx+self._mask_side, 2):
                    id_side.append([iy,ix])

            det_side = reducer.instrument.get_detector_from_pixel(id_side, workspace)
            MaskDetectors(Workspace=workspace, DetectorList=det_side) 
        
        return output_str
