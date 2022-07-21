from mantid.kernel import *
from mantid.api import *
from mantid import simpleapi
from sansdata import SANSdata
import numpy as np


class LoadSANSLegacy(PythonAlgorithm):

    def category(self):
        return "Examples"

    def name(self):
        return "LoadSANS1_MLZ"

    def summary(self):
        return "Load the SANS1_MLZ Legacy data file to the mantid workspace."

    def PyInit(self):
        self.declareProperty(FileProperty("Filename", "",
                                          FileAction.Load, ['.001']),
                             "Name of SANS experimental data file.")

        self.declareProperty(WorkspaceProperty("OutputWorkspace",
                                               "", direction=Direction.Output),
                             doc="Name of the workspace to store the experimental data.")

        self.declareProperty("SectionOption", "CommentSection",
                             StringListValidator(["CommentSection", "AllSections"]))

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
        except FileNotFoundError as e:
            raise RuntimeError(str(e))
        except TypeError as e:
            raise RuntimeError(str(e) + "\nprobably incorrect 'Counts' data")
        else:
            yunit = "Counts"
            ylabel = "Intensity"
            xunit = "Wavelength"
            simpleapi.CreateWorkspace(OutputWorkspace=out_ws_name, DataX=data_x, DataY=data_y,
                                      DataE=data_e, NSpec=n_spec, UnitX=xunit)
            outws = simpleapi.AnalysisDataService.retrieve(out_ws_name)

            run = outws.mutableRun()
            # ToDo split functionality; why all in 1 function
            run.addProperty('run_title',
                            metadata.file.info['Title'] if metadata.file.info['Title'] else
                            metadata.file.info['FileName'], True)
            run.setStartAndEndTime(DateAndTime(metadata.file.run_start()),
                                   DateAndTime(metadata.file.run_end()))
            outws.setYUnit(yunit)
            outws.setYUnitLabel(ylabel)

            simpleapi.LoadInstrument(outws, InstrumentName='sans-1', RewriteSpectraMap=True)
            self.log().notice('wow')

            simpleapi.AddSampleLogMultiple(outws, LogNames=logs["names"], LogValues=logs["values"],
                                           LogUnits=logs["units"])
            self.setProperty("OutputWorkspace", outws)

    def create_datasets(self, metadata):
        self.log().debug('Creation data for workspace started')
        nrows = len(metadata.counts.data)
        nbins = len(metadata.counts.data[0])
        wavelength_error = 0.15
        tmp_wavelength = float(self.getPropertyValue("Wavelength"))
        if tmp_wavelength > 0:
            metadata.comment.wavelength = tmp_wavelength

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
        if option == 'CommentSection':
            for i in data_tobe_logged.keys():
                logs["names"].append(i)
                logs["values"].append(metadata.comment.__dict__[i])
                logs["units"].append(data_tobe_logged[i])

        elif option == 'AllSections':
            for i in metadata.comment.info.keys():
                logs["names"].append(i)
                logs["values"].append(metadata.comment.info[i])
        self.log().debug('Creation sample logs successful')
        return logs


# Register algorithm with mantid
AlgorithmFactory.subscribe(LoadSANSLegacy)
