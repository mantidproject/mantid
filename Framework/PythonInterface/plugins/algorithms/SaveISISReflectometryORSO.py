# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.utils.reflectometry.orso_helper import *

from mantid.kernel import Direction, Property
from mantid.api import AlgorithmFactory, WorkspaceProperty, FileProperty, FileAction, PythonAlgorithm, PropertyMode


class Prop:
    INPUT_WS = "InputWorkspace"
    WRITE_RESOLUTION = "WriteResolution"
    RESOLUTION = "Resolution"
    THETA = "ThetaIn"
    FILENAME = "Filename"


class SaveISISReflectometryORSO(PythonAlgorithm):
    """
    See https://www.reflectometry.org/ for more information about the ORSO .ort format
    """

    _FACILITY = "ISIS"
    _ISIS_DOI_PREFIX = "10.5286/ISIS.E.RB"
    _RB_NUM_LOGS = ("rb_proposal", "experiment_identifier")
    _INVALID_HEADER_COMMENT = "Mantid@ISIS output may not be fully ORSO compliant"

    def category(self):
        return "Reflectometry\\ISIS"

    def name(self):
        """Return the name of the algorithm."""
        return "SaveISISReflectometryORSO"

    def summary(self):
        """Return a summary of the algorithm."""
        return "Saves ISIS processed reflectometry workspaces into the ASCII implementation of the ORSO data standard."

    def PyInit(self):
        self.declareProperty(
            WorkspaceProperty(name=Prop.INPUT_WS, defaultValue="", direction=Direction.Input, optional=PropertyMode.Mandatory),
            doc="The workspace containing the processed reflectivity data to save",
        )

        self.declareProperty(
            name=Prop.WRITE_RESOLUTION,
            defaultValue=True,
            direction=Direction.Input,
            doc="Whether to compute resolution values and write them as the fourth data column.",
        )

        self.declareProperty(
            name=Prop.RESOLUTION,
            defaultValue=Property.EMPTY_DBL,
            direction=Direction.Input,
            doc="Logarithmic resolution used to process the reflectometry data."
            f"This value is only used if {Prop.WRITE_RESOLUTION} is set to True.",
        )

        self.declareProperty(
            name=Prop.THETA,
            defaultValue=Property.EMPTY_DBL,
            direction=Direction.Input,
            doc="Angle in degrees to use for calculating the resolution. "
            f"This value is only used if {Prop.WRITE_RESOLUTION} is set to True "
            f"and nothing has been provided for the {Prop.RESOLUTION} parameter.",
        )

        self.declareProperty(
            FileProperty(Prop.FILENAME, "", FileAction.Save, MantidORSOSaver.FILE_EXT), doc="File path to save the .ort file to"
        )

    def PyExec(self):
        ws = self.getProperty(Prop.INPUT_WS).value

        # We cannot include all the mandatory information required by the standard, so we include a comment to highlight
        # this at the top of the file. In future this comment will not be needed, or we may need to add validation to
        # determine if it should be included (although ideally validation would be implemented in the orsopy library).
        orso_saver = MantidORSOSaver(self.getProperty(Prop.FILENAME).value, self._INVALID_HEADER_COMMENT)

        # Create the file contents
        orso_saver.add_dataset(self._create_dataset(ws))

        # Write the file to disk in the ORSO ASCII format
        orso_saver.save_orso_ascii()

    def _create_dataset(self, ws, dataset_name=None):
        data_columns = self._create_data_columns(ws)
        dataset = MantidORSODataset(dataset_name, data_columns, ws)
        self._add_optional_header_info(dataset, ws)
        return dataset

    def _create_data_columns(self, ws):
        """
        Set up the column headers and data values
        """
        resolution = self._get_resolution(ws)

        alg = self.createChildAlgorithm("ConvertToPointData", InputWorkspace=ws, OutputWorkspace="pointData")
        alg.execute()
        point_data = alg.getProperty("OutputWorkspace").value

        q_data = point_data.extractX()[0]
        reflectivity = point_data.extractY()[0]
        reflectivity_error = point_data.extractE()[0]
        q_resolution = resolution if resolution is None else q_data * resolution

        data_columns = MantidORSODataColumns(q_data, reflectivity, reflectivity_error, q_resolution)

        return data_columns

    def _add_optional_header_info(self, dataset, ws):
        """
        Populate the non_mandatory data in the header
        """
        run = ws.getRun()
        rb_number, doi = self._get_rb_number_and_doi(run)
        dataset.set_facility(self._FACILITY)
        dataset.set_proposal_id(rb_number)
        dataset.set_doi(doi)

    def _get_rb_number_and_doi(self, run):
        """
        Check if the experiment RB number can be found in the workspace logs.
        This can be stored under different log names depending on whether time slicing was performed.
        If found, the RB number is used to provide the ISIS experiment DOI.
        """
        for log_name in self._RB_NUM_LOGS:
            if run.hasProperty(log_name):
                rb_num = str(run.getProperty(log_name).value)
                return rb_num, f"{self._ISIS_DOI_PREFIX}{rb_num}"
        return None, None

    def _get_resolution(self, ws):
        if not self.getProperty(Prop.WRITE_RESOLUTION).value:
            return None

        if not self.getProperty(Prop.RESOLUTION).isDefault:
            return self.getProperty(Prop.RESOLUTION).value

        # Attempt to calculate the resolution if it hasn't been passed in
        try:
            alg = self.createChildAlgorithm("NRCalculateSlitResolution", Workspace=ws)
            alg.setLogging(False)
            if not self.getProperty(Prop.THETA).isDefault:
                alg.setProperty("TwoTheta", 2 * self.getProperty(Prop.THETA).value)
            alg.execute()
            resolution = alg.getProperty("Resolution").value
            self.log().notice(f"Resolution for {ws.name()} calculated as {resolution}")
        except RuntimeError:
            self.log().warning(f"Unable to calculate resolution for {ws.name()}")
            return None

        return resolution


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SaveISISReflectometryORSO)
