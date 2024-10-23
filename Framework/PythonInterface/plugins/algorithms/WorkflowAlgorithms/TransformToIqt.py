# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,too-many-instance-attributes
from mantid.api import mtd, AlgorithmFactory, ITableWorkspaceProperty, MatrixWorkspaceProperty, PropertyMode, Progress, PythonAlgorithm
from mantid.kernel import Direction, logger, IntBoundedValidator
from mantid.simpleapi import CreateEmptyTableWorkspace, CropWorkspace

from IndirectCommon import check_analysers_or_e_fixed, check_dimensions_equal, check_hist_zero, get_efixed, get_workspace_name_prefix

DEFAULT_ITERATIONS = 50
DEFAULT_SEED = 89631139


class TransformToIqt(PythonAlgorithm):
    _sample = None
    _resolution = None
    _e_min = None
    _e_max = None
    _e_width = None
    _number_points_per_bin = None
    _parameter_table = None
    _output_workspace = None
    _dry_run = None
    _calculate_errors = None
    _enforce_normalization = None
    _number_of_iterations = None
    _seed = None

    def category(self):
        return "Workflow\\Inelastic;Workflow\\MIDAS"

    def summary(self):
        return "Transforms an inelastic reduction to I(Q, t)"

    def PyInit(self):
        self.declareProperty(
            MatrixWorkspaceProperty("SampleWorkspace", "", optional=PropertyMode.Mandatory, direction=Direction.Input),
            doc="Name for the sample workspace.",
        )

        self.declareProperty(
            MatrixWorkspaceProperty("ResolutionWorkspace", "", optional=PropertyMode.Mandatory, direction=Direction.Input),
            doc="Name for the resolution workspace.",
        )

        self.declareProperty(name="EnergyMin", defaultValue=-0.5, doc="Minimum energy for fit.")
        self.declareProperty(name="EnergyMax", defaultValue=0.5, doc="Maximum energy for fit.")
        self.declareProperty(
            name="BinReductionFactor",
            defaultValue=10.0,
            doc="Decrease total number of spectrum points by this ratio through merging of " "intensities from neighbouring bins.",
        )

        self.declareProperty(
            "NumberOfIterations",
            DEFAULT_ITERATIONS,
            IntBoundedValidator(lower=1),
            doc="Number of randomised simulations for monte-carlo error calculation.",
        )

        self.declareProperty(
            "SeedValue",
            DEFAULT_SEED,
            IntBoundedValidator(lower=1),
            doc="Seed for pseudo-random number generator in monte-carlo error calculation.",
        )

        self.declareProperty(
            ITableWorkspaceProperty("ParameterWorkspace", "", direction=Direction.Output, optional=PropertyMode.Optional),
            doc="Table workspace for saving TransformToIqt properties",
        )

        self.declareProperty(
            MatrixWorkspaceProperty("OutputWorkspace", "", direction=Direction.Output, optional=PropertyMode.Optional),
            doc="Output workspace",
        )

        self.declareProperty(name="DryRun", defaultValue=False, doc="Only calculate and output the parameters")
        self.declareProperty("CalculateErrors", defaultValue=True, doc="Calculate monte-carlo errors.")
        self.declareProperty("EnforceNormalization", defaultValue=True, doc="Normalization to enforce I(t=0)")

    def PyExec(self):
        self._setup()

        self._check_analysers_and_reflection()

        self._calculate_parameters()

        if not self._dry_run:
            self._output_workspace = self._transform()

            self._add_logs()

        else:
            skip_prog = Progress(self, start=0.3, end=1.0, nreports=2)
            skip_prog.report("skipping transform")
            skip_prog.report("skipping add logs")
            logger.information("Dry run, will not run TransformToIqt")

        self.setProperty("ParameterWorkspace", self._parameter_table)
        self.setProperty("OutputWorkspace", self._output_workspace)

    def _setup(self):
        """
        Gets algorithm properties.
        """
        self._sample = self.getPropertyValue("SampleWorkspace")
        self._resolution = self.getPropertyValue("ResolutionWorkspace")

        self._e_min = self.getProperty("EnergyMin").value
        self._e_max = self.getProperty("EnergyMax").value
        self._number_points_per_bin = self.getProperty("BinReductionFactor").value

        self._parameter_table = self.getPropertyValue("ParameterWorkspace")
        if self._parameter_table == "":
            self._parameter_table = get_workspace_name_prefix(self._sample) + "TransformToIqtParameters"

        self._calculate_errors = self.getProperty("CalculateErrors").value
        self._enforce_normalization = self.getProperty("EnforceNormalization").value
        self._number_of_iterations = self.getProperty("NumberOfIterations").value
        self._seed = self.getProperty("SeedValue").value

        self._output_workspace = self.getPropertyValue("OutputWorkspace")
        if self._output_workspace == "":
            self._output_workspace = get_workspace_name_prefix(self._sample) + "iqt"

        self._dry_run = self.getProperty("DryRun").value

    def validateInputs(self):
        """
        Validate input properties.
        """
        issues = dict()

        e_min = self.getProperty("EnergyMin").value
        e_max = self.getProperty("EnergyMax").value

        # Check for swapped energy values
        if e_min > e_max:
            energy_swapped = "EnergyMin is greater than EnergyMax"
            issues["EnergyMin"] = energy_swapped
            issues["EnergyMax"] = energy_swapped

        return issues

    def _calculate_parameters(self):
        """
        Calculates the TransformToIqt parameters and saves in a table workspace.
        """
        end_prog = 0.3 if self._calculate_errors else 0.9
        workflow_prog = Progress(self, start=0.0, end=end_prog, nreports=8)
        workflow_prog.report("Cropping Workspace")
        cropped_workspace = CropWorkspace(InputWorkspace=self._sample, Xmin=self._e_min, Xmax=self._e_max, StoreInADS=False)
        workflow_prog.report("Calculating table properties")
        number_input_points = cropped_workspace.blocksize()
        num_bins = int(number_input_points / self._number_points_per_bin)
        self._e_width = (abs(self._e_min) + abs(self._e_max)) / num_bins

        workflow_prog.report("Attempting to Access IPF")
        try:
            workflow_prog.report("Access IPF")
            instrument = mtd[self._sample].getInstrument()

            analyserName = instrument.getStringParameter("analyser")[0]
            analyser = instrument.getComponentByName(analyserName)

            if analyser is not None:
                logger.debug("Found %s component in instrument %s, will look for resolution there" % (analyserName, instrument))
                resolution = analyser.getNumberParameter("resolution")[0]
            else:
                logger.debug(
                    "No %s component found on instrument %s, will look for resolution in top level instrument" % (analyserName, instrument)
                )
                resolution = instrument.getNumberParameter("resolution")[0]

            logger.information("Got resolution from IPF: %f" % resolution)
            workflow_prog.report("IPF resolution obtained")
        except (AttributeError, IndexError):
            workflow_prog.report("Resorting to Default")
            resolution = get_efixed(self._sample) * 0.01
            logger.warning(
                "Could not get the resolution from the IPF, using 1% of the E Fixed value for the " "resolution: {0}".format(resolution)
            )

        resolution_bins = int(round((2 * resolution) / self._e_width))

        if resolution_bins < 5:
            logger.warning("Resolution curve has <5 points. Results may be unreliable.")

        workflow_prog.report("Creating Parameter table")
        param_table = CreateEmptyTableWorkspace(OutputWorkspace=self._parameter_table)

        workflow_prog.report("Populating Parameter table")
        param_table.addColumn("int", "SampleInputBins")
        param_table.addColumn("float", "BinReductionFactor")
        param_table.addColumn("int", "SampleOutputBins")
        param_table.addColumn("float", "EnergyMin")
        param_table.addColumn("float", "EnergyMax")
        param_table.addColumn("float", "EnergyWidth")
        param_table.addColumn("float", "Resolution")
        param_table.addColumn("int", "ResolutionBins")

        param_table.addRow(
            [
                number_input_points,
                self._number_points_per_bin,
                num_bins,
                self._e_min,
                self._e_max,
                self._e_width,
                resolution,
                resolution_bins,
            ]
        )

        self.setProperty("ParameterWorkspace", param_table)

    def _add_logs(self):
        sample_logs = [
            ("iqt_sample_workspace", self._sample),
            ("iqt_resolution_workspace", self._resolution),
            ("iqt_binning", "%f,%f,%f" % (self._e_min, self._e_width, self._e_max)),
        ]

        log_alg = self.createChildAlgorithm(name="AddSampleLogMultiple", startProgress=0.9, endProgress=1.0, enableLogging=True)
        log_alg.setProperty("Workspace", self._output_workspace)
        log_alg.setProperty("LogNames", [item[0] for item in sample_logs])
        log_alg.setProperty("LogValues", [item[1] for item in sample_logs])
        log_alg.execute()

    def _transform(self):
        """
        Run TransformToIqt.
        """
        # Process resolution data
        res_number_of_histograms = check_hist_zero(self._resolution)[0]
        sample_number_of_histograms = check_hist_zero(self._sample)[0]
        if res_number_of_histograms > 1 and sample_number_of_histograms is not res_number_of_histograms:
            check_dimensions_equal(self._sample, "Sample", self._resolution, "Resolution")

        calculateiqt_alg = self.createChildAlgorithm(name="CalculateIqt", startProgress=0.3, endProgress=1.0, enableLogging=True)
        calculateiqt_alg.setAlwaysStoreInADS(False)
        args = {
            "InputWorkspace": self._sample,
            "OutputWorkspace": "iqt",
            "ResolutionWorkspace": self._resolution,
            "EnergyMin": self._e_min,
            "EnergyMax": self._e_max,
            "EnergyWidth": self._e_width,
            "CalculateErrors": self._calculate_errors,
            "EnforceNormalization": self._enforce_normalization,
            "NumberOfIterations": self._number_of_iterations,
            "SeedValue": self._seed,
        }
        for key, value in args.items():
            calculateiqt_alg.setProperty(key, value)
        calculateiqt_alg.execute()

        iqt = calculateiqt_alg.getProperty("OutputWorkspace").value

        # Set Y axis unit and label
        iqt.setYUnit("")
        iqt.setYUnitLabel("Intensity")
        return iqt

    def _check_analysers_and_reflection(self):
        try:
            check_analysers_or_e_fixed(self._sample, self._resolution)
        except ValueError:
            # A genuine error the shows that the two runs are incompatible
            raise
        except BaseException:
            # Checking could not be performed due to incomplete or no
            # instrument
            logger.warning("Could not check for matching analyser and reflection")


# Register algorithm with Mantid
AlgorithmFactory.subscribe(TransformToIqt)
