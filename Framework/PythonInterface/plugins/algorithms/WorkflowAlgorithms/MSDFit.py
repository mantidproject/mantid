# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
from mantid.api import (
    mtd,
    AlgorithmFactory,
    DataProcessorAlgorithm,
    ITableWorkspaceProperty,
    MatrixWorkspace,
    MatrixWorkspaceProperty,
    NumericAxis,
    Progress,
    PropertyMode,
    WorkspaceGroupProperty,
)
from mantid.kernel import logger, Direction, StringListValidator
from mantid.simpleapi import PlotPeakByLogValue


class MSDFit(DataProcessorAlgorithm):
    _output_fit_ws = None
    _model = None
    _spec_range = None
    _x_range = None
    _input_ws = None
    _output_param_ws = None
    _output_msd_ws = None

    def category(self):
        return "Workflow\\MIDAS"

    def summary(self):
        return "Fits Intensity vs Q for 3 models to obtain the mean squared displacement."

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "", direction=Direction.Input), doc="Sample input workspace")
        self.declareProperty(
            name="Model",
            defaultValue="Gauss",
            validator=StringListValidator(["Gauss", "Peters", "Yi"]),
            doc="Model options : Gauss, Peters, Yi",
        )

        self.declareProperty(name="XStart", defaultValue=0.0, doc="Start of fitting range")
        self.declareProperty(name="XEnd", defaultValue=0.0, doc="End of fitting range")

        self.declareProperty(name="SpecMin", defaultValue=0, doc="Start of spectra range to be fit")
        self.declareProperty(name="SpecMax", defaultValue=0, doc="End of spectra range to be fit")

        self.declareProperty(
            MatrixWorkspaceProperty("OutputWorkspace", "", direction=Direction.Output), doc="Output mean squared displacement"
        )

        self.declareProperty(
            ITableWorkspaceProperty("ParameterWorkspace", "", direction=Direction.Output, optional=PropertyMode.Optional),
            doc="Output fit parameters table",
        )

        self.declareProperty(
            WorkspaceGroupProperty("FitWorkspaces", "", direction=Direction.Output, optional=PropertyMode.Optional),
            doc="Output fitted workspaces",
        )

    def validateInputs(self):
        issues = dict()

        workspace = self.getProperty("InputWorkspace").value
        x_min = self.getProperty("XStart").value
        x_max = self.getProperty("XEnd").value
        spec_min = self.getProperty("SpecMin").value
        spec_max = self.getProperty("SpecMax").value

        if isinstance(workspace, MatrixWorkspace):
            x_data = workspace.readX(0)
            if x_min < x_data[0]:
                issues["XStart"] = "Must be greater than minimum X value in workspace"

            if x_max > x_data[-1]:
                issues["XEnd"] = "Must be less than maximum X value in workspace"

            if spec_max > workspace.getNumberHistograms():
                issues["SpecMax"] = "Maximum spectrum number must be less than number of spectra in workspace"
        else:
            issues["InputWorkspace"] = "The InputWorkspace must be a MatrixWorkspace."

        if x_min > x_max:
            msg = "XStart must be less than XEnd"
            issues["XStart"] = msg
            issues["XEnd"] = msg

        if spec_min < 0:
            issues["SpecMin"] = "Minimum spectrum number must be greater than or equal to 0"

        if spec_min > spec_max:
            msg = "SpecMin must be less than SpecMax"
            issues["SpecMin"] = msg
            issues["SpecMax"] = msg

        return issues

    def PyExec(self):
        self._setup()
        progress = Progress(self, 0.0, 0.05, 3)
        self._original_ws = self._input_ws

        rename_alg = self.createChildAlgorithm("RenameWorkspace", enableLogging=False)
        rename_alg.setProperty("InputWorkspace", self._input_ws)
        rename_alg.setProperty("OutputWorkspace", self._input_ws + "_" + self._model)
        rename_alg.execute()
        self._input_ws = self._input_ws + "_" + self._model
        input_params = [self._input_ws + ",i%d" % i for i in range(self._spec_range[0], self._spec_range[1] + 1)]

        # Fit line to each of the spectra
        if self._model == "Gauss":
            logger.information("Model : Gaussian approximation")
            function = "name=MsdGauss, Height=1.0, Msd=0.1"
            function += ",constraint=(Height>0.0, Msd>0.0)"
            params_list = ["Height", "Msd"]
        elif self._model == "Peters":
            logger.information("Model : Peters & Kneller")
            function = "name=MsdPeters, Height=1.0, Msd=1.0, Beta=1.0"
            function += ",constraint=(Height>0.0, Msd>0.0, 100.0>Beta>0.3)"
            params_list = ["Height", "Msd", "Beta"]
        elif self._model == "Yi":
            logger.information("Model : Yi et al")
            function = "name=MsdYi, Height=1.0, Msd=1.0, Sigma=0.1"
            function += ",constraint=(Height>0.0, Msd>0.0, Sigma>0.0)"
            params_list = ["Height", "Msd", "Sigma"]
        else:
            raise ValueError("No Model defined")

        input_params = ";".join(input_params)
        progress.report("Sequential fit")
        PlotPeakByLogValue(
            Input=input_params,
            OutputWorkspace=self._output_msd_ws,
            Function=function,
            StartX=self._x_range[0],
            EndX=self._x_range[1],
            FitType="Sequential",
            CreateOutput=True,
        )

        delete_alg = self.createChildAlgorithm("DeleteWorkspace", enableLogging=False)
        delete_alg.setProperty("Workspace", self._output_msd_ws + "_NormalisedCovarianceMatrices")
        delete_alg.execute()
        delete_alg.setProperty("Workspace", self._output_msd_ws + "_Parameters")
        delete_alg.execute()
        rename_alg = self.createChildAlgorithm("RenameWorkspace", enableLogging=False)
        rename_alg.setProperty("InputWorkspace", self._output_msd_ws)
        rename_alg.setProperty("OutputWorkspace", self._output_param_ws)
        rename_alg.execute()

        progress.report("Create output files")

        # Create workspaces for each of the parameters
        parameter_ws_group = []
        for par in params_list:
            ws_name = self._output_msd_ws + "_" + par
            parameter_ws_group.append(ws_name)
            convert_alg = self.createChildAlgorithm("ConvertTableToMatrixWorkspace", enableLogging=False)
            convert_alg.setProperty("InputWorkspace", self._output_param_ws)
            convert_alg.setProperty("OutputWorkspace", ws_name)
            convert_alg.setProperty("ColumnX", "axis-1")
            convert_alg.setProperty("ColumnY", par)
            convert_alg.setProperty("ColumnE", par + "_Err")
            convert_alg.execute()
            mtd.addOrReplace(ws_name, convert_alg.getProperty("OutputWorkspace").value)

        append_alg = self.createChildAlgorithm("AppendSpectra", enableLogging=False)
        append_alg.setProperty("InputWorkspace1", self._output_msd_ws + "_" + params_list[0])
        append_alg.setProperty("InputWorkspace2", self._output_msd_ws + "_" + params_list[1])
        append_alg.setProperty("ValidateInputs", False)
        append_alg.setProperty("OutputWorkspace", self._output_msd_ws)
        append_alg.execute()
        mtd.addOrReplace(self._output_msd_ws, append_alg.getProperty("OutputWorkspace").value)
        if len(params_list) > 2:
            append_alg.setProperty("InputWorkspace1", self._output_msd_ws)
            append_alg.setProperty("InputWorkspace2", self._output_msd_ws + "_" + params_list[2])
            append_alg.setProperty("ValidateInputs", False)
            append_alg.setProperty("OutputWorkspace", self._output_msd_ws)
            append_alg.execute()
            mtd.addOrReplace(self._output_msd_ws, append_alg.getProperty("OutputWorkspace").value)
        for par in params_list:
            delete_alg.setProperty("Workspace", self._output_msd_ws + "_" + par)
            delete_alg.execute()

        progress.report("Change axes")
        # Sort ascending x
        sort_alg = self.createChildAlgorithm("SortXAxis", enableLogging=False)
        sort_alg.setProperty("InputWorkspace", self._output_msd_ws)
        sort_alg.setProperty("OutputWorkspace", self._output_msd_ws)
        sort_alg.execute()
        mtd.addOrReplace(self._output_msd_ws, sort_alg.getProperty("OutputWorkspace").value)
        # Create a new x axis for the Q and Q**2 workspaces
        xunit = mtd[self._output_msd_ws].getAxis(0).setUnit("Label")
        xunit.setLabel("Temperature", "K")
        # Create a new vertical axis for the Q and Q**2 workspaces
        y_axis = NumericAxis.create(len(params_list))
        for idx in range(len(params_list)):
            y_axis.setValue(idx, idx)
        mtd[self._output_msd_ws].replaceAxis(1, y_axis)

        # Rename fit workspace group
        original_fit_ws_name = self._output_msd_ws + "_Workspaces"
        if original_fit_ws_name != self._output_fit_ws:
            rename_alg.setProperty("InputWorkspace", self._output_msd_ws + "_Workspaces")
            rename_alg.setProperty("OutputWorkspace", self._output_fit_ws)
            rename_alg.execute()

        # Add sample logs to output workspace
        copy_alg = self.createChildAlgorithm("CopyLogs", enableLogging=False)
        copy_alg.setProperty("InputWorkspace", self._input_ws)
        copy_alg.setProperty("OutputWorkspace", self._output_msd_ws)
        copy_alg.execute()
        copy_alg.setProperty("InputWorkspace", self._input_ws)
        copy_alg.setProperty("OutputWorkspace", self._output_fit_ws)
        copy_alg.execute()

        rename_alg = self.createChildAlgorithm("RenameWorkspace", enableLogging=False)
        rename_alg.setProperty("InputWorkspace", self._input_ws)
        rename_alg.setProperty("OutputWorkspace", self._original_ws)
        rename_alg.execute()

        self.setProperty("OutputWorkspace", self._output_msd_ws)
        self.setProperty("ParameterWorkspace", self._output_param_ws)
        self.setProperty("FitWorkspaces", self._output_fit_ws)

    def _setup(self):
        """
        Gets algorithm properties.
        """
        self._input_ws = self.getPropertyValue("InputWorkspace")
        self._model = self.getPropertyValue("Model")
        self._output_msd_ws = self.getPropertyValue("OutputWorkspace")

        self._output_param_ws = self.getPropertyValue("ParameterWorkspace")
        if self._output_param_ws == "":
            self._output_param_ws = self._output_msd_ws + "_Parameters"

        self._output_fit_ws = self.getPropertyValue("FitWorkspaces")
        if self._output_fit_ws == "":
            self._output_fit_ws = self._output_msd_ws + "_Workspaces"

        self._x_range = [self.getProperty("XStart").value, self.getProperty("XEnd").value]

        self._spec_range = [self.getProperty("SpecMin").value, self.getProperty("SpecMax").value]


AlgorithmFactory.subscribe(MSDFit)
