# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Engineering.EnggUtils import GROUP, create_spectrum_list_from_string, CALIB_DIR
from Engineering.common import path_handling
from mantid.api import AnalysisDataService as ADS
from os import path
from mantid.simpleapi import Load, LoadDetectorsGroupingFile, CreateGroupingWorkspace, SaveDetectorsGrouping
from mantid.kernel import logger

GROUP_FILES = {
    GROUP.BOTH: "ENGINX_NorthAndSouth_grouping.xml",
    GROUP.NORTH: "ENGINX_North_grouping.xml",
    GROUP.SOUTH: "ENGINX_South_grouping.xml",
    GROUP.TEXTURE20: "ENGINX_Texture20_grouping.xml",
    GROUP.TEXTURE30: "ENGINX_Texture30_grouping.xml",
}
GROUP_BANK_ARGS = {GROUP.BOTH: "NorthBank,SouthBank", GROUP.NORTH: "NorthBank", GROUP.SOUTH: "SouthBank"}
GROUP_DESCRIPTIONS = {
    GROUP.BOTH: "North and South Banks",
    GROUP.NORTH: "North Bank",
    GROUP.SOUTH: "South Bank",
    GROUP.CROPPED: "Custom spectrum numbers",
    GROUP.CUSTOM: "Custom grouping file",
    GROUP.TEXTURE20: "Texture20",
    GROUP.TEXTURE30: "Texture30",
}
GROUP_WS_NAMES = {
    GROUP.BOTH: "NorthAndSouthBank_grouping",
    GROUP.NORTH: "NorthBank_grouping",
    GROUP.SOUTH: "SouthBank_grouping",
    GROUP.CROPPED: "Cropped_spectra_grouping",
    GROUP.CUSTOM: "Custom_grouping_file",
    GROUP.TEXTURE20: "Texture20_grouping",
    GROUP.TEXTURE30: "Texture30_grouping",
}
GROUP_SUFFIX = {
    GROUP.BOTH: "all_banks",
    GROUP.NORTH: "bank_1",
    GROUP.SOUTH: "bank_2",
    GROUP.CROPPED: "Cropped",
    GROUP.CUSTOM: "Custom",
    GROUP.TEXTURE20: "Texture20",
    GROUP.TEXTURE30: "Texture30",
}  # prm suffix
GROUP_FOC_WS_SUFFIX = {
    GROUP.BOTH: "bank",
    GROUP.NORTH: "bank_1",
    GROUP.SOUTH: "bank_2",
    GROUP.CROPPED: "Cropped",
    GROUP.CUSTOM: "Custom",
    GROUP.TEXTURE20: "Texture20",
    GROUP.TEXTURE30: "Texture30",
}


