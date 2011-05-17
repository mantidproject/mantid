from MantidFramework import *
from mantidsimple import *
import math  

class ReactorSANSResolution(PythonAlgorithm):
    """
        Calculate and populate the Q resolution      
    """
    
    def category(self):
        return "SANS"

    def name(self):
        return "ReactorSANSResolution"

    def PyInit(self):
        # Input workspace
        self.declareWorkspaceProperty("InputWorkspace", "", Direction.Input, Description="Name the workspace to calculate the transmission from")
        # Output workspace to put the transmission histo into
        self.declareProperty("OutputWorkspace", "", Description="Name of the workspace that will contain the transmission histogram")

    def PyExec(self):
        input_ws = self.getProperty("InputWorkspace")
        if input_ws.getAxis(0).getUnit().name() != "MomentumTransfer":
            raise RuntimeError, "ReactorSANSResolution expects an input workspace with units of Q"

        output_ws_name = self.getProperty("OutputWorkspace")

        if input_ws.getName() != output_ws_name:
            CloneWorkspace(input_ws, output_ws_name)
            output_ws = mtd[output_ws_name]
        else:
            output_ws = input_ws

        # Q resolution calculation
        # All distances in mm
        wvl = None
        if input_ws.getRun().hasProperty("wavelength"):            
            wvl = input_ws.getRun().hasProperty("wavelength")
            
        d_wvl = None
        if input_ws.getRun().hasProperty("wavelength-spread"):            
            d_wvl = input_ws.getRun().hasProperty("wavelength-spread")
        
        source_apert_radius = None
        if input_ws.getRun().hasProperty("source-aperture-diameter"):
            source_apert_radius = input_ws.getRun().getProperty("source-aperture-diameter").value/2.0
            
        sample_apert_radius = None
        if input_ws.getRun().hasProperty("sample-aperture-diameter"):
            sample_apert_radius = input_ws.getRun().getProperty("sample-aperture-diameter").value/2.0
            
        source_sample_distance = None
        if input_ws.getRun().hasProperty("source-sample-distance"):
            source_sample_distance = input_ws.getRun().getProperty("source-sample-distance").value
            
        sample_detector_distance = None
        if input_ws.getRun().hasProperty("sample_detector_distance"):
            sample_detector_distance = input_ws.getRun().getProperty("sample_detector_distance").value
        
        pixel_size_x = input_ws.getInstrument().getNumberParameter("x-pixel-size")[0]
        
        if wvl is not None and d_wvl is not None \
            and source_apert_radius is not None and sample_apert_radius is not None \
            and source_sample_distance is not None and sample_detector_distance is not None:
            k = 2.0*math.pi/wvl
            res_factor = math.pow(k*source_apert_radius/source_sample_distance, 2)
            res_factor += (math.pow(k*sample_apert_radius*(source_sample_distance+sample_detector_distance)/(source_sample_distance*sample_detector_distance), 2)/4.0)
            res_factor += math.pow(k*pixel_size_x/sample_detector_distance, 2)/12.0
            
            for i in range(len(output_ws.readX(0))):
                 output_ws.dataDx(0)[i] = math.sqrt(res_factor+math.pow((output_ws.readX(0)[i]*d_wvl), 2)/6.0)       
        else:
            raise RuntimeError, "ReactorSANSResolution could not find all the run parameters needed to compute the resolution."

mtd.registerPyAlgorithm(ReactorSANSResolution())
