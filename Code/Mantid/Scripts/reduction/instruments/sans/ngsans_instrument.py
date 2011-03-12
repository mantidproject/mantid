"""
    Instrument class for NIST SANS reduction
"""
from reduction import Instrument
from hfir_instrument import HFIRSANS
from reduction import ReductionStep
from reduction import extract_workspace_name

# Mantid imports
import mantidsimple
from mantidsimple import mtd

class NGSANS(HFIRSANS):
    """
        NIST SANS instrument description
    """
    def __init__(self) :
        super(NGSANS, self).__init__(instrument_id="NGSANS")

    def get_detector_from_pixel(self, pixel_list):
        """
            Returns a list of detector IDs from a list of [x,y] pixels,
            where the pixel coordinates are in pixel units.
        """
        return [ self.nx_pixels*p[1] + p[0] + self.nMonitors for p in pixel_list ]

class NGSANSLoadRun(ReductionStep):
    """
        Load NIST SANS raw data
    """
    def __init__(self, datafile=None):
        super(NGSANSLoadRun, self).__init__()
        self._data_file = datafile
                
    def execute(self, reducer, workspace, force=False):      
        # If we don't have a data file, look up the workspace handle
        # Only files that are used for computing data corrections have
        # a path that is passed directly. Data files that are reduced
        # are simply found in reducer._data_files 
        if self._data_file is None:
            if workspace in reducer._data_files:
                data_file = reducer._data_files[workspace]
            elif workspace in reducer._extra_files:
                data_file = reducer._extra_files[workspace]
            else:
                raise RuntimeError, "NGSANS.LoadRun doesn't recognize workspace handle %s" % workspace
        else:
            data_file = self._data_file
        
        # Load data
        filepath = reducer._full_file_path(data_file)

        # Check whether that file was already loaded
        # The file also has to be pristine
        if mtd[workspace] is not None and not force and reducer.is_clean(workspace):
            mtd.sendLogMessage("Data %s is already loaded: delete it first if reloading is intended" % (workspace))
            return "Data %s is already loaded: delete it first if reloading is intended" % (workspace)

        mantidsimple.LoadNISTSANS(Filename=filepath, OutputWorkspace=workspace)
        sdd = mtd[workspace].getRun().getProperty("sample_detector_distance").value
        # Move the detector to its correct position
        mantidsimple.MoveInstrumentComponent(workspace, "detector1", Z=sdd/1000.0, RelativePosition=0)

        # Move detector array to correct position
        [pixel_ctr_x, pixel_ctr_y] = reducer.get_beam_center()
        if pixel_ctr_x is not None and pixel_ctr_y is not None:
            [beam_ctr_x, beam_ctr_y] = reducer.instrument.get_coordinate_from_pixel(pixel_ctr_x, pixel_ctr_y)
            [default_pixel_x, default_pixel_y] = reducer.instrument.get_default_beam_center()
            [default_x, default_y] = reducer.instrument.get_coordinate_from_pixel(default_pixel_x, default_pixel_y)
            mantidsimple.MoveInstrumentComponent(workspace, "detector1", 
                                    X = default_x-beam_ctr_x,
                                    Y = default_y-beam_ctr_y,
                                    RelativePosition="1")
        else:
            mtd.sendLogMessage("Beam center isn't defined: skipping beam center alignment for %s" % workspace)
 
        
        mtd.sendLogMessage("Loaded %s: sample-detector distance = %g" %(workspace, sdd))
        
        # Remove the dirty flag if it existed
        reducer.clean(workspace)
        return "Data file loaded: %s" % (workspace)