class CalibrationInfo:
    def __init__(self, group=None, instrument=None, ceria_path=None):
        self.group = group
        self.instrument = instrument
        self.ceria_path = ceria_path
        self.group_ws = None
        self.prm_filepath = None
        self.grouping_filepath = None
        self.spectra_list = None
        self.calibration_table = None

    def clear(self):
        self.group = None
        self.group_ws = None
        self.prm_filepath = None
        self.grouping_filepath = None
        self.spectra_list = None
        self.ceria_path = None
        self.instrument = None
        self.calibration_table = None

    # getters
    def get_foc_ws_suffix(self):
        if self.group:
            if self.group != GROUP.CUSTOM:
                return GROUP_FOC_WS_SUFFIX[self.group]
            else:
                fname = path.split(self.grouping_filepath)[1].split(".")[0]
                return f"{GROUP_FOC_WS_SUFFIX[GROUP.CUSTOM]}_{fname}"

    def get_group_suffix(self):
        if self.group:
            if self.group != GROUP.CUSTOM:
                return GROUP_SUFFIX[self.group]
            else:
                fname = path.split(self.grouping_filepath)[1].split(".")[0]
                return f"{GROUP_SUFFIX[GROUP.CUSTOM]}_{fname}"

    def get_group_ws_name(self):
        if self.group:
            if self.group != GROUP.CUSTOM:
                return GROUP_WS_NAMES[self.group]
            else:
                fname = path.split(self.grouping_filepath)[1].split(".")[0]
                return f"{GROUP_WS_NAMES[GROUP.CUSTOM]}_{fname}"

    def get_group_description(self):
        if self.group:
            return GROUP_DESCRIPTIONS[self.group]

    def get_group_file(self):
        if self.group:
            return GROUP_FILES[self.group]

    def get_calibration_table(self):
        return self.calibration_table

    def get_ceria_path(self):
        return self.ceria_path

    def get_ceria_runno(self):
        if self.ceria_path and self.instrument:
            return path_handling.get_run_number_from_path(self.ceria_path, self.instrument)

    def get_instrument(self):
        return self.instrument

    def get_prm_filepath(self):
        return self.prm_filepath

    # setters
    def set_prm_filepath(self, prm_filepath):
        self.prm_filepath = prm_filepath

    def set_calibration_table(self, cal_table):
        self.calibration_table = cal_table

    def set_calibration_paths(self, instrument, ceria_path):
        self.ceria_path = ceria_path
        self.instrument = instrument

    def set_calibration_from_prm_fname(self, file_path):
        """
        Determine the ROI, instrument and ceria run from the .prm calibration file that is being loaded
        :param file_path: Path of the .prm file being loaded
        """
        basepath, fname = path.split(file_path)
        # fname has form INSTRUMENT_ceriaRunNo_BANKS
        # BANKS can be "all_banks, "bank_1", "bank_2", "Cropped", "Custom", "Texture20", "Texture30"
        fname_words = fname.split("_")
        suffix = fname_words[-1].split(".")[0]  # take last element and remove extension
        if any(grp.value == suffix for grp in GROUP):
            self.group = GROUP(suffix)
            self.prm_filepath = file_path
        else:
            raise ValueError("Group not set: region of interest not recognised from .prm file name")
        self.set_calibration_paths(*fname_words[0:2])

    def set_spectra_list(self, spectra_list_str):
        self.spectra_list = create_spectrum_list_from_string(spectra_list_str)

    def set_grouping_file(self, grouping_filepath):
        self.grouping_filepath = grouping_filepath

    def set_group(self, group):
        self.group = group

    # functional
    def load_relevant_calibration_files(self, output_prefix="engggui"):
        """
        Load calibration table ws output from second step of calibration (PDCalibration of ROI focused spectra)
        :param output_prefix: prefix for workspace
        """
        filepath = path.splitext(self.prm_filepath)[0] + ".nxs"  # change extension to .nxs
        self.calibration_table = output_prefix + "_calibration_" + self.get_group_suffix()

        try:
            Load(Filename=filepath, OutputWorkspace=self.calibration_table)
        except Exception as e:
            logger.error("Unable to load calibration file " + filepath + ". Error: " + str(e))

        # load in custom grouping - checks if applicable inside method
        if not self.group.banks:
            self.load_custom_grouping_workspace()
        else:
            self.get_group_ws()  # creates group workspace

    def load_custom_grouping_workspace(self) -> None:
        """
        Load a custom grouping workspace saved post calibration (e.g. when user supplied custom spectra numbers or .cal)
        """
        if not self.group.banks:
            # no need to load grp ws for bank grouping
            ws_name = self.get_group_ws_name()
            grouping_filepath = path.splitext(self.prm_filepath)[0] + ".xml"
            self.group_ws = LoadDetectorsGroupingFile(InputFile=grouping_filepath, OutputWorkspace=ws_name)

    def save_grouping_workspace(self, directory: str) -> None:
        """
        Save grouping workspace created for custom spectra or .cal cropping.
        :param directory: directory in which to save grouping workspace
        """
        if self.group and not self.group.banks:
            filename = self.generate_output_file_name(ext=".xml")
            SaveDetectorsGrouping(InputWorkspace=self.group_ws, OutputFile=path.join(directory, filename))
        else:
            logger.warning("Only save grouping workspace for custom or cropped groupings.")
        return

    def generate_output_file_name(self, group=None, ext=".prm"):
        """
        Generate an output filename in the form INSTRUMENT_ceriaRunNo_BANKS
        :param ext: Extension to be used on the saved file
        :param group: group to use instead of that stored in self.group (e.g. North and South bank only)
        :return: filename
        """
        if not group:
            suffix = self.get_group_suffix()
        else:
            suffix = GROUP_SUFFIX[group]
        return "_".join([self.instrument, self.get_ceria_runno(), suffix]) + ext

    def get_subplot_title(self, ispec):
        """
        :param ispec: spectrum index for which the calibration results (TOF vs d) are being plotted
        :return: string to use as subplot title in plot generated in calibration tab
        """
        if self.group in [GROUP.NORTH, GROUP.SOUTH, GROUP.CUSTOM, GROUP.CROPPED]:
            return self.get_group_description()
        elif self.group == GROUP.BOTH:
            return GROUP_DESCRIPTIONS[GROUP.NORTH] if ispec == 0 else GROUP_DESCRIPTIONS[GROUP.SOUTH]
        else:
            return f"{self.get_group_description()} spec: {ispec}"  # texture

    def get_group_ws(self):
        """
        Returns grouping workspace for ROI (creates if not present)
        :return: group workspace
        """
        if not self.group_ws or not ADS.doesExist(self.group_ws.name()):
            if self.group.banks:
                self.create_bank_grouping_workspace()
            elif self.group == GROUP.CROPPED:
                self.create_grouping_workspace_from_spectra_list()
            elif self.group == GROUP.CUSTOM:
                ext = self.grouping_filepath.split(".")[-1]
                if ext == "cal":
                    self.create_grouping_workspace_from_calfile()
                elif ext == "xml":
                    self.group_ws = LoadDetectorsGroupingFile(InputFile=self.grouping_filepath, OutputWorkspace=self.get_group_ws_name())
        return self.group_ws

    def create_bank_grouping_workspace(self):
        """
        Create grouping workspace for ROI corresponding to one or more banks
        """
        ws_name = self.get_group_ws_name()
        grp_ws = None
        try:
            grp_ws = LoadDetectorsGroupingFile(InputFile=path.join(CALIB_DIR, GROUP_FILES[self.group]), OutputWorkspace=ws_name)
        except ValueError:
            logger.notice("Grouping file not found in user directories - creating one")
            if self.group.banks and self.group != GROUP.TEXTURE20 and self.group != GROUP.TEXTURE30:
                grp_ws, _, _ = CreateGroupingWorkspace(
                    InstrumentName=self.instrument, OutputWorkspace=ws_name, GroupNames=GROUP_BANK_ARGS[self.group]
                )
        if grp_ws:
            self.group_ws = grp_ws
        else:
            raise ValueError(
                "Could not find or create grouping requested - make sure the directory of the grouping.xml files is on the path"
            )

    def create_grouping_workspace_from_calfile(self):
        """
        Create grouping workspace for ROI defined in .cal file
        """
        grp_ws, _, _ = CreateGroupingWorkspace(
            InstrumentName=self.instrument, OldCalFilename=self.grouping_filepath, OutputWorkspace=self.get_group_ws_name()
        )
        self.group_ws = grp_ws

    def create_grouping_workspace_from_spectra_list(self):
        """
        Create grouping workspace for ROI defined as a list of spectrum numbers
        """
        grp_ws, _, _ = CreateGroupingWorkspace(InstrumentName=self.instrument, OutputWorkspace=self.get_group_ws_name())
        for spec in self.spectra_list:
            det_ids = grp_ws.getDetectorIDs(spec - 1)
            grp_ws.setValue(det_ids[0], 1)
        self.group_ws = grp_ws

    def is_valid(self):
        """
        :return: bool for if CalibrationInfo object can be used for focusing
        """
        return (
            self.ceria_path is not None
            and self.instrument is not None
            and self.calibration_table is not None
            and self.group_ws is not None
            and self.calibration_table in ADS
            and self.group_ws in ADS
        )
