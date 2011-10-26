"""*WIKI* 

Load reflectometry data

*WIKI*"""
from MantidFramework import *
from mantidsimple import *
from LargeScaleStructures.REF_L_geometry import create_geometry
import os
import tempfile

class RefLoad(PythonAlgorithm):
    """
        Load reflectometry data
    """
    
    def category(self):
        return "Workflow\\Reflectometry"

    def name(self):
        return "RefLoad"

    def PyInit(self):
        # Input workspace
        self.declareFileProperty("Filename", "", FileAction.Load, Direction=Direction.Input, Description="File to load as reflectometry data")
        self.declareWorkspaceProperty("OutputWorkspace", "", Direction=Direction.Output, Description="Name of the workspace that will contain the data")
        self.declareProperty("PixelSizeX", 0.0007, Description="Size of pixels in X [m]")
        self.declareProperty("PixelSizeY", 0.0007, Description="Size of pixels in Y [m]")
        self.declareProperty("OutputMessage", "", Direction=Direction.Output)

    def PyExec(self):
        input_file = self.getPropertyValue("Filename")
        pixel_size_x = self.getProperty("PixelSizeX")
        pixel_size_y = self.getProperty("PixelSizeY")
        output_ws_name = self.getPropertyValue("OutputWorkspace")
        
        geometry_file = os.path.join(tempfile.gettempdir(), "REF_L_Definition.xml")
        create_geometry(geometry_file, pixel_width=pixel_size_x, pixel_height=pixel_size_y)
        Load(Filename=input_file, OutputWorkspace=output_ws_name)
        LoadInstrument(Workspace=output_ws_name, Filename=geometry_file, MonitorList='-1')
        mtd[output_ws_name].getRun().addProperty_str("geometry_file", geometry_file, True)
        
        self.setPropertyValue("OutputWorkspace", output_ws_name)

mtd.registerPyAlgorithm(RefLoad())
