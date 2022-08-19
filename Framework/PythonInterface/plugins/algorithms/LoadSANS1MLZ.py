from mantid.kernel import Direction, DateAndTime, FloatBoundedValidator, StringListValidator
from mantid.api import PythonAlgorithm, AlgorithmFactory, WorkspaceProperty, \
    FileProperty, FileAction
from mantid.simpleapi import CreateWorkspace, LoadInstrument, AddSampleLogMultiple, \
    AnalysisDataService, DeleteWorkspace
from SANS1DataMLZ import SANSdata
import numpy as np


class LoadSANS1MLZ(PythonAlgorithm):
    """
    Load a SANS1_MLZ raw data file to the matrix workspace
    """

    def category(self):
        return 'Workflow\\MLZ\\SANS-1;DataHandling\\Text'

    def name(self):
        return "LoadSANS1_MLZ"

    def summary(self):
        return "Load the SANS1_MLZ raw data file to the mantid workspace."

    def PyInit(self):
        self.declareProperty(FileProperty("Filename", "",
                                          FileAction.Load, ['.001', '.002']),
                             "Name of SANS experimental data file.")

        self.declareProperty(WorkspaceProperty("OutputWorkspace", "", direction=Direction.Output),
                             doc="Name of the workspace to store the experimental data.")

        self.declareProperty(name="Wavelength",
                             defaultValue=0.0,
                             validator=FloatBoundedValidator(lower=0.0),
                             doc="Wavelength in Angstrom. If 0, the wavelength "
                                 "will be read from the data file.")

        self.declareProperty(name='Mode',
                             defaultValue='vector',
                             validator=StringListValidator(['vector', '128x128']),
                             doc='Choose workspace type.')

    def PyExec(self):
        filename = self.getPropertyValue("Filename")
        out_ws_name = self.getPropertyValue("OutputWorkspace")
        workspace_mode = self.getPropertyValue('Mode')
        metadata = SANSdata()

        try:
            metadata.analyze_source(filename)
            data_x, data_y, data_e, n_spec = self.create_datasets(metadata, workspace_mode)
            logs = self.create_logs(metadata)
            y_unit, y_label, x_unit = self.create_labels(workspace_mode)
        except FileNotFoundError as error:
            raise RuntimeError(str(error))
        except TypeError as error:
            raise RuntimeError(str(error) + "\nprobably incorrect 'Counts' data")
        else:
            self._log_data_analyzing(metadata)
            CreateWorkspace(OutputWorkspace=out_ws_name,
                            DataX=data_x, DataY=data_y,
                            DataE=data_e, NSpec=n_spec,
                            UnitX=x_unit)
            out_ws = AnalysisDataService.retrieve(out_ws_name)
            out_ws.setYUnit(y_unit)
            out_ws.setYUnitLabel(y_label)

            run = out_ws.mutableRun()
            run.addProperty('run_title', metadata.file.get_title(), True)
            run.setStartAndEndTime(DateAndTime(metadata.file.run_start()),
                                   DateAndTime(metadata.file.run_end()))

            if workspace_mode != '128x128':
                LoadInstrument(out_ws, InstrumentName='sans-1', RewriteSpectraMap=True)

            AddSampleLogMultiple(out_ws,
                                 LogNames=logs["names"],
                                 LogValues=logs["values"],
                                 LogUnits=logs["units"])

            self.setProperty("OutputWorkspace", out_ws)
            DeleteWorkspace(out_ws)

    def create_datasets(self, metadata: SANSdata, workspace_mode) -> tuple:
        """
        :return: DataX, DataY, DataE and number of spectra
        (including monitors)
        """
        self.log().debug('Creation data for workspace started')

        if workspace_mode == '128x128':
            n_spec = 128
            data_y = metadata.counts.data
            self._wavelength(metadata)
            data_e = np.sqrt(data_y)
            data_x = range(n_spec)
        else:
            n_spec = metadata.spectrum_amount()
            data_y = metadata.data_y()
            self._wavelength(metadata)
            data_e = metadata.data_e()
            data_x = metadata.data_x()

        self.log().debug('Creation data for workspace successful')
        return data_x, data_y, data_e, n_spec

    def create_logs(self, metadata: SANSdata) -> dict:
        """
        :return: logs with units
        warning! essential_data_tobe_logged should match
        with main variables
        """
        self.log().debug('Creation sample logs started')
        # units of measurement of main variables
        essential_data_tobe_logged = {
            'det1_x_value': 'mm',
            'det1_z_value': 'mm',
            'wavelength': 'Angstrom',
            'wavelength_error_mult': '',
            'st1_x_value': '',
            'st1_x_offset': '',
            'st1_y_value': '',
            'st1_y_offset': '',
            'st1_z_value': '',
            'st1_z_offset': '',
            'det1_omg_value': 'degrees',
            'duration': 'sec',
            'sum_all_counts': '',
            'monitor1': '',
            'monitor2': '',
            'sample_detector_distance': 'mm',
            'thickness': 'mm',
            'position': '',
            'transmission': '',
            'scaling': '',
            'probability': '',
            'beamcenter_x': '',
            'beamcenter_y': '',
            'aperture': '',
            'collimation': '',
        }
        logs = {"names": [], "values": [], "units": []}
        sections_tobe_logged = metadata.get_subsequence()

        for section in sections_tobe_logged:
            logs["names"] = np.append(list(section.get_values_dict().keys()), logs["names"])
            logs["values"] = np.append(list(section.get_values_dict().values()),
                                       logs["values"])
        logs["units"] = np.append([essential_data_tobe_logged[variable] for variable in logs["names"]],
                                  logs["units"])
        for section in sections_tobe_logged:
            logs["names"] = np.append([f"{section.section_name}.{variable}" for variable in section.info.keys()],
                                      logs["names"])
            logs["values"] = np.append(list(section.info.values()), logs["values"])

        logs["units"] = np.append(['' for _ in range(len(logs['names']) - len(logs['units']))],
                                  logs["units"])
        self.log().debug('Creation sample logs successful')
        return logs

    def _wavelength(self, metadata):
        user_wavelength = float(self.getPropertyValue("Wavelength"))
        if user_wavelength > 0.001:
            metadata.setup.wavelength = user_wavelength
            self.log().notice('Wavelength set to user input.')
        if (type(metadata.setup.wavelength) is str) or (metadata.setup.wavelength == 0.0):
            self.log().error('Wavelength not defined.')

    def _log_data_analyzing(self, metadata):
        """
        show the notifications raised during
        analyzing SANS-1 datafile
        """
        for note in metadata.logs['notice']:
            self.log().notice(note)
        for warn in metadata.logs['warning']:
            self.log().warning(warn)

    @staticmethod
    def create_labels(workspace_mode):
        if workspace_mode != '128x128':
            y_unit = ""
            y_label = "Counts"
            x_unit = "Wavelength"
        else:
            y_unit = ""
            y_label = "Counts"
            x_unit = ""
        return y_unit, y_label, x_unit


# Register algorithm with mantid
AlgorithmFactory.subscribe(LoadSANS1MLZ)
