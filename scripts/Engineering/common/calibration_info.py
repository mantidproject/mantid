# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from Engineering.EnggUtils import create_spectrum_list_from_string, CALIB_DIR
from Engineering.common import path_handling
from mantid.api import AnalysisDataService as ADS
from mantid.dataobjects import GroupingWorkspace
from os import path
from mantid.simpleapi import Load, LoadDetectorsGroupingFile, CreateGroupingWorkspace, SaveDetectorsGrouping
from mantid.kernel import logger
from typing import Sequence
from Engineering.common.instrument_config import get_instr_config, INSTRUMENT_GROUP


class CalibrationInfo:
    def __init__(
        self,
        group: INSTRUMENT_GROUP | None = None,
        instrument: str | None = None,
        ceria_path: str | None = None,
        vanadium_path: str | None = None,
    ):
        self.group = group
        self.instrument = instrument
        self.config = get_instr_config(instrument)
        self.ceria_path = ceria_path
        self.vanadium_path = vanadium_path
        self.group_ws = None
        self.prm_filepath = None
        self.grouping_filepath = None
        self.spectra_list = None
        self.spectra_list_str = None
        self.calibration_table = None
        self.extra_group_suffix = ""
        self.fit_peak_shape = "BackToBackExponential"

    def clear(self):
        self.group = None
        self.group_ws = None
        self.prm_filepath = None
        self.grouping_filepath = None
        self.spectra_list = None
        self.spectra_list_str = None
        self.ceria_path = None
        self.vanadium_path = None
        self.instrument = None
        self.config = None
        self.calibration_table = None
        self.extra_group_suffix = ""
        self.fit_peak_shape = "BackToBackExponential"

    # getters
    def get_foc_ws_suffix(self) -> str:
        return self._get_suffix("foc_ws_suffix")

    def get_group_suffix(self) -> str:
        return self._get_suffix("suffix")

    def get_group_ws_name(self) -> str:
        return self._get_suffix("ws_name")

    def _get_suffix(self, attr: str) -> str:
        """Get the full suffix for the current group"""
        if self.group:
            self.set_extra_group_suffix()
            return f"{getattr(self.config.group_info[self.group], attr)}{self.extra_group_suffix}"
        return ""

    def get_group_banks(self) -> Sequence[int]:
        if self.group:
            return self.config.group_info[self.group].banks
        return []

    def get_group_description(self) -> str | None:
        if self.group:
            return self.config.group_info[self.group].description

    def get_group_file(self) -> str | None:
        if self.group:
            return self.config.group_info[self.group].grouping_file

    def get_calibration_table(self) -> str | None:
        return self.calibration_table

    def get_ceria_path(self) -> str | None:
        return self.ceria_path

    def get_ceria_runno(self) -> str | None:
        if self.ceria_path and self.instrument:
            return path_handling.get_run_number_from_path(self.ceria_path, self.instrument)

    def get_vanadium_path(self) -> str | None:
        return self.vanadium_path

    def get_vanadium_runno(self) -> str | None:
        if self.vanadium_path and self.instrument:
            return path_handling.get_run_number_from_path(self.vanadium_path, self.instrument)

    def get_instrument(self) -> str | None:
        return self.instrument

    def get_prm_filepath(self) -> str | None:
        return self.prm_filepath

    def get_group(self) -> INSTRUMENT_GROUP:
        return self.group

    def get_fit_peak_shape(self) -> str:
        return self.fit_peak_shape

    # setters
    def set_extra_group_suffix(self) -> None:
        self.extra_group_suffix = ""
        if self.group == self.config.group.CUSTOM:
            self.set_grouping_filepath_from_prm_filepath()
            if self.grouping_filepath:
                filename = path.basename(self.grouping_filepath)
                self.extra_group_suffix = f"_{path.splitext(filename)[0].split('_')[-1]}"
            # to prevent really long suffixes, will use the last text after the _ in the prm filename
        elif self.group == self.config.group.CROPPED and self.spectra_list_str:
            self.extra_group_suffix = f"_{self.spectra_list_str}"

    def set_prm_filepath(self, prm_filepath: str | None) -> None:
        self.prm_filepath = prm_filepath

    def set_calibration_table(self, cal_table: str) -> None:
        self.calibration_table = cal_table

    def set_calibration_paths(self, instrument: str, ceria_path: str, vanadium_path: str = None) -> None:
        self.ceria_path = ceria_path
        self.vanadium_path = vanadium_path
        self.instrument = instrument
        self.set_config(instrument)

    def set_config(self, instrument: str) -> None:
        self.config = get_instr_config(instrument)

    def set_calibration_from_prm_fname(self, file_path: str, instr: str) -> None:
        """
        Determine the ROI, instrument and ceria run from the .prm calibration file that is being loaded
        :param file_path: Path of the .prm file being loaded, has form INSTRUMENT_ceriaRunNo_BANKS.ext
        where BANKS = "all_banks", "bank_1", "bank_2", "Cropped_{specstr}", "Custom_{grpfp}", "Texture20", "Texture30"
        :param instr: Instrument name for retrieving the calibration configuration
        """
        self.set_config(instr)
        basepath, fname = path.split(file_path)
        inst_bank = fname.split("_")[:2]
        suffix = (fname[len("_".join(inst_bank)) + 1 :]).split(".")[0]  # string after INSTRUMENT_ceriaRunNo_ minus ext
        found_group = False
        for grp in self.config.group:
            if grp.value in suffix:
                self.group = grp
                self.prm_filepath = file_path
                self.set_grouping_filepath_from_prm_filepath()
                found_group = True
        if not found_group:
            raise ValueError("Group not set: region of interest not recognised from .prm file name")
        self.set_calibration_paths(*inst_bank)

    def set_spectra_list(self, spectra_list_str: str) -> None:
        self.spectra_list_str = spectra_list_str
        self.spectra_list = create_spectrum_list_from_string(spectra_list_str)
        self.set_prm_filepath(None)  # clear any prm filepath as won't correspond to this spec_list

    def set_grouping_file(self, grouping_filepath: str) -> None:
        self.grouping_filepath = grouping_filepath
        self.set_prm_filepath(None)  # clear any prm filepath as won't correspond to this grouping

    def set_group(self, group: INSTRUMENT_GROUP) -> None:
        self.group = group

    def set_fit_peak_shape(self, peak_shape: str) -> None:
        self.fit_peak_shape = peak_shape

    # functional
    def is_texture_group(self) -> bool:
        return self.get_group() in self.config.texture_groups if self.config else False

    def set_grouping_filepath_from_prm_filepath(self) -> None:
        """
        If there is a prm filepath declared, assign the xml filepath as the grouping filepath
        """
        if self.prm_filepath:
            self.grouping_filepath = path.splitext(self.prm_filepath)[0] + ".xml"

    def load_relevant_calibration_files(self, output_prefix: str = "engggui") -> None:
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
        if not self.get_group_banks():
            self.load_custom_grouping_workspace()
        else:
            self.get_group_ws()  # creates group workspace

    def load_custom_grouping_workspace(self) -> None:
        """
        Load a custom grouping workspace saved post calibration (e.g. when user supplied custom spectra numbers or .cal)
        """
        if not self.get_group_banks():
            # no need to load grp ws for bank grouping
            ws_name = self.get_group_ws_name()
            self.set_grouping_filepath_from_prm_filepath()
            self.group_ws = LoadDetectorsGroupingFile(InputFile=self.grouping_filepath, OutputWorkspace=ws_name)

    def save_grouping_workspace(self, directory: str) -> None:
        """
        Save grouping workspace created for custom spectra or .cal cropping.
        :param directory: directory in which to save grouping workspace
        """
        if self.group and not self.get_group_banks():
            filename = self.generate_output_file_name(ext=".xml")
            SaveDetectorsGrouping(InputWorkspace=self.group_ws, OutputFile=path.join(directory, filename))
        else:
            logger.warning("Only save grouping workspace for custom or cropped groupings.")
        return

    def generate_output_file_name(self, group: INSTRUMENT_GROUP | None = None, ext: str = ".prm") -> str:
        """
        Generate an output filename in the form INSTRUMENT_ceriaRunNo_BANKS
        :param ext: Extension to be used on the saved file
        :param group: group to use instead of that stored in self.group (e.g. North and South bank only)
        :return: filename
        """
        if not group:
            suffix = self.get_group_suffix()
        else:
            suffix = self.config.group_info[group].suffix
        return "_".join([self.instrument, self.get_ceria_runno(), suffix]) + ext

    def get_subplot_title(self, ispec: int) -> str:
        """
        :param ispec: spectrum index for which the calibration results (TOF vs d) are being plotted
        :return: string to use as subplot title in plot generated in calibration tab
        """
        if self.group in [self.config.group.NORTH, self.config.group.SOUTH, self.config.group.CUSTOM, self.config.group.CROPPED]:
            return self.get_group_description()
        elif self.group == self.config.group.BOTH:
            return (
                self.config.group_info[self.config.group.NORTH].description
                if ispec == 0
                else self.config.group_info[self.config.group.SOUTH].description
            )
        else:
            return f"{self.get_group_description()} spec: {ispec}"  # texture

    def get_group_ws(self) -> GroupingWorkspace:
        """
        Returns grouping workspace for ROI (creates if not present)
        :return: group workspace
        """
        if not self.group_ws or not ADS.doesExist(self.group_ws.name()):
            self.update_group_ws_from_group()
        return self.group_ws

    def update_group_ws_from_group(self) -> None:
        if self.group:
            if self.get_group_banks():
                self.create_bank_grouping_workspace()
            elif self.group == self.config.group.CROPPED:
                self.create_grouping_workspace_from_spectra_list()
            elif self.group == self.config.group.CUSTOM:
                self.create_grouping_workspace_from_file()

    def create_grouping_workspace_from_file(self) -> None:
        """
        Create grouping workspace from a custom file (.cal or .xml)
        """
        if not self.grouping_filepath:
            raise ValueError("Grouping file path is not set.")
        ext = path.splitext(self.grouping_filepath)[-1].lower()
        if ext == ".cal":
            self.group_ws, _, _ = CreateGroupingWorkspace(
                InstrumentName=self.instrument, OldCalFilename=self.grouping_filepath, OutputWorkspace=self.get_group_ws_name()
            )
        elif ext == ".xml":
            self.group_ws = LoadDetectorsGroupingFile(InputFile=self.grouping_filepath, OutputWorkspace=self.get_group_ws_name())
        else:
            raise ValueError(f"Unsupported file extension: {ext}")

    def create_bank_grouping_workspace(self) -> None:
        """
        Create grouping workspace for ROI corresponding to one or more banks
        """
        ws_name = self.get_group_ws_name()
        grp_ws = None
        try:
            grp_ws = LoadDetectorsGroupingFile(
                InputFile=path.join(CALIB_DIR, self.config.group_info[self.group].grouping_file), OutputWorkspace=ws_name
            )
        except ValueError:
            logger.notice("Grouping file not found in user directories - creating one")
            if self.get_group_banks() and self.group not in self.config.texture_groups:
                grp_ws, _, _ = CreateGroupingWorkspace(
                    InstrumentName=self.instrument, OutputWorkspace=ws_name, GroupNames=self.config.group_info[self.group].bank_args
                )
        if grp_ws:
            self.group_ws = grp_ws
        else:
            raise ValueError(
                "Could not find or create grouping requested - make sure the directory of the grouping.xml files is on the path"
            )

    def create_grouping_workspace_from_spectra_list(self) -> None:
        """
        Create grouping workspace for ROI defined as a list of spectrum numbers
        """
        grp_ws, _, _ = CreateGroupingWorkspace(InstrumentName=self.instrument, OutputWorkspace=self.get_group_ws_name())
        for spec in self.spectra_list:
            det_ids = grp_ws.getDetectorIDs(spec - 1)
            grp_ws.setValue(det_ids[0], 1)
        self.group_ws = grp_ws

    def is_valid(self) -> bool:
        """
        :return: bool for if CalibrationInfo object can be used for focusing
        """
        return (
            self.ceria_path is not None
            and self.vanadium_path is not None
            and self.instrument is not None
            and self.calibration_table is not None
            and self.group_ws is not None
            and self.calibration_table in ADS
            and self.group_ws in ADS
        )
