from mantid.kernel import Direction, DateAndTime, FloatBoundedValidator
from mantid.api import PythonAlgorithm, AlgorithmFactory, WorkspaceProperty, FileProperty, FileAction
from mantid.simpleapi import (
    CreateWorkspace,
    LoadInstrument,
    AddSampleLogMultiple,
    AnalysisDataService,
    DeleteWorkspace,
    MoveInstrumentComponent,
)
from SANS1DataMLZ import SANSdata
import numpy as np


class LoadSANS1MLZ(PythonAlgorithm):
    """
    Load a SANS1_MLZ raw data file to the matrix workspace
    """

    def category(self):
        return "Workflow\\MLZ\\SANSOne;DataHandling\\Text"

    def name(self):
        return "LoadSANS1_MLZ"

    def summary(self):
        return "Load the SANS1_MLZ raw data file to the mantid workspace."

    def PyInit(self):
        self.declareProperty(FileProperty("Filename", "", FileAction.Load, [".001"]), "Name of SANS-1 experimental data file.")

        self.declareProperty(
            WorkspaceProperty("OutputWorkspace", "", direction=Direction.Output), doc="Name of the workspace to store experimental data."
        )

        self.declareProperty(
            name="Wavelength",
            defaultValue=0.0,
            validator=FloatBoundedValidator(lower=0.0),
            doc="Wavelength in Angstrom. If 0, the wavelength will be read from the data file.",
        )

    def PyExec(self):
        filename = self.getPropertyValue("Filename")
        out_ws_name = self.getPropertyValue("OutputWorkspace")
        metadata = SANSdata()

        metadata.analyze_source(filename)
        data_x, data_y, data_e, n_spec = self.create_datasets(metadata)
        logs = self.create_logs(metadata)
        y_unit, y_label, x_unit = self.create_labels()

        self._log_data_analyzing(metadata)
        CreateWorkspace(OutputWorkspace=out_ws_name, DataX=data_x, DataY=data_y, DataE=data_e, NSpec=n_spec, UnitX=x_unit)
        out_ws = AnalysisDataService.retrieve(out_ws_name)
        out_ws.setYUnit(y_unit)
        out_ws.setYUnitLabel(y_label)

        run = out_ws.mutableRun()
        run.addProperty("run_title", metadata.file.get_title(), True)
        run.setStartAndEndTime(DateAndTime(metadata.file.run_start()), DateAndTime(metadata.file.run_end()))

        LoadInstrument(out_ws, InstrumentName="sans-1", RewriteSpectraMap=True)
        MoveInstrumentComponent(Workspace=out_ws, ComponentName="detector", Z=str(metadata.setup.l2), RelativePosition=False)

        AddSampleLogMultiple(out_ws, LogNames=logs["names"], LogValues=logs["values"], LogUnits=logs["units"])

        self.setProperty("OutputWorkspace", out_ws)
        DeleteWorkspace(out_ws)

    def create_datasets(self, metadata: SANSdata) -> tuple:
        """
        :return: DataX, DataY, DataE and number of spectra
        (including monitors)
        """
        self.log().debug("Creation data for workspace started")

        n_spec = metadata.spectrum_amount()
        data_y = metadata.data_y()
        self._set_wavelength(metadata)
        data_e = metadata.data_e()
        data_x = metadata.data_x()

        self.log().debug("Creation data for workspace successful")
        return data_x, data_y, data_e, n_spec

    def create_logs(self, metadata: SANSdata) -> dict:
        """
        :return: logs with units
        warning! essential_data_tobe_logged should match
        with the most relevant parameters (defined in
        SANS1DataMLZ file using the following format ->
        'parameter_name: type = ...')
        """
        self.log().debug("Creation sample logs started")
        # units of the most relevant parameters
        essential_data_tobe_logged = {
            "det1_x_value": "mm",
            "det1_z_value": "mm",
            "wavelength": "Angstrom",
            "wavelength_error_mult": "",
            "st1_x_value": "",
            "st1_x_offset": "",
            "st1_y_value": "",
            "st1_y_offset": "",
            "st1_z_value": "",
            "st1_z_offset": "",
            "det1_omg_value": "degrees",
            "duration": "sec",
            "sum_all_counts": "",
            "monitor1": "",
            "monitor2": "",
            "l2": "m",
            "thickness": "mm",
            "position": "",
            "transmission": "",
            "scaling": "",
            "probability": "",
            "aperture": "",
            "collimation": "",
        }
        logs = {"names": [], "values": [], "units": []}
        sections_tobe_logged = metadata.get_subsequence()

        for section in sections_tobe_logged:
            logs["names"] = np.append(list(section.get_values_dict().keys()), logs["names"])
            logs["values"] = np.append(list(section.get_values_dict().values()), logs["values"])
        logs["units"] = np.append([essential_data_tobe_logged[variable] for variable in logs["names"]], logs["units"])
        for section in sections_tobe_logged:
            logs["names"] = np.append([f"{section.section_name}.{variable}" for variable in section.info.keys()], logs["names"])
            logs["values"] = np.append(list(section.info.values()), logs["values"])

        logs["units"] = np.append(["" for _ in range(len(logs["names"]) - len(logs["units"]))], logs["units"])
        self.log().debug("Creation sample logs successful")
        return logs

    def _set_wavelength(self, metadata):
        """
        check if a custom wavelength provided
        """
        user_wavelength = float(self.getPropertyValue("Wavelength"))
        if not self.getProperty("Wavelength").isDefault:
            metadata.setup.wavelength = user_wavelength
            self.log().notice("Wavelength set to user input.")
        if isinstance(metadata.setup.wavelength, str) or metadata.setup.wavelength == 0.0:
            self.log().error("Wavelength not defined.")

    def _log_data_analyzing(self, metadata):
        """
        show the notifications raised during
        the loading process of a SANS-1 datafile
        """
        for note in metadata.logs["notice"]:
            self.log().notice(note)
        for warn in metadata.logs["warning"]:
            self.log().warning(warn)

    @staticmethod
    def create_labels():
        y_unit = ""
        y_label = "Counts"
        x_unit = "Wavelength"
        return y_unit, y_label, x_unit


# Register algorithm with mantid
AlgorithmFactory.subscribe(LoadSANS1MLZ)
