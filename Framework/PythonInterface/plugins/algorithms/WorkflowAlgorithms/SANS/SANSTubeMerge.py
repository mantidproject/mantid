# -*- coding: utf-8 -*-# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.api import DataProcessorAlgorithm, AlgorithmFactory, FileProperty, FileAction, Progress, AnalysisDataService


class Prop:
    FRONT = "Front"
    REAR = "Rear"
    OUTPUT_FILE = "OutputFile"


class SANSTubeMerge(DataProcessorAlgorithm):
    _REAR_DET_NAME = "rear-detector"
    _CALIBRATED_WS_NAME = "empty_instr"
    _NEXUS_SUFFIX = ".nxs"

    def category(self):
        return "SANS\\Calibration"

    def name(self):
        """Return the name of the algorithm."""
        return "SANSTubeMerge"

    def summary(self):
        return "Merge the calibrations of the Sans2d front and rear detectors to obtain a single calibration file."

    def seeAlso(self):
        """Return a list of related algorithm names."""
        return ["SANSTubeCalibration"]

    def PyInit(self):
        self.declareProperty(
            FileProperty(name=Prop.FRONT, defaultValue="", action=FileAction.Load, extensions=["nxs"]),
            doc="The path to the Nexus file containing the front detector calibration",
        )
        self.declareProperty(
            FileProperty(name=Prop.REAR, defaultValue="", action=FileAction.Load, extensions=["nxs"]),
            doc="The path to the Nexus file containing the rear detector calibration",
        )
        self.declareProperty(
            FileProperty(name=Prop.OUTPUT_FILE, defaultValue="", action=FileAction.OptionalSave, extensions=["nxs"]),
            doc="The location to save the merged calibration file to.",
        )

    def PyExec(self):
        prog_report = Progress(self, start=0.0, end=0.5, nreports=2)
        prog_report.report("Loading Rear")
        rear_calib = self._load_file(self.getProperty(Prop.REAR).value, "rear_calib")
        prog_report.report("Loading Front")
        front_calib = self._load_file(self.getProperty(Prop.FRONT).value, "front_calib")

        # In the front detector calibration, move the pixels for the rear detector
        # so that both calibrations are now in a single workspace
        rear_inst = rear_calib.getInstrument()
        det_ids = front_calib.detectorInfo().detectorIDs()

        move_alg = self._create_child_alg("MoveInstrumentComponent")
        move_alg.setProperty("Workspace", front_calib)
        move_alg.setProperty("RelativePosition", False)

        prog_report = Progress(self, start=0.5, end=0.75, nreports=det_ids.size)
        for det_id in det_ids:
            prog_report.report(f"Checking detector {det_id}")
            det = rear_inst.getDetector(det_id.item())
            if self._REAR_DET_NAME in det.getFullName():
                prog_report.report(f"Moving detector {det_id}")
                move_alg.setProperty("DetectorID", det_id.item())
                move_alg.setProperty("X", det.getPos().getX())
                move_alg.setProperty("Y", det.getPos().getY())
                move_alg.setProperty("Z", det.getPos().getZ())
                move_alg.execute()

        prog_report = Progress(self, start=0.75, end=0.9, nreports=2)
        prog_report.report("Creating merged calibration workspace")
        self._create_empty_calibrated_workspace(front_calib, self._CALIBRATED_WS_NAME)

        output_filename = self.getProperty(Prop.OUTPUT_FILE).value
        if output_filename:
            prog_report.report("Saving to file")
            self._save_ws_as_nexus(self._CALIBRATED_WS_NAME, output_filename)

    def _load_file(self, file_name: str, ws_name: str):
        alg = self._create_child_alg("Load", Filename=file_name, OutputWorkspace=ws_name)
        alg.execute()
        return alg.getProperty("OutputWorkspace").value

    def _create_empty_calibrated_workspace(self, merged_calib_ws, calibrated_ws_name: str) -> None:
        """Copy the instrument parameters of a merged calibration workspace into a new workspace with no counts"""

        inst_file_name = merged_calib_ws.getInstrumentFilename(merged_calib_ws.getInstrument().getName())
        load_inst_alg = self._create_child_alg("LoadEmptyInstrument", Filename=inst_file_name, OutputWorkspace=calibrated_ws_name)
        load_inst_alg.execute()
        calibrated_ws = load_inst_alg.getProperty("OutputWorkspace").value

        copy_param_alg = self._create_child_alg("CopyInstrumentParameters", InputWorkspace=merged_calib_ws, OutputWorkspace=calibrated_ws)
        copy_param_alg.execute()

        remove_history_alg = self._create_child_alg("RemoveWorkspaceHistory", Workspace=calibrated_ws)
        remove_history_alg.execute()

        AnalysisDataService.addOrReplace(calibrated_ws_name, calibrated_ws)

    def _save_ws_as_nexus(self, ws_name: str, filename: str) -> None:
        save_filepath = filename if filename.endswith(self._NEXUS_SUFFIX) else f"{filename}{self._NEXUS_SUFFIX}"
        save_alg = self._create_child_alg("SaveNexusProcessed", InputWorkspace=ws_name, Filename=save_filepath)
        save_alg.execute()

    def _create_child_alg(self, name: str, store_in_ADS: bool = False, **kwargs):
        alg = self.createChildAlgorithm(name, **kwargs)
        alg.setRethrows(True)
        if store_in_ADS:
            alg.setAlwaysStoreInADS(True)
        return alg


AlgorithmFactory.subscribe(SANSTubeMerge)
