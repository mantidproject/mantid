# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
from mantid.kernel import *
from mantid.api import *

from vesuvio.base import VesuvioBase, TableWorkspaceDictionaryFacade
from vesuvio.fitting import parse_fit_options
from vesuvio.instrument import VESUVIO

# Loading difference modes
_DIFF_MODES = ("double", "single")
# Fitting modes
_FIT_MODES = ("bank", "spectrum")


class VesuvioTOFFit(VesuvioBase):
    def summary(self):
        return "Processes runs for Vesuvio at ISIS"

    def category(self):
        return "Inelastic\\Indirect\\Vesuvio"

    def PyInit(self):
        # Inputs
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "", Direction.Input), doc="Input TOF workspace")

        float_length_validator = FloatArrayLengthValidator()
        float_length_validator.setLengthMin(1)
        self.declareProperty(FloatArrayProperty("Masses", float_length_validator), doc="Mass values for fitting")

        self.declareProperty(
            "MassProfiles",
            "",
            StringMandatoryValidator(),
            doc="Functions used to approximate mass profile. "
            "The format is function=Function1Name,param1=val1,"
            "param2=val2;function=Function2Name,param3=val3,param4=val4",
        )

        # ----- Optional ------
        self.declareProperty("WorkspaceIndex", 0, IntBoundedValidator(lower=0), doc="Workspace index for fit. [Default=0]")
        self.declareProperty(
            "Background", "", doc="Function used to fit the background. " "The format is function=FunctionName,param1=val1,param2=val2"
        )
        self.declareProperty(
            "IntensityConstraints",
            "",
            doc="A semi-colon separated list of intensity constraints defined " "as lists e.g [0,1,0,-4];[1,0,-2,0]",
        )

        self.declareProperty("Ties", "", doc="A string representing the ties to be applied to the fit")

        self.declareProperty(
            "FitMode", "bank", StringListValidator(list(_FIT_MODES)), doc="Fit either bank-by-bank or detector-by-detector"
        )

        self.declareProperty("MaxIterations", 5000, IntBoundedValidator(lower=0), doc="Maximum number of fitting iterations")
        self.declareProperty("Minimizer", "Levenberg-Marquardt,AbsError=1e-08,RelError=1e-08", doc="String defining the minimizer")

        # Outputs
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", Direction.Output), doc="The name of the fitted workspaces.")
        self.declareProperty(
            ITableWorkspaceProperty("FitParameters", "", Direction.Output), doc="The name of the fitted parameter workspaces."
        )
        self.declareProperty("Chi2", 0.0, direction=Direction.Output, doc="The value of the Chi-Squared.")

    def PyExec(self):
        """PyExec is defined by base. It sets the instrument and calls this method"""
        self._INST = VESUVIO()
        tof_data = self.getProperty("InputWorkspace").value

        reduced_chi_square, fit_params, fit_data = self._fit_tof(tof_data)
        self.setProperty("OutputWorkspace", fit_data)
        self.setProperty("FitParameters", fit_params)
        self.setProperty("Chi2", reduced_chi_square)

    def _fit_tof(self, tof_data):
        """
        Runs a fit against the loaded data
        """
        fit_opts = parse_fit_options(
            mass_values=self.getProperty("Masses").value,
            profile_strs=self.getProperty("MassProfiles").value,
            background_str=self.getProperty("Background").value,
            constraints_str=self.getProperty("IntensityConstraints").value,
        )

        return self._run_fit(tof_data, self.getProperty("WorkspaceIndex").value, fit_opts)

    # -----------------------------------------------------------------------------------------
    def _run_fit(self, tof_ws, workspace_index, fit_options):
        """
        Run the Fit algorithm with the given options on the input data
        @param data_ws :: The workspace containing the data to fit too
        @param fit_options :: An object of type FitOptions containing
                              the parameters
        """
        if fit_options.global_fit:
            raise RuntimeError("Global fit not implemented yet")
        else:
            return self._run_fit_impl(tof_ws, workspace_index, fit_options, simulation=False)

    def _run_fit_impl(self, data_ws, workspace_index, fit_options, simulation=False):
        """
        Run the Fit algorithm with the given options on the input data
        @param data_ws :: The workspace containing the data to fit too
        @param workspace_index :: The spectra to fit against
        @param fit_options :: An object of type FittingOptions containing
                              the parameters
        @param simulation :: If true then only a single iteration is run
        """
        if simulation:
            raise RuntimeError("Simulation not implemented yet")
        else:
            # Run fitting first time using constraints matrix to reduce active parameter set
            function_str = fit_options.create_function_str()
            constraints = fit_options.create_constraints_str()
            ties = fit_options.create_ties_str()
            user_ties = self.getProperty("Ties").value
            if user_ties != "":
                ties = ties + "," + user_ties

            _, params, fitted_data = self._do_fit(
                function_str, data_ws, workspace_index, constraints, ties, max_iter=self.getProperty("MaxIterations").value
            )

            # Run second time using standard CompositeFunction & no constraints matrix to
            # calculate correct reduced chisq
            param_values = TableWorkspaceDictionaryFacade(params)

        function_str = fit_options.create_function_str(param_values)
        max_iter = 0 if simulation else 1
        reduced_chi_square, _, _ = self._do_fit(function_str, data_ws, workspace_index, constraints, ties, max_iter=max_iter)

        return reduced_chi_square, params, fitted_data

    # ----------------------------------------------------------------------------------------

    # pylint: disable=too-many-arguments
    def _do_fit(self, function_str, data_ws, index, constraints, ties, max_iter):
        # The tof data is required to be in seconds for the fitting
        # in order to re-use the standard Mantid Polynomial function. This polynomial simply
        # accepts the data "as is" in the workspace so if it is in microseconds then the
        # we would have to either implement a another wrapper to translate or write another
        # Polynomial.
        # The simplest option is to put the data in seconds here and then put it back afterward
        data_ws = self._execute_child_alg("ScaleX", InputWorkspace=data_ws, OutputWorkspace=data_ws, Operation="Multiply", Factor=1e-06)

        outputs = self._execute_child_alg(
            "Fit",
            Function=function_str,
            InputWorkspace=data_ws,
            WorkspaceIndex=index,
            Ties=ties,
            Constraints=constraints,
            CreateOutput=True,
            OutputCompositeMembers=True,
            MaxIterations=max_iter,
            Minimizer=self.getPropertyValue("Minimizer"),
        )

        result, reduced_chi_squared = outputs[0], outputs[1]
        params, fitted_data = outputs[3], outputs[4]

        # Output result of fiting to log
        result_log_str = "Fit result: {0}".format(result)
        if result == "success":
            logger.information(result_log_str)
        else:
            logger.warning(result_log_str)

        fitted_data = self._execute_child_alg(
            "ScaleX", InputWorkspace=fitted_data, OutputWorkspace=fitted_data, Operation="Multiply", Factor=1e06
        )
        data_ws = self._execute_child_alg("ScaleX", InputWorkspace=data_ws, OutputWorkspace=data_ws, Operation="Multiply", Factor=1e06)

        return reduced_chi_squared, params, fitted_data


# -----------------------------------------------------------------------------------------
AlgorithmFactory.subscribe(VesuvioTOFFit)
