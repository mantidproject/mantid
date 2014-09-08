import mantid.simpleapi as msi
import mantid.api
from mantid.kernel import logger
import re

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
    def get_first_of_coadd_ws(cls, candidate):
        return re.split(',|:', candidate)[0]

    @classmethod
    def to_single_workspace(cls, candidate):
        if isinstance(candidate, str):
            ConvertToWavelength.get_first_of_coadd_ws(candidate)
        ws = ConvertToWavelength.to_workspace(candidate)
        input = None
        if isinstance(ws, mantid.api.WorkspaceGroup):
            input = ws[0]
        else:
            input = ws
        output = msi.CloneWorkspace(OutputWorkspace="_singleWorkspace",InputWorkspace=input)
        return output

    @classmethod
    def to_workspace(cls, candidate, ws_prefix="_"):
        _workspace = None
        if isinstance(candidate, str):
            candidate = ConvertToWavelength.get_first_of_coadd_ws(candidate)
        if isinstance(candidate, mantid.api.Workspace):
            _workspace = candidate
        elif isinstance(candidate, str):
            if  mantid.api.AnalysisDataService.doesExist(candidate.strip()):
                _workspace = mantid.api.AnalysisDataService.retrieve(candidate.strip())
            elif  mantid.api.AnalysisDataService.doesExist(ws_prefix + str(candidate.strip())):
                _workspace = mantid.api.AnalysisDataService.retrieve(ws_prefix + str(candidate.strip()))
            else:
                ws_name = ws_prefix + str(candidate.strip())
                msi.Load(Filename=candidate, OutputWorkspace=ws_name)
                _workspace = mantid.api.AnalysisDataService.retrieve(ws_name)
        else:
             raise ValueError("Unknown source item %s" % candidate)
        return _workspace

    def get_workspace_from_list(self, index):
        ws = self.__ws_list[index]
        return ws

    def get_ws_list_size(self):
        return len(self.__ws_list)

    def get_name_list(self):
        return self.__source_list

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
            if isinstance(source, str):
                source_list = re.split(',|:', source)
            else:
                source_list = [source]
        else:
            source_list = source
        self.__source_list = source_list
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

        _in_rng = msi.CloneWorkspace(ws)
        if not isinstance(rng, tuple):
            raise ValueError("Elements must be tuples.")

        def is_actual_range(rng):
            return len(rng) == 2 and isinstance(rng[0], int) and isinstance(rng[1], int)

        if is_actual_range(rng):
            start, stop = rng[0], rng[1]
            _in_rng = msi.CropWorkspace(InputWorkspace=_in_rng, StartWorkspaceIndex=start,EndWorkspaceIndex=stop)
        else:
            for subrng in rng:
                _in_rng = ConvertToWavelength.crop_range(ws, subrng)

        return _in_rng

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
        _monitor_ws: A workspace of monitors
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
        _monitor_ws = msi.CropWorkspace(InputWorkspace=sum_wavelength, StartWorkspaceIndex=monitor_workspace_index,EndWorkspaceIndex=monitor_workspace_index)
        # Crop out the detector workspace then chop out the x-ranges of interest.
        _detector_ws =  ConvertToWavelength.crop_range(sum_wavelength, detector_workspace_indexes)

        _detector_ws =  msi.CropWorkspace(InputWorkspace=_detector_ws, XMin=wavelength_min, XMax=wavelength_max)

        # Apply a flat background
        if correct_monitor and all((bg_min, bg_max)):
            _monitor_ws = msi.CalculateFlatBackground(InputWorkspace=_monitor_ws,WorkspaceIndexList=0,StartX=bg_min, EndX=bg_max)

        msi.DeleteWorkspace(Workspace=sum_wavelength.getName())
        return (_monitor_ws, _detector_ws)





