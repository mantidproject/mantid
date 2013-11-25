import mantid.simpleapi as msi
import mantid.api
from mantid.kernel import logger

class ConvertToWavelength(object):
    
    # List of workspaces to process.
    __ws_list = []
    
    @classmethod
    def sum_workspaces(cls, workspaces):
        """
        Sum together all workspaces. return the result.
        Returns:
        Result of sum ( a workspace)
        """
        return sum(workspaces)
    
    @classmethod
    def to_workspace(cls, candidate):
        workspace = None
        if isinstance(candidate, mantid.api.MatrixWorkspace):
            workspace = candidate
        elif isinstance(candidate, str):
            if  mantid.api.AnalysisDataService.doesExist(candidate):
                workspace = mantid.api.AnalysisDataService.retrieve(candidate)
            else:
                 workspace = msi.Load(Filename=candidate)
        else:
             raise ValueError("Unknown source item %s" % candidate)
        return workspace

    def __to_workspace_list(self, source_list):
        temp=[]
        for item in source_list:
            temp.append(ConvertToWavelength.to_workspace(item))
        self.__ws_list = temp
        
    
    def __init__(self, source):
        """
        Constructor
        Arguments: 
        list -- source workspace or workspaces.
        
        Convert inputs into a list of workspace objects.
        """
        source_list = None
        if not isinstance(source, list):
            source_list = [source]
        else:
            source_list = source
        self.__to_workspace_list(source_list)    
            
    @classmethod
    def crop_range(cls, ws, rng):
        """
        Given a range of workspace indexes to keep (rng), Crop the workspace such that these are kept.
        Arguments:
        
        ws : Workspace to crop spectrum from
        rng : Tuple of range tuples. syntax may be (start, stop) or ((start0, stop0), (start1, stop1), ...)
        
        returns a new copy of the workspace with spectra cropped out.
        """
        
        in_rng = msi.CloneWorkspace(ws)
        if not isinstance(rng, tuple):
            raise ValueError("Elements must be tuples.")
        
        def is_actual_range(rng):
            return len(rng) == 2 and isinstance(rng[0], int) and isinstance(rng[1], int)
        
        if is_actual_range(rng):
            start, stop = rng[0], rng[1]
            in_rng = msi.CropWorkspace(InputWorkspace=in_rng, StartWorkspaceIndex=start,EndWorkspaceIndex=stop)
        else:
            for subrng in rng:
                in_rng = ConvertToWavelength.crop_range(ws, subrng)
  
        return in_rng
        
    def convert(self, wavelength_min, wavelength_max, detector_workspace_indexes, monitor_workspace_index, correct_monitor=False, bg_min=None, bg_max=None):
        """
        Run the conversion
        
        Arguments:
        
        workspace_ids: Start and end ranges. Ids to be considered as workspaces. Nested list syntax supported
        wavelength_min: min wavelength in x for monitor workspace
        wavelength_max: max wavelength in x for detector workspace
        detector_workspace_indexes: Tuple of workspace indexes (or tuple of tuple min, max ranges to keep)
        monitor_workspace_index: The index of the monitor workspace
        correct_monitor: Flag indicating that monitors should have a flat background correction applied
        bg_min: x min background in wavelength
        bg_max: x max background in wavelength
        
        Returns:
        monitor_ws: A workspace of monitors
        """
        # Sanity check inputs.
        if(wavelength_min >= wavelength_max):
            raise ValueError("Wavelength_min must be < wavelength_max min: %s, max: %s" % (wavelength_min, wavelength_max))
        
        if correct_monitor and not all((bg_min, bg_max)):
            raise ValueError("Either provide ALL, monitors_to_correct, bg_min, bg_max or none of them")
        
        if all((bg_min, bg_max)) and bg_min >= bg_max:
            raise ValueError("Background min must be < Background max")
        
        sum = ConvertToWavelength.sum_workspaces(self.__ws_list)
        sum_wavelength= msi.ConvertUnits(InputWorkspace=sum, Target="Wavelength", AlignBins='1')
       
        logger.debug("Monitor detector index %s" % str(monitor_workspace_index))
            
        # Crop out the monitor workspace
        monitor_ws = msi.CropWorkspace(InputWorkspace=sum_wavelength, StartWorkspaceIndex=monitor_workspace_index,EndWorkspaceIndex=monitor_workspace_index)
        # Crop out the detector workspace then chop out the x-ranges of interest.
        detector_ws =  ConvertToWavelength.crop_range(sum_wavelength, detector_workspace_indexes)
        
        detector_ws =  msi.CropWorkspace(InputWorkspace=detector_ws, XMin=wavelength_min, XMax=wavelength_max)

        # Apply a flat background
        if correct_monitor and all((bg_min, bg_max)):
            monitor_ws = msi.CalculateFlatBackground(InputWorkspace=monitor_ws,WorkspaceIndexList=0,StartX=bg_min, EndX=bg_max)
        
        return (monitor_ws, detector_ws)
        
        
   
        
        
