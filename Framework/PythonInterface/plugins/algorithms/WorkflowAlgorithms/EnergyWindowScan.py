# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import *
from mantid.kernel import *
from mantid import config

import os


def _str_or_none(s):
    if s != "":
        return s
    else:
        return None


def _ws_or_none(s):
    if s != "":
        return mtd[s]
    else:
        return None


def _elems_or_none(l):
    if len(l) != 0:
        return l
    else:
        return None


class EnergyWindowScan(DataProcessorAlgorithm):
    _data_files = None
    _sum_files = None
    _load_logs = None
    _calibration_ws = None
    _instrument_name = None
    _analyser = None
    _reflection = None
    _efixed = None
    _spectra_range = None
    _background_range = None
    _elastic_range = None
    _inelastic_range = None
    _total_range = None
    _rebin_string = None
    _detailed_balance = None
    _grouping_method = None
    _grouping_ws = None
    _grouping_map_file = None
    _output_ws = None
    _output_x_units = None
    _ipf_filename = None
    _sample_log_name = None
    _sample_log_value = None
    _scan_ws = None

    def category(self):
        return "Workflow\\Inelastic;Inelastic\\Indirect;Workflow\\MIDAS"

    def summary(self):
        return "Runs an energy transfer reduction for an inelastic indirect geometry instrument."

    def PyInit(self):
        # Input properties
        self.declareProperty(StringArrayProperty(name="InputFiles"), doc="Comma separated list of input files")

        self.declareProperty(name="LoadLogFiles", defaultValue=True, doc="Load log files when loading runs")

        self.declareProperty(
            WorkspaceProperty("CalibrationWorkspace", "", direction=Direction.Input, optional=PropertyMode.Optional),
            doc="Workspace containing calibration data",
        )

        # Instrument configuration properties
        self.declareProperty(
            name="Instrument", defaultValue="", validator=StringListValidator(["IRIS", "OSIRIS"]), doc="Instrument used during run."
        )
        self.declareProperty(
            name="Analyser",
            defaultValue="",
            validator=StringListValidator(["graphite", "mica", "fmica"]),
            doc="Analyser bank used during run.",
        )
        self.declareProperty(
            name="Reflection",
            defaultValue="",
            validator=StringListValidator(["002", "004", "006"]),
            doc="Reflection number for instrument setup during run.",
        )

        self.declareProperty(
            IntArrayProperty(name="SpectraRange", values=[0, 1], validator=IntArrayMandatoryValidator()),
            doc="Comma separated range of spectra number to use.",
        )

        self.declareProperty(FloatArrayProperty(name="ElasticRange"), doc="Energy range for the elastic component.")
        self.declareProperty(FloatArrayProperty(name="InelasticRange"), doc="Energy range for the inelastic component.")
        self.declareProperty(FloatArrayProperty(name="TotalRange"), doc="Energy range for the total energy component.")

        self.declareProperty(name="DetailedBalance", defaultValue=Property.EMPTY_DBL, doc="Apply the detailed balance correction")

        # Spectra grouping options
        self.declareProperty(
            name="GroupingMethod",
            defaultValue="Individual",
            validator=StringListValidator(["Individual", "All", "File", "Workspace", "IPF"]),
            doc="Method used to group spectra.",
        )
        self.declareProperty(
            WorkspaceProperty("GroupingWorkspace", "", direction=Direction.Input, optional=PropertyMode.Optional),
            doc="Workspace containing spectra grouping.",
        )
        self.declareProperty(
            FileProperty("MapFile", "", action=FileAction.OptionalLoad, extensions=[".map"]), doc="File containing spectra grouping."
        )

        self.declareProperty(name="SampleEnvironmentLogName", defaultValue="sample", doc="Name of the sample environment log entry")

        sampEnvLogVal_type = ["last_value", "average"]
        self.declareProperty(
            "SampleEnvironmentLogValue",
            "last_value",
            StringListValidator(sampEnvLogVal_type),
            doc="Value selection of the sample environment log entry",
        )

        self.declareProperty(name="MSDFit", defaultValue=False, doc="Perform an MSDFit")

        self.declareProperty(name="SumFiles", defaultValue=False, doc="Toggle input file summing or sequential processing")
        # Output properties
        self.declareProperty(name="ReducedWorkspace", defaultValue="Reduced", doc="Workspace group for the resulting workspaces.")
        self.declareProperty(name="ScanWorkspace", defaultValue="Scan", doc="Workspace for the scan results.")

    def PyExec(self):
        self._setup()

        process_prog = Progress(self, start=0.1, end=0.9, nreports=4)
        process_prog.report("Energy Transfer")
        scan_alg = self.createChildAlgorithm("ISISIndirectEnergyTransfer", 0.05, 0.95)
        scan_alg.setProperty("InputFiles", self._data_files)
        scan_alg.setProperty("SumFiles", self._sum_files)
        scan_alg.setProperty("LoadLogFiles", self._load_logs)
        scan_alg.setProperty("CalibrationWorkspace", self._calibration_ws)
        scan_alg.setProperty("Instrument", self._instrument_name)
        scan_alg.setProperty("Analyser", self._analyser)
        scan_alg.setProperty("Reflection", self._reflection)
        scan_alg.setProperty("Efixed", self._efixed)
        scan_alg.setProperty("SpectraRange", self._spectra_range)
        scan_alg.setProperty("BackgroundRange", self._background_range)
        scan_alg.setProperty("RebinString", self._rebin_string)
        scan_alg.setProperty("DetailedBalance", self._detailed_balance)
        scan_alg.setProperty("ScaleFactor", self._scale_factor)
        scan_alg.setProperty("FoldMultipleFrames", self._fold_multiple_frames)
        scan_alg.setProperty("GroupingMethod", self._grouping_method)
        scan_alg.setProperty("GroupingWorkspace", self._grouping_ws)
        scan_alg.setProperty("GroupingFile", self._grouping_map_file)
        scan_alg.setProperty("UnitX", self._output_x_units)
        scan_alg.setProperty("OutputWorkspace", self._output_ws)
        scan_alg.execute()

        logger.information("OutputWorkspace : %s" % self._output_ws)
        logger.information("ScanWorkspace : %s" % self._scan_ws)

        elwin_prog = Progress(self, start=0.9, end=1.0, nreports=4)

        elwin_alg = self.createChildAlgorithm("ElasticWindowMultiple", enableLogging=False)
        elwin_prog.report("Elastic window")
        elwin_alg.setProperty("InputWorkspaces", self._output_ws)
        elwin_alg.setProperty("IntegrationRangeStart", self._elastic_range[0])
        elwin_alg.setProperty("IntegrationRangeEnd", self._elastic_range[1])
        elwin_alg.setProperty("SampleEnvironmentLogName", self._sample_log_name)
        elwin_alg.setProperty("SampleEnvironmentLogValue", self._sample_log_value)
        elwin_alg.setAlwaysStoreInADS(True)
        elwin_alg.setProperty("OutputInQ", self._scan_ws + "_el_eq1")
        elwin_alg.setProperty("OutputInQSquared", self._scan_ws + "_el_eq2")
        elwin_alg.setProperty("OutputELF", self._scan_ws + "_el_elf")
        elwin_alg.setProperty("OutputELT", self._scan_ws + "_el_elt")
        elwin_alg.execute()

        elwin_prog.report("Inelastic window")
        elwin_alg.setProperty("InputWorkspaces", self._output_ws)
        elwin_alg.setProperty("IntegrationRangeStart", self._inelastic_range[0])
        elwin_alg.setProperty("IntegrationRangeEnd", self._inelastic_range[1])
        elwin_alg.setProperty("SampleEnvironmentLogName", self._sample_log_name)
        elwin_alg.setProperty("SampleEnvironmentLogValue", self._sample_log_value)
        elwin_alg.setProperty("OutputInQ", self._scan_ws + "_inel_eq1")
        elwin_alg.setProperty("OutputInQSquared", self._scan_ws + "_inel_eq2")
        elwin_alg.setProperty("OutputELF", self._scan_ws + "_inel_elf")
        elwin_alg.setProperty("OutputELT", self._scan_ws + "_inel_elt")
        elwin_alg.execute()

        elwin_prog.report("Total window")
        elwin_alg.setProperty("InputWorkspaces", self._output_ws)
        elwin_alg.setProperty("IntegrationRangeStart", self._total_range[0])
        elwin_alg.setProperty("IntegrationRangeEnd", self._total_range[1])
        elwin_alg.setProperty("SampleEnvironmentLogName", self._sample_log_name)
        elwin_alg.setProperty("SampleEnvironmentLogValue", self._sample_log_value)
        elwin_alg.setProperty("OutputInQ", self._scan_ws + "_total_eq1")
        elwin_alg.setProperty("OutputInQSquared", self._scan_ws + "_total_eq2")
        elwin_alg.setProperty("OutputELF", self._scan_ws + "_total_elf")
        elwin_alg.setProperty("OutputELT", self._scan_ws + "_total_elt")
        elwin_alg.execute()

        # storing in ADS so eisf workspace is created
        divide_alg = self.createChildAlgorithm("Divide", enableLogging=False)
        divide_alg.setAlwaysStoreInADS(True)
        divide_alg.setProperty("LHSWorkspace", self._scan_ws + "_el_eq1")
        divide_alg.setProperty("RHSWorkspace", self._scan_ws + "_total_eq1")
        divide_alg.setProperty("OutputWorkspace", self._scan_ws + "_eisf")
        divide_alg.execute()

        x_values = mtd[self._scan_ws + "_el_eq1"].readX(0)
        num_hist = mtd[self._scan_ws + "_el_eq1"].getNumberHistograms()
        if len(x_values) < 2 and self._msdfit:
            logger.warning("Unable to perform MSDFit")
            self._msdfit = False

        if self._msdfit:
            msdfit_prog = Progress(self, start=0.9, end=1.0, nreports=4)
            msdfit_prog.report("msdFit")
            msd_alg = self.createChildAlgorithm("MSDFit", enableLogging=True)
            msd_alg.setProperty("InputWorkspace", self._scan_ws + "_el_eq1")
            msd_alg.setProperty("Xstart", x_values[0])
            msd_alg.setProperty("XEnd", x_values[len(x_values) - 1])
            msd_alg.setProperty("SpecMin", 0)
            msd_alg.setProperty("SpecMax", num_hist - 1)
            msd_alg.setProperty("OutputWorkspace", self._scan_ws + "_msd")
            msd_alg.setProperty("FitWorkspaces", self._scan_ws + "_msd_fit")
            msd_alg.execute()

    def validateInputs(self):
        """
        Validates algorithm properties.
        """
        issues = dict()

        # Validate the instrument configuration by checking if a parameter file exists
        instrument_name = self.getPropertyValue("Instrument")
        analyser = self.getPropertyValue("Analyser")
        reflection = self.getPropertyValue("Reflection")

        ipf_filename = os.path.join(
            config["instrumentDefinition.directory"], instrument_name + "_" + analyser + "_" + reflection + "_Parameters.xml"
        )

        if not os.path.exists(ipf_filename):
            error_message = "Invalid instrument configuration"
            issues["Instrument"] = error_message
            issues["Analyser"] = error_message
            issues["Reflection"] = error_message

        # Validate spectra range

        spectra_range = self.getProperty("SpectraRange").value
        if len(spectra_range) != 2:
            issues["SpectraRange"] = "Range must contain exactly two items"
        elif spectra_range[0] > spectra_range[1]:
            issues["SpectraRange"] = "Range must be in format: lower,upper"

        # Validate ranges
        elastic_range = self.getProperty("ElasticRange").value
        if elastic_range is not None:
            if len(elastic_range) != 2:
                issues["ElasticRange"] = "Range must contain exactly two items"
            elif elastic_range[0] > elastic_range[1]:
                issues["ElasticRange"] = "Range must be in format: lower,upper"

        inelastic_range = self.getProperty("InelasticRange").value
        if inelastic_range is not None:
            if len(inelastic_range) != 2:
                issues["InelasticRange"] = "Range must contain exactly two items"
            elif inelastic_range[0] > inelastic_range[1]:
                issues["InelasticRange"] = "Range must be in format: lower,upper"

        total_range = self.getProperty("TotalRange").value
        if inelastic_range is not None:
            if len(total_range) != 2:
                issues["TotalRange"] = "Range must contain exactly two items"
            elif total_range[0] > total_range[1]:
                issues["TotalRange"] = "Range must be in format: lower,upper"

        # Validate grouping method
        grouping_method = self.getPropertyValue("GroupingMethod")
        grouping_ws = _ws_or_none(self.getPropertyValue("GroupingWorkspace"))

        if grouping_method == "Workspace" and grouping_ws is None:
            issues["GroupingWorkspace"] = "Must select a grouping workspace for current GroupingWorkspace"

        return issues

    def _setup(self):
        """
        Gets algorithm properties.
        """

        # Get properties
        self._data_files = self.getProperty("InputFiles").value
        self._sum_files = self.getProperty("SumFiles").value
        self._load_logs = self.getProperty("LoadLogFiles").value
        self._calibration_ws = ""

        self._instrument_name = self.getPropertyValue("Instrument")
        self._analyser = self.getPropertyValue("Analyser")
        self._reflection = self.getPropertyValue("Reflection")
        self._efixed = Property.EMPTY_DBL

        self._spectra_range = self.getProperty("SpectraRange").value
        self._background_range = ""
        self._rebin_string = ""
        self._detailed_balance = self.getProperty("DetailedBalance").value
        self._scale_factor = 1.0
        self._fold_multiple_frames = False
        self._elastic_range = self.getProperty("ElasticRange").value
        self._inelastic_range = self.getProperty("InelasticRange").value
        self._total_range = self.getProperty("TotalRange").value

        self._grouping_method = self.getPropertyValue("GroupingMethod")
        self._grouping_ws = ""
        self._grouping_map_file = ""

        self._output_x_units = "DeltaE"

        self._msdfit = self.getProperty("MSDFit").value

        self._output_ws = self.getPropertyValue("ReducedWorkspace")
        self._sample_log_name = self.getPropertyValue("SampleEnvironmentLogName")
        self._sample_log_value = self.getPropertyValue("SampleEnvironmentLogValue")

        self._scan_ws = self.getPropertyValue("ScanWorkspace")

        # Disable sum files if there is only one file
        if (len(self._data_files) == 1) & self._sum_files:
            logger.warning("SumFiles disabled when only one input file is provided.")
            self._sum_files = False

        # Get the IPF filename
        self._ipf_filename = os.path.join(
            config["instrumentDefinition.directory"],
            self._instrument_name + "_" + self._analyser + "_" + self._reflection + "_Parameters.xml",
        )
        logger.information("Instrument parameter file: %s" % self._ipf_filename)

        # Warn when grouping options are to be ignored
        if self._grouping_method != "Workspace" and self._grouping_ws is not None:
            logger.warning("GroupingWorkspace will be ignored by selected GroupingMethod")

        if self._grouping_method != "File" and self._grouping_map_file is not None:
            logger.warning("MapFile will be ignored by selected GroupingMethod")


# Register algorithm with Mantid
AlgorithmFactory.subscribe(EnergyWindowScan)
