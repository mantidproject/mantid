"""
    Implementation absolute scale calculation for reactor SANS instruments
    
    #FIXME: Allow the selection of a box around the beam
     
"""
import os
import sys
import math
from reduction import ReductionStep
from reduction import extract_workspace_name
from reduction import validate_step

# Mantid imports
from mantidsimple import *

class BaseAbsoluteScale(ReductionStep):
    """

    """
    def __init__(self, scaling_factor=1.0, error=0.0):
        """
            Default scaling factor entered by hand
            @param scaling_factor: scaling factor
            @param error: error on the scaling factor
        """
        super(BaseAbsoluteScale, self).__init__()
        self._scaling_factor = scaling_factor
        self._error = error
        
    def get_scaling_factor(self):
        """
            Returns the beam center
        """
        return self._scaling_factor
    
    def execute(self, reducer, workspace=None):
        """
            Do nothing since the scaling factor was set by hand
        """
        return "Absolute scale factor: %6.4g" % self._scaling_factor
    
    
class AbsoluteScale(BaseAbsoluteScale):
    """
    """
    def __init__(self, data_file, beamstop_radius=None, attenuator_trans=1.0, 
                 sample_thickness=1.0, apply_sensitivity=False):
        """
            @param beamstop_radius: beamstop radius to use. Will otherwise be read from file, if possible [mm]
            @param attenuator_trans: attenuator transmission
            
            TODO: Read attenuator transmission from file
        """
        super(AbsoluteScale, self).__init__()
        self._data_file = data_file
        self._attenuator_trans = attenuator_trans
        self._beamstop_radius = beamstop_radius
        self._apply_sensitivity = apply_sensitivity
        self._sample_thickness = sample_thickness
        
    def execute(self, reducer, workspace=None):
        """
            @param reducer: Reducer object for which this step is executed
        """

        # Sanity check
        if self._data_file is None:
            raise RuntimeError, "AbsoluteScale called with no defined direct beam file"

        # Load data file
        filepath = reducer._full_file_path(self._data_file)
        data_file_ws = "_abs_scale_"+extract_workspace_name(filepath)
        
        loader = reducer._data_loader.clone(data_file=filepath)
        loader.set_beam_center(reducer.get_beam_center())
        loader.execute(reducer, data_file_ws)        
        
        # Get counting time
        timer = mtd[data_file_ws].dataY(reducer.NORMALIZATION_TIME)[0]
        monitor = mtd[data_file_ws].dataY(reducer.NORMALIZATION_MONITOR)[0]
        
        sdd_property = mtd[data_file_ws].getRun().getProperty("sample_detector_distance")
        if sdd_property is not None:
            sdd = sdd_property.value
        else:
            raise RuntimeError, "AbsoluteScale could not read the sample-detector-distance"
        
        if self._beamstop_radius is None:
            beamstop_property = mtd[data_file_ws].getRun().getProperty("beam-trap-radius")
            if beamstop_property is not None:
                self._beamstop_radius = beamstop_property.value
            else:
                raise RuntimeError, "AbsoluteScale could not read the beam stop radius and none was provided"
        
        # Apply sensitivity correction
        if self._apply_sensitivity and reducer.get_sensitivity_correcter() is not None:
            reducer.get_sensitivity_correcter().execute(data_file_ws)
        
        det_count = 1
        cylXML = '<infinite-cylinder id="asbsolute_scale">' + \
                   '<centre x="0.0" y="0.0" z="0.0" />' + \
                   '<axis x="0.0" y="0.0" z="1.0" />' + \
                   '<radius val="%12.10f" />' % (self._beamstop_radius/1000.0) + \
                 '</infinite-cylinder>\n'
                 
        det_finder = FindDetectorsInShape(Workspace=data_file_ws, ShapeXML=cylXML)
        det_list = det_finder.getPropertyValue("DetectorList")
        GroupDetectors(InputWorkspace=data_file_ws,  OutputWorkspace="_absolute_scale",  DetectorList=det_list, KeepUngroupedSpectra="0")
        det_count = mtd["_absolute_scale"].dataY(0)[0]
        
        # Pixel size, in mm
        pixel_size_param = mtd[data_file_ws].getInstrument().getNumberParameter("x-pixel-size")
        if pixel_size_param is not None:
            pixel_size = pixel_size_param[0]
        else:
            raise RuntimeError, "AbsoluteScale could not read the pixel size"
        
        if self._sample_thickness<=0:
            raise RuntimeError, "Invalid value for sample thickness: %g cm" % self._sample_thickness
            
        self._scaling_factor = 1.0/(self._sample_thickness*det_count/timer/self._attenuator_trans/(monitor/timer)*(pixel_size/sdd)*(pixel_size/sdd))

        return "Absolute scale factor: %6.4g" % self._scaling_factor
    
        

            
