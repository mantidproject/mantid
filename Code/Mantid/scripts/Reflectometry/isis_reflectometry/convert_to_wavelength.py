import mantid.simpleapi as msi
import mantid.api
from mantid.kernel import logger

class ConvertToWavelength(object):
    
    # List of workspaces to process.
    __ws_list = []
    
    @classmethod 
    def get_monitors_mask(cls, ws):  
        """
        Get the monitor indexes as a mask.
        
        Arguments:
        ws -- Workspace to determine the monitor masks for.
        """
        monitor_masks = list()
        for i in range(ws.getNumberHistograms()):
            ismonitor = False
            try:
                det = ws.getDetector(i)
                ismonitor = det.isMonitor()
            except RuntimeError:
                pass
            monitor_masks.append(ismonitor)
        return monitor_masks
    
    @classmethod
    def sum_workspaces(cls, workspaces):
        """
        Sum together all workspaces. return the result.
        Returns:
        Result of sum ( a workspace)
        """
        return sum(workspaces)
    
    def __to_workspace_list(self, source_list):
        temp=[]
        for item in source_list:
            if isinstance(item, mantid.api.MatrixWorkspace):
                temp.append(item)
            elif isinstance(item, str):
                if not mtd.doesExist(item):
                    raise ValueError("Unknown source item %s" % item)
                temp.append(mtd[item])
            else:
                raise ValueError("Expects a list of workspace or workspace names.")
        return temp
        
    
    def __init__(self, source):
        """
        Constructor
        Arguments: 
        list -- source workspace or workspaces.
        
        Convert inputs into a list of workspace objects.
        """
        if not isinstance(source, list):
            source_list = [source]
            self.__ws_list = source_list
        else:
            self.__ws_list = source
        
    def convert(self, wavelength_min, wavelength_max, monitors_to_correct=None, bg_min=None, bg_max=None):
        """
        Run the conversion
        
        Arguments:
        bg_min: x min background in wavelength
        bg_max: x max background in wavelength
        wavelength_min: min wavelength in x for monitor workspace
        wavelength_max: max wavelength in x for detector workspace
        
        Returns:
        monitor_ws: A workspace of monitors
        """
        # Sanity check inputs.
        if(wavelength_min >= wavelength_max):
            raise ValueError("Wavelength_min must be < wavelength_max min: %s, max: %s" % (wavelength_min, wavelength_max))
        
        if any((monitors_to_correct, bg_min, bg_max)) and not all((monitors_to_correct, bg_min, bg_max)):
            raise ValueError("Either provide ALL, monitors_to_correct, bg_min, bg_max or none of them")
        
        if all((bg_min, bg_max)) and bg_min >= bg_max:
            raise ValueError("Background min must be < Background max")
        
        sum = ConvertToWavelength.sum_workspaces(self.__ws_list)
        sum_wavelength= msi.ConvertUnits(InputWorkspace=sum, Target="Wavelength")
        monitor_masks = ConvertToWavelength.get_monitors_mask(sum)
    
        # Assuming that the monitors are in a block start-end.
        first_detector_index = monitor_masks.index(False)
        logger.debug("First detector index %s" % str(first_detector_index))
            
        # Crop out the monitor workspace
        monitor_ws = msi.CropWorkspace(InputWorkspace=sum_wavelength, StartWorkspaceIndex=0,EndWorkspaceIndex=first_detector_index-1)
        # Crop out the detector workspace
        detector_ws = msi.CropWorkspace(InputWorkspace=sum_wavelength, XMin=wavelength_min,XMax=wavelength_max,StartWorkspaceIndex=first_detector_index)
        # Apply a flat background
        if all((monitors_to_correct, bg_min, bg_max)):
            monitor_ws = msi.CalculateFlatBackground(InputWorkspace=monitor_ws,WorkspaceIndexList=monitors_to_correct,StartX=bg_min, EndX=bg_max)
        
        return (monitor_ws, detector_ws)
        
        
   
        
        
