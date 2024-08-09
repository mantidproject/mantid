# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-many-locals
from mantid.api import PythonAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty, PropertyMode, Progress, TextAxis
from mantid.kernel import Direction, StringListValidator
from mantid.simpleapi import mtd


def set_y_axis_labels(workspace, labels):
    axis = TextAxis.create(len(labels))
    for index, label in enumerate(labels):
        axis.setLabel(index, label)
    workspace.replaceAxis(1, axis)


class IndirectTwoPeakFit(PythonAlgorithm):

    _sample_workspace = None
    _res_ws = None
    _e_min = None
    _e_max = None
    _bgd = None
    _elastic = None
    _parameter_table = None
    _output_workspace = None
    _temporary_fit_name = None

    def category(self):
        return "Workflow\\Inelastic;Workflow\\MIDAS"

    def summary(self):
        return "Performs a convolution fit for 1 and 2 Lorentzians."

    def PyInit(self):
        self.declareProperty(
            MatrixWorkspaceProperty("SampleWorkspace", "", optional=PropertyMode.Mandatory, direction=Direction.Input),
            doc="Name for the sample workspace.",
        )

        self.declareProperty(name="EnergyMin", defaultValue=-0.5, doc="Minimum energy for fit. Default=-0.5")

        self.declareProperty(name="EnergyMax", defaultValue=0.5, doc="Maximum energy for fit. Default=0.5")

        self.declareProperty(
            name="Minimizer",
            defaultValue="Levenberg-Marquardt",
            validator=StringListValidator(["Levenberg-Marquardt", "FABADA"]),
            doc="Type of minimizer",
        )

        self.declareProperty(name="MaxIterations", defaultValue=500, doc="Max iterations. Default=500")

        self.declareProperty(name="OutputName", defaultValue="", doc="Output workspace base name")

    def validateInputs(self):
        self._setup()
        issues = dict()

        if self._e_max <= self._e_min:
            issues["EnergyMax"] = "Energy maximum must be greater than energy minimum."

        if self._max_iterations == 0 or self._max_iterations == "":
            issues["MaxIterations"] = "Maximum iterations must be greater than 0."

        return issues

    def _setup(self):
        self._sample_workspace = self.getProperty("SampleWorkspace").value

        self._e_min = self.getProperty("EnergyMin").value
        self._e_max = self.getProperty("EnergyMax").value

        self._minimizer = self.getProperty("Minimizer").value
        self._max_iterations = self.getProperty("MaxIterations").value

        self._output_name = self.getProperty("OutputName").value
        if self._output_name == "":
            self._output_name = "Two_peak"

    def PyExec(self):
        from IndirectCommon import convert_to_elastic_q

        progress_tracker = Progress(self, start=0.05, end=0.95, nreports=2)

        # Convert sample workspace to elastic Q
        self._temporary_fit_name = "__fit_ws"
        self._crop_workspace(self._sample_workspace, self._temporary_fit_name, self._e_min, self._e_max)

        self._convert_to_histogram(self._temporary_fit_name)
        convert_to_elastic_q(self._temporary_fit_name)

        # Perform fits
        progress_tracker.report("Fitting 1 peak...")
        self._fit("1L")
        progress_tracker.report("Fitting 2 peaks...")
        self._fit("2L")
        self._delete_workspace(self._temporary_fit_name)

        # Appends the chi workspaces and replaces the y labels
        self._append_chi_squared_workspaces()

        # Appends the result workspaces and replaces the y labels
        self._append_result_workspaces()

    def _fit(self, fit_type):
        self._fit_type = fit_type
        self._output_workspace = self._output_name + "_" + self._fit_type

        # Execute PlotPeakByLogValue
        self._plot_peak_by_log_value(
            self._create_input_string(),
            self._create_function_string(self._fit_type),
            self._e_min,
            self._e_max,
            "Sequential",
            self._minimizer,
            self._max_iterations,
            self._output_workspace,
        )

        # Remove unused workspaces
        self._delete_workspace(self._output_workspace + "_NormalisedCovarianceMatrices")
        self._delete_workspace(self._output_workspace + "_Parameters")

        # Rename workspaces to match user input
        self._fit_group_name = self._output_workspace + "_Workspaces"
        self._parameter_name = self._output_workspace + "_Parameters"
        self._rename_workspace(self._output_workspace, self._parameter_name)

        # Create result workspace
        self._result_name = self._output_workspace + "_Result"
        self._process_indirect_fit_parameters(
            self._parameter_name, "axis-1", "MomentumTransfer", self._get_fit_parameters(fit_type), self._result_name
        )

        self._transfer_sample_logs(self._result_name)

        # Convert table workspace to matrix workspace
        chi_workspace = self._output_workspace + "_ChiSq"
        self._convert_to_matrix_workspace(self._parameter_name, "axis-1", "Chi_squared", chi_workspace)

        self._transfer_sample_logs(chi_workspace)

        # Rename and transfer sample logs
        self._rename_and_transfer_logs()

    def _create_input_string(self):
        number_of_histograms = mtd[self._temporary_fit_name].getNumberHistograms()
        input_string = [self._temporary_fit_name + ",i" + str(i) for i in range(number_of_histograms - 1)]
        input_string = ";".join(input_string)
        return input_string

    def _rename_and_transfer_logs(self):
        workspace_names = mtd[self._fit_group_name].getNames()
        for index, workspace_name in enumerate(workspace_names):
            output_ws = self._output_workspace + "_" + str(index) + "_Workspace"
            self._rename_workspace(workspace_name, output_ws)
            self._transfer_sample_logs(output_ws)

    def _get_fit_parameters(self, fit_type):
        parameters = ""
        if fit_type == "1L":
            parameters += "A0,Amplitude,FWHM"
            if self._elastic:
                parameters += ",Height"
        elif fit_type == "2L":
            parameters += "A0,f0.Amplitude,f0.FWHM,f1.Amplitude,f1.FWHM"
            if self._elastic:
                parameters += ",f2.Height"
        return parameters

    def _append_chi_squared_workspaces(self):
        # Append chi_squared workspaces
        chi_name = self._output_name + "_ChiSq"
        chi_1L_name = self._output_name + "_1L_ChiSq"
        chi_2L_name = self._output_name + "_2L_ChiSq"
        self._clone_workspace(chi_1L_name, chi_name)
        self._append_to(chi_name, chi_2L_name)

        # Replace y axis labels
        set_y_axis_labels(mtd[chi_name], ["1 peak", "2 peaks"])

        # Delete unwanted workspaces
        self._delete_workspace(chi_1L_name)
        self._delete_workspace(chi_2L_name)

    def _append_result_workspaces(self):
        # Append result workspaces
        result_name = self._output_name + "_Result"
        result_1L_name = self._output_name + "_1L_Result"
        result_2L_name = self._output_name + "_2L_Result"
        temporary_name = "__spectrum"
        self._extract_single_spectrum(result_1L_name, result_name, 1)
        self._extract_single_spectrum(result_2L_name, temporary_name, 1)
        self._append_to(result_name, temporary_name)
        self._extract_single_spectrum(result_2L_name, temporary_name, 3)
        self._append_to(result_name, temporary_name)

        # Replace y axis labels
        set_y_axis_labels(mtd[result_name], ["fwhm.1", "fwhm.2.1", "fwhm.2.2"])

        # Delete unwanted workspaces
        self._delete_workspace(result_1L_name)
        self._delete_workspace(result_2L_name)
        self._delete_workspace(self._output_name + "_1L_Parameters")
        self._delete_workspace(self._output_name + "_2L_Parameters")

    def _transfer_sample_logs(self, workspace):
        """
        Copy the sample logs from the input workspace and add them to the output workspaces
        """
        sample_logs = {"e_min": self._e_min, "e_max": self._e_max, "fit_type": self._fit_type}

        self._copy_log(self._sample_workspace, workspace)

        log_names = [item for item in sample_logs]
        log_values = [sample_logs[item] for item in sample_logs]

        self._add_sample_log_multiple(workspace, log_names, log_values)

    def _create_function_string(self, fit_type):
        function = ""
        if fit_type == "1L":
            function += self._create_1L_function("__temporary")
        elif fit_type == "2L":
            function += self._create_2L_function("__temporary")

        self._delete_workspace("__temporary")
        return function

    def _create_1L_function(self, temporary_name):
        self._extract_single_spectrum(self._sample_workspace, temporary_name, 0)

        amplitude, fwhm = self._get_amplitude_and_fwhm_from_fit(temporary_name)
        function = "name=Lorentzian,Amplitude={0},PeakCentre=0.0,FWHM={1}".format(str(amplitude), str(fwhm))
        function += ",constraint=(Amplitude>0.0,FWHM>0.0)"
        return function

    def _create_2L_function(self, temporary_name):
        self._extract_single_spectrum(self._fit_group_name[:-10] + "0_Workspace", temporary_name, 2)

        # Make any negative y values positive
        self._force_positive_y_values(temporary_name, 0)

        amplitude0, fwhm0 = mtd[self._result_name].readY(0)[0], mtd[self._result_name].readY(1)[0]
        amplitude1, fwhm1 = self._get_amplitude_and_fwhm_from_fit(temporary_name)
        function = "name=Lorentzian,Amplitude={0},PeakCentre=0.0,FWHM={1}".format(amplitude0, fwhm0)
        function += ",constraint=(Amplitude>0.0,FWHM>0.0)"
        function += ";name=Lorentzian,Amplitude={0},PeakCentre=0.0,FWHM={1}".format(amplitude1, fwhm1)
        function += ",constraint=(Amplitude>0.0,FWHM>0.0);ties=(f1.PeakCentre=f0.PeakCentre)"
        return function

    def _force_positive_y_values(self, temporary_name, index):
        from numpy import array

        new_y_array = [-y_value if y_value < 0.0 else y_value for y_value in mtd[temporary_name].readY(index)]
        mtd[temporary_name].setY(0, array(new_y_array))

    def _get_amplitude_and_fwhm_from_fit(self, workspace):
        from IndirectCommon import get_efixed

        parameter_values = self._get_fit_parameters_from_fit(workspace, get_efixed(self._temporary_fit_name))
        return parameter_values[0], parameter_values[2]

    def _get_fit_parameters_from_fit(self, input_name, e_fixed):
        function = "name=Lorentzian, Amplitude=1.0, PeakCentre=0.0, FWHM={0}".format(e_fixed)
        self._fit_workspace(input_name, function, "__peak", True)

        return self._get_fit_parameter_values("__peak_Parameters")

    def _get_fit_parameter_values(self, parameter_table_name):
        from numpy import asarray

        parameter_values = asarray(mtd[parameter_table_name].column("Value"))

        self._delete_workspace(parameter_table_name)
        return parameter_values

    def _fit_workspace(self, input_name, function, output_name, parameters_only):
        fit_alg = self.createChildAlgorithm("Fit", enableLogging=False)
        fit_alg.setAlwaysStoreInADS(True)
        fit_alg.setProperty("Function", function)
        fit_alg.setProperty("InputWorkspace", input_name)
        fit_alg.setProperty("Output", output_name)
        fit_alg.setProperty("OutputParametersOnly", parameters_only)
        fit_alg.execute()

        self._delete_workspace("__peak_NormalisedCovarianceMatrix")

    def _plot_peak_by_log_value(
        self,
        input_string,
        function,
        start_x,
        end_x,
        fit_type,
        minimizer,
        max_iterations,
        output_name,
        create_output=True,
        composite_members=True,
        convolve_members=True,
    ):
        plot_alg = self.createChildAlgorithm("PlotPeakByLogValue", enableLogging=False)
        plot_alg.setProperty("Input", input_string)
        plot_alg.setProperty("Function", function)
        plot_alg.setProperty("StartX", start_x)
        plot_alg.setProperty("EndX", end_x)
        plot_alg.setProperty("FitType", fit_type)
        plot_alg.setProperty("Minimizer", minimizer)
        plot_alg.setProperty("MaxIterations", max_iterations)
        plot_alg.setProperty("CreateOutput", create_output)
        plot_alg.setProperty("OutputCompositeMembers", composite_members)
        plot_alg.setProperty("ConvolveMembers", convolve_members)
        plot_alg.setProperty("OutputWorkspace", output_name)
        plot_alg.execute()
        mtd.addOrReplace(output_name, plot_alg.getProperty("OutputWorkspace").value)

    def _process_indirect_fit_parameters(self, input_name, column_x, x_unit, parameter_names, output_name):
        pifp_alg = self.createChildAlgorithm("ProcessIndirectFitParameters", enableLogging=False)
        pifp_alg.setProperty("InputWorkspace", input_name)
        pifp_alg.setProperty("ColumnX", column_x)
        pifp_alg.setProperty("XAxisUnit", x_unit)
        pifp_alg.setProperty("ParameterNames", parameter_names)
        pifp_alg.setProperty("OutputWorkspace", output_name)
        pifp_alg.execute()
        self._result_ws = pifp_alg.getProperty("OutputWorkspace").value
        mtd.addOrReplace(self._result_name, pifp_alg.getProperty("OutputWorkspace").value)

    def _convert_to_matrix_workspace(self, table_name, column_x, column_y, output_name):
        ctmw_alg = self.createChildAlgorithm("ConvertTableToMatrixWorkspace", enableLogging=False)
        ctmw_alg.setProperty("InputWorkspace", table_name)
        ctmw_alg.setProperty("ColumnX", column_x)
        ctmw_alg.setProperty("ColumnY", column_y)
        ctmw_alg.setProperty("OutputWorkspace", output_name)
        ctmw_alg.execute()
        mtd.addOrReplace(output_name, ctmw_alg.getProperty("OutputWorkspace").value)

    def _convert_to_histogram(self, workspace_name):
        convert_to_hist_alg = self.createChildAlgorithm("ConvertToHistogram", enableLogging=False)
        convert_to_hist_alg.setProperty("InputWorkspace", workspace_name)
        convert_to_hist_alg.setProperty("OutputWorkspace", workspace_name)
        convert_to_hist_alg.execute()
        mtd.addOrReplace(workspace_name, convert_to_hist_alg.getProperty("OutputWorkspace").value)

    def _clone_workspace(self, input_name, output_name):
        clone_alg = self.createChildAlgorithm("CloneWorkspace", enableLogging=False)
        clone_alg.setProperty("InputWorkspace", input_name)
        clone_alg.setProperty("OutputWorkspace", output_name)
        clone_alg.execute()
        mtd.addOrReplace(output_name, clone_alg.getProperty("OutputWorkspace").value)

    def _crop_workspace(self, input_name, output_name, x_min, x_max):
        crop_alg = self.createChildAlgorithm("CropWorkspace", enableLogging=False)
        crop_alg.setProperty("InputWorkspace", input_name)
        crop_alg.setProperty("OutputWorkspace", output_name)
        crop_alg.setProperty("XMin", x_min)
        crop_alg.setProperty("XMax", x_max)
        crop_alg.execute()
        mtd.addOrReplace(output_name, crop_alg.getProperty("OutputWorkspace").value)

    def _delete_workspace(self, input_name):
        delete_alg = self.createChildAlgorithm("DeleteWorkspace", enableLogging=False)
        delete_alg.setProperty("Workspace", input_name)
        delete_alg.execute()

    def _rename_workspace(self, input_name, output_name):
        rename_alg = self.createChildAlgorithm("RenameWorkspace", enableLogging=False)
        rename_alg.setProperty("InputWorkspace", input_name)
        rename_alg.setProperty("OutputWorkspace", output_name)
        rename_alg.execute()
        mtd.addOrReplace(output_name, rename_alg.getProperty("OutputWorkspace").value)

    def _extract_single_spectrum(self, input_name, output_name, index):
        extract_alg = self.createChildAlgorithm("ExtractSingleSpectrum", enableLogging=False)
        extract_alg.setProperty("InputWorkspace", input_name)
        extract_alg.setProperty("WorkspaceIndex", index)
        extract_alg.setProperty("OutputWorkspace", output_name)
        extract_alg.execute()
        mtd.addOrReplace(output_name, extract_alg.getProperty("OutputWorkspace").value)

    def _append_to(self, initial_workspace, to_append):
        append_alg = self.createChildAlgorithm("AppendSpectra", enableLogging=False)
        append_alg.setProperty("InputWorkspace1", initial_workspace)
        append_alg.setProperty("InputWorkspace2", to_append)
        append_alg.setProperty("OutputWorkspace", initial_workspace)
        append_alg.execute()
        mtd.addOrReplace(initial_workspace, append_alg.getProperty("OutputWorkspace").value)

    def _copy_log(self, input_name, output_name):
        copy_log_alg = self.createChildAlgorithm("CopyLogs", enableLogging=False)
        copy_log_alg.setProperty("InputWorkspace", input_name)
        copy_log_alg.setProperty("OutputWorkspace", output_name)
        copy_log_alg.execute()
        mtd.addOrReplace(output_name, copy_log_alg.getProperty("OutputWorkspace").value)

    def _add_sample_log_multiple(self, input_name, log_names, log_values):
        sample_log_mult_alg = self.createChildAlgorithm("AddSampleLogMultiple", enableLogging=False)
        sample_log_mult_alg.setProperty("Workspace", input_name)
        sample_log_mult_alg.setProperty("LogNames", log_names)
        sample_log_mult_alg.setProperty("LogValues", log_values)
        sample_log_mult_alg.execute()


# Register algorithm with Mantid
AlgorithmFactory.subscribe(IndirectTwoPeakFit)
