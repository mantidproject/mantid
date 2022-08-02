from mantid.kernel import *
from mantid.api import *
from mantid import simpleapi
from sansdata import SANSdata
import numpy as np


class LoadSANS1MLZ(PythonAlgorithm):
    """
    Load the SANS1_MLZ raw data file to the matrix workspace
    """

    def category(self):
        return 'Workflow\\MLZ\\SANS-1;DataHandling\\Text'

    def name(self):
        return "LoadSANS1_MLZ"

    def summary(self):
        return "Load the SANS1_MLZ raw data file to the mantid workspace."

    def PyInit(self):
        self.declareProperty(FileProperty("Filename", "",
                                          FileAction.Load, ['.001']),
                             "Name of SANS experimental data file.")

        self.declareProperty(WorkspaceProperty("OutputWorkspace", "", direction=Direction.Output),
                             doc="Name of the workspace to store the experimental data.")

        self.declareProperty(name="Wavelength",
                             defaultValue=0.0,
                             validator=FloatBoundedValidator(lower=0.0),
                             doc="Wavelength in Angstrom. If 0 will be read from data file.")

    def PyExec(self):
        filename = self.getPropertyValue("Filename")
        out_ws_name = self.getPropertyValue("OutputWorkspace")
        metadata = SANSdata()

        try:
            metadata.analyze_source(filename)
            data_x, data_y, data_e, n_spec = self.create_datasets(metadata)
            logs = self.create_logs(metadata)
            y_unit, y_label, x_unit = self.create_labels()
        except FileNotFoundError as e:
            raise RuntimeError(str(e))
        except TypeError as e:
            raise RuntimeError(str(e) + "\nprobably incorrect 'Counts' data")
        else:
            simpleapi.CreateWorkspace(OutputWorkspace=out_ws_name,
                                      DataX=data_x, DataY=data_y,
                                      DataE=data_e, NSpec=n_spec,
                                      UnitX=x_unit)
            out_ws = simpleapi.AnalysisDataService.retrieve(out_ws_name)
            out_ws.setYUnit(y_unit)
            out_ws.setYUnitLabel(y_label)

            run = out_ws.mutableRun()
            run.addProperty('run_title', metadata.file.get_title(), True)
            run.setStartAndEndTime(DateAndTime(metadata.file.run_start()), DateAndTime(metadata.file.run_end()))

            simpleapi.LoadInstrument(out_ws, InstrumentName='sans-1', RewriteSpectraMap=True)

            simpleapi.AddSampleLogMultiple(out_ws,
                                           LogNames=logs["names"],
                                           LogValues=logs["values"],
                                           LogUnits=logs["units"])
            self.setProperty("OutputWorkspace", out_ws)

    def create_datasets(self, metadata):
        """
        return data values: DataX, DataY, DataE, amount of spectra
        monitors included
        """
        self.log().debug('Creation data for workspace started')
        nrows = len(metadata.counts.data)
        nbins = len(metadata.counts.data[0])
        n_spec = nrows * nbins

        if nrows != 128 or nbins != 128:
            raise RuntimeError("'Counts' section include incorrect data:"
                               " must be 128x128")

        wavelength_error = 0.15  # ToDo error?? wavelength - error > 0 !!
        metadata.comment.set_wavelength(float(self.getPropertyValue("Wavelength")))
        data_y = np.append([], metadata.counts.data)
        if metadata.counter.is_monitors_exist():
            n_spec += 2
            data_y = np.append(data_y, [metadata.counter.monitor1, metadata.counter.monitor2])
        data_e = np.array(np.sqrt(data_y))
        data_x = np.zeros(2 * n_spec)
        data_x.fill(metadata.comment.wavelength + wavelength_error)
        data_x[::2] -= wavelength_error * 2

        self.log().debug('Creation data for workspace successful')
        return data_x, data_y, data_e, n_spec

    def create_logs(self, metadata):
        """
        create logs with units
        warning! essential_data_tobe_logged should match
        """
        self.log().debug('Creation sample logs started')
        # units of measurement of main variables
        essential_data_tobe_logged = {
            'det1_x_value': 'mm',
            'det1_z_value': 'mm',
            'wavelength': 'Angstrom',
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
        }

        logs = {"names": [], "values": [], "units": []}

        for i in metadata.get_subsequence()[:-1]:
            logs["names"] = np.append(list(i.get_values_dict().keys()), logs["names"])
            logs["values"] = np.append(list(i.get_values_dict().values()), logs["values"])
        logs["units"] = np.append([essential_data_tobe_logged[j] for j in logs["names"]],  logs["units"])
        for i in metadata.get_subsequence()[:-1]:
            logs["names"] = np.append([f"{i.section_name}.{j}" for j in i.info.keys()], logs["names"])
            logs["values"] = np.append(list(i.info.values()), logs["values"])

        logs["units"] = np.append(['' for i in range(len(logs['names']) - len(logs['units']))],
                                  logs["units"])
        self.log().debug('Creation sample logs successful')
        return logs

    @staticmethod
    def create_labels():
        yunit = "Counts"
        ylabel = "Counts"
        xunit = "Wavelength"
        return yunit, ylabel, xunit


# Register algorithm with mantid
AlgorithmFactory.subscribe(LoadSANS1MLZ)
