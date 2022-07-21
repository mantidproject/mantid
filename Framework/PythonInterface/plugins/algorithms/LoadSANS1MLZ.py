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

        self.declareProperty(WorkspaceProperty("OutputWorkspace",
                                               "", direction=Direction.Output),
                             doc="Name of the workspace to store the experimental data.")

        self.declareProperty("SectionOption", "EssentialData",
                             StringListValidator(["EssentialData", "CommentSection", "AllSections"]))

        self.declareProperty(name="Wavelength", defaultValue=0.0, validator=FloatBoundedValidator(lower=0.0),
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
            simpleapi.CreateWorkspace(OutputWorkspace=out_ws_name, DataX=data_x, DataY=data_y,
                                      DataE=data_e, NSpec=n_spec, UnitX=x_unit)
            out_ws = simpleapi.AnalysisDataService.retrieve(out_ws_name)
            out_ws.setYUnit(y_unit)
            out_ws.setYUnitLabel(y_label)

            run = out_ws.mutableRun()
            run.addProperty('run_title',
                            metadata.file.info['Title'] if metadata.file.info['Title'] else
                            metadata.file.info['FileName'], True)
            run.setStartAndEndTime(DateAndTime(metadata.file.run_start()),
                                   DateAndTime(metadata.file.run_end()))

            simpleapi.LoadInstrument(out_ws, InstrumentName='sans-1', RewriteSpectraMap=True)

            simpleapi.AddSampleLogMultiple(out_ws, LogNames=logs["names"], LogValues=logs["values"],
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
        wavelength_error = 0.15
        tmp_wavelength = float(self.getPropertyValue("Wavelength"))
        if tmp_wavelength > 0:
            metadata.comment.wavelength = tmp_wavelength
            metadata.comment.info['selector_lambda_value'] = tmp_wavelength

        if nrows != 128 or nbins != 128:
            raise RuntimeError("'Counts' section include incorrect data:"
                               " must be 128x128")
        n_spec = nrows * nbins + 2
        # ToDo monitors ; refactor numbers 2...
        datay = np.append([], metadata.counts.data)
        datay = np.append(datay, [100000, 0])
        datae = np.array(np.sqrt(datay))
        datax = np.zeros(2 * n_spec)
        datax.fill(metadata.comment.wavelength + wavelength_error)
        datax[::2] -= wavelength_error * 2

        self.log().debug('Creation data for workspace successful')
        return datax, datay, datae, n_spec

    def create_logs(self, metadata):
        self.log().debug('Creation sample logs started')
        data_tobe_logged = {'det1_x_value': 'mm',
                            'det1_z_value': 'mm',
                            'wavelength': 'Angstrom',
                            'st1_x_value': '',
                            'st1_x_offset': '',
                            'st1_y_value': '',
                            'st1_y_offset': '',
                            'st1_z_value': '',
                            'st1_z_offset': '',
                            'det1_omg_value': 'degree'
                            }

        option = self.getPropertyValue("SectionOption")
        logs = {"names": [],
                "values": [],
                "units": []}
        if option == 'EssentialData':
            for i in data_tobe_logged.keys():
                logs["names"].append(i)
                logs["values"].append(metadata.comment.__dict__[i])
                logs["units"].append(data_tobe_logged[i])

        elif option == 'CommentSection':
            for i in metadata.comment.info.keys():
                logs["names"].append(i)
                logs["values"].append(metadata.comment.info[i])
        elif option == 'AllSections':
            for section in metadata.get_subsequence()[:-1]:
                for i in section.info.keys():
                    logs["names"].append(i)
                    logs["values"].append(section.info[i])
        self.log().debug('Creation sample logs successful')
        return logs

    @staticmethod
    def create_labels():
        yunit = "Counts"
        ylabel = "Intensity"
        xunit = "Wavelength"
        return yunit, ylabel, xunit


# Register algorithm with mantid
AlgorithmFactory.subscribe(LoadSANS1MLZ)
