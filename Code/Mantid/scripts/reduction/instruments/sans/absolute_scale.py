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
    def __init__(self, data_file, beamstop_diameter=None, attenuator_trans=1.0, 
                 sample_thickness=None, apply_sensitivity=False):
        """
            @param beamstop_diameter: beamstop diameter to use. Will otherwise be read from file, if possible [mm]
            @param attenuator_trans: attenuator transmission
            
            TODO: Read attenuator transmission from file
        """
        super(AbsoluteScale, self).__init__()
        self._data_file = data_file
        self._attenuator_trans = attenuator_trans
        self._beamstop_diameter = beamstop_diameter
        self._apply_sensitivity = apply_sensitivity
        self._sample_thickness = sample_thickness
        self._scaling_factor = None
        
    def execute(self, reducer, workspace=None):
        """
            @param reducer: Reducer object for which this step is executed
        """
        # If we haven't calculated the scaling factor, do it now
        if self._scaling_factor is None:
            self._compute_scaling_factor(reducer)
            
        if self._sample_thickness is not None:
            thickness = self._sample_thickness
        else:
            if mtd[workspace].getRun().hasProperty("sample-thickness"):
                thickness = mtd[workspace].getRun().getProperty("sample-thickness").value
            else:
                raise RuntimeError, "AbsoluteScale could not get the sample thickness"
                
        if thickness <= 0:
            raise RuntimeError, "Invalid value for sample thickness: %g cm" % thickness
        scale = self._scaling_factor/thickness
        if workspace is not None:
            Scale(workspace, workspace, scale, 'Multiply')
        
        return "Absolute scaling done [scaling factor = %g]" % scale

    def _compute_scaling_factor(self, reducer):
        """
            Compute the scaling factor
        """
        # Sanity check
        if self._data_file is None:
            raise RuntimeError, "AbsoluteScale called with no defined direct beam file"

        # Load data file
        filepath = reducer._full_file_path(self._data_file)
        data_file_ws = "__abs_scale_"+extract_workspace_name(filepath)
        
        loader = reducer._data_loader.clone(data_file=filepath)
        loader.set_beam_center(reducer.get_beam_center())
        loader.execute(reducer, data_file_ws)        
        
        # Get counting time
        if reducer._normalizer is None:
            # Note: this option shouldn't really be allowed
            monitor = reducer.NORMALIZATION_MONITOR
        else:
            monitor = reducer._normalizer.get_normalization_spectrum()
        monitor = mtd[data_file_ws].dataY(monitor)[0]
        
        if mtd[data_file_ws].getRun().hasProperty("sample_detector_distance"):
            sdd = mtd[data_file_ws].getRun().getProperty("sample_detector_distance").value
        else:
            raise RuntimeError, "AbsoluteScale could not read the sample-detector-distance"
        
        if self._beamstop_diameter is not None:
            beam_diameter = self._beamstop_diameter
        else:
            if mtd[data_file_ws].getRun().hasProperty("beam-diameter"):
                beam_diameter = mtd[data_file_ws].getRun().getProperty("beam-diameter").value
            else:
                raise RuntimeError, "AbsoluteScale could not read the beam radius and none was provided"        
        
        # Apply sensitivity correction
        if self._apply_sensitivity and reducer.get_sensitivity_correcter() is not None:
            reducer.get_sensitivity_correcter().execute(data_file_ws)
        
        det_count = 1
        cylXML = '<infinite-cylinder id="asbsolute_scale">' + \
                   '<centre x="0.0" y="0.0" z="0.0" />' + \
                   '<axis x="0.0" y="0.0" z="1.0" />' + \
                   '<radius val="%12.10f" />' % (beam_diameter/2000.0) + \
                 '</infinite-cylinder>\n'
                 
        det_finder = FindDetectorsInShape(Workspace=data_file_ws, ShapeXML=cylXML)
        det_list = det_finder.getPropertyValue("DetectorList")
        det_count_ws = "__absolute_scale"
        GroupDetectors(InputWorkspace=data_file_ws,  OutputWorkspace=det_count_ws,  DetectorList=det_list, KeepUngroupedSpectra="0")
        det_count = mtd[det_count_ws].dataY(0)[0]
        
        # Pixel size, in mm
        pixel_size_param = mtd[data_file_ws].getInstrument().getNumberParameter("x-pixel-size")
        if pixel_size_param is not None:
            pixel_size = pixel_size_param[0]
        else:
            raise RuntimeError, "AbsoluteScale could not read the pixel size"
        
        # (detector count rate)/(attenuator transmission)/(monitor rate)*(pixel size/SDD)**2
        self._scaling_factor = 1.0/(det_count/self._attenuator_trans/(monitor)*(pixel_size/sdd)*(pixel_size/sdd))

        mtd.deleteWorkspace(data_file_ws)
        mtd.deleteWorkspace(det_count_ws)
        
        

            
