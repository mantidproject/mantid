import numpy as np
import datetime
from orsopy import fileio

from mantid.kernel import Direction, FloatMandatoryValidator, version
from mantid.api import AlgorithmFactory, WorkspaceProperty, FileProperty, FileAction, PythonAlgorithm
from mantid.simpleapi import ConvertToPointData


class SaveReflectometryORSO(PythonAlgorithm):
    """
    Saves ISIS processed reflectometry workspaces into the ORSO .ort format (https://www.reflectometry.org/)
    """

    def category(self):
        return "Reflectometry\\ISIS"

    def PyInit(self):
        """
        Parameters
        ----------
        InputWorkspace: Workspace, the reflectometry workspace to be saved into the ORSO format.
        OutputFile: str, the filepath for the output .ort file to be saved to
        """
        self.declareProperty(
            WorkspaceProperty(name="InputWorkspace", defaultValue="", direction=Direction.Input),
            doc="Processed reflectivity workspace to save",
        )

        self.declareProperty(
            name="Resolution",
            defaultValue=0.02,
            direction=Direction.Input,
            doc="Logarithmic resolution used to process the reflectometry data",
            validator=FloatMandatoryValidator(),
        )

        self.declareProperty(FileProperty("Filename", "", FileAction.Save, ".ort"), doc="File path to write the .ort file to.")

    def PyExec(self):
        """
        Extracts all of the available metadata from a workspace created by ReflectometryReductionOneAuto and
        saves it to the ascii implementation of the ORSO data standard.
        """
        wksp = self.getProperty("InputWorkspace").value
        resolution = self.getProperty("Resolution").value
        save_filename = self.getProperty("Filename").value

        rb_number = wksp.getRun().getLogData("rb_proposal").value  # ISIS Specific
        # Maybe ISIS Specific
        start_time = datetime.datetime.strptime(wksp.getRun().getLogData("run_start").value, "%Y-%m-%dT%H:%M:%S")

        # Create and assign the metadata to the ORSO header
        header = fileio.orso.Orso.empty()

        header.data_source.experiment.probe = "neutron"
        header.data_source.experiment.facility = "ISIS"  # This may be able to be extracted from Mantid/workspace?
        header.data_source.experiment.proposalID = str(rb_number)  # ISIS Specific
        header.data_source.experiment.doi = f"10.5286/ISIS.E.RB{rb_number}"  # ISIS Specific
        header.data_source.experiment.instrument = wksp.getInstrument().getName()
        header.data_source.experiment.start_date = start_time

        header.data_source.sample.name = wksp.getTitle()

        # This is likely to change as is currently buggy in orsopy
        header.reduction.software = "Mantid"
        header.reduction.name = "Mantid Workbench"
        header.reduction.version = str(version())

        # ISIS Specific unit
        q_column = fileio.base.Column(name="Qz", unit="1/angstrom", physical_quantity="wavevector transfer")
        r_column = fileio.base.Column(name="R", unit=None, physical_quantity="reflectivity")
        # ISIS Specific value_is for both R and dQ
        dr_column = fileio.base.ErrorColumn(error_of="R", error_type="uncertainty", value_is="sigma")
        dq_column = fileio.base.ErrorColumn(error_of="Qz", error_type="resolution", value_is="sigma")

        # Combine header columns together and then check it complies with the standard
        header.columns = [q_column, r_column, dr_column, dq_column]
        _correct_header = fileio.orso.Orso(**header.to_dict())

        # Writing the data needs to be after header is converted to dict
        _tempwksp = ConvertToPointData(InputWorkspace=wksp)
        q = _tempwksp.extractX()[0]
        R = _tempwksp.extractY()[0]
        dR = _tempwksp.extractE()[0]
        dq = q * resolution
        dataset = fileio.orso.OrsoDataset(info=header, data=np.array([q, R, dR, dq]).T)

        # Write the file to disk
        fileio.orso.save_orso(datasets=[dataset], fname=save_filename)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SaveReflectometryORSO)
