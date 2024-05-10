# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import *
from mantid.api import *
from mantid.kernel import *
from mantid import config

import os
import warnings


class VesuvioDiffractionReduction(DataProcessorAlgorithm):
    _workspace_names = None
    _chopped_data = None
    _output_ws = None
    _data_files = None
    _instrument_name = None
    _mode = None
    _par_filename = None
    _spectra_range = None
    _grouping_method = None
    _rebin_string = None
    _ipf_filename = None
    _sum_files = None

    def category(self):
        return "Diffraction\\Reduction"

    def seeAlso(self):
        return ["ISISIndirectDiffractionReduction"]

    def summary(self):
        return "Performs diffraction reduction for VESUVIO. This algorithm is deprecated (April-2017)."

    def PyInit(self):
        self.declareProperty(StringArrayProperty("InputFiles"), doc="Comma separated list of input files.")

        self.declareProperty(
            FileProperty("InstrumentParFile", "", action=FileAction.Load, extensions=[".dat", ".par"]),
            doc="PAR file containing instrument definition.",
        )

        self.declareProperty(name="SumFiles", defaultValue=False, doc="Enabled to sum spectra from each input file.")

        self.declareProperty(IntArrayProperty("SpectraRange", [3, 198]), doc="Range of spectra to use.")

        self.declareProperty(name="RebinParam", defaultValue="", doc="Rebin parameters.")

        self.declareProperty(
            name="GroupingPolicy",
            defaultValue="All",
            validator=StringListValidator(["All", "Individual", "IPF"]),
            doc="Selects the type of detector grouping to be used.",
        )

        self.declareProperty(
            WorkspaceGroupProperty("OutputWorkspace", "", direction=Direction.Output), doc="Group name for the result workspaces."
        )

    def validateInputs(self):
        """
        Checks for issues with user input.
        """
        issues = dict()

        # Validate input files
        input_files = self.getProperty("InputFiles").value
        if len(input_files) == 0:
            issues["InputFiles"] = "InputFiles must contain at least one filename"

        # Validate detector range
        detector_range = self.getProperty("SpectraRange").value
        if len(detector_range) != 2:
            issues["SpectraRange"] = "SpectraRange must be an array of 2 values only"
        else:
            if detector_range[0] > detector_range[1]:
                issues["SpectraRange"] = "SpectraRange must be in format [lower_index,upper_index]"

        return issues

    def PyExec(self):

        warnings.warn("This algorithm is depreciated (April-2017). Please use ISISIndirectDiffractionReduction")

        from IndirectReductionCommon import (
            load_files,
            get_multi_frame_rebin,
            identify_bad_detectors,
            unwrap_monitor,
            process_monitor_efficiency,
            scale_monitor,
            scale_detectors,
            rebin_reduction,
            group_spectra,
            fold_chopped,
            rename_reduction,
            mask_detectors,
        )

        self._setup()

        load_opts = dict()
        load_opts["Mode"] = "FoilOut"
        load_opts["InstrumentParFile"] = self._par_filename
        # Tell LoadVesuvio to load the monitors and keep them in the output
        load_opts["LoadMonitors"] = True

        prog_reporter = Progress(self, start=0.0, end=1.0, nreports=1)

        prog_reporter.report("Loading Files")
        self._workspace_names, self._chopped_data, _ = load_files(
            self._data_files,
            ipf_filename=self._ipf_filename,
            spec_min=self._spectra_range[0],
            spec_max=self._spectra_range[1],
            sum_files=self._sum_files,
            load_opts=load_opts,
        )

        prog_reporter.resetNumSteps(len(self._workspace_names), 0.0, 1.0)

        for c_ws_name in self._workspace_names:
            is_multi_frame = isinstance(mtd[c_ws_name], WorkspaceGroup)

            # Get list of workspaces
            if is_multi_frame:
                workspaces = mtd[c_ws_name].getNames()
            else:
                workspaces = [c_ws_name]

            # Process rebinning for framed data
            rebin_string_2, num_bins = get_multi_frame_rebin(c_ws_name, self._rebin_string)

            masked_detectors = identify_bad_detectors(workspaces[0])

            # Process workspaces
            for ws_name in workspaces:
                monitor_ws_name = ws_name + "_mon"

                # Process monitor
                if not unwrap_monitor(ws_name):
                    ConvertUnits(InputWorkspace=monitor_ws_name, OutputWorkspace=monitor_ws_name, Target="Wavelength", EMode="Elastic")

                process_monitor_efficiency(ws_name)
                scale_monitor(ws_name)

                # Scale detector data by monitor intensities
                scale_detectors(ws_name, "Elastic")

                # Remove the no longer needed monitor workspace
                DeleteWorkspace(monitor_ws_name)

                # Convert to dSpacing
                ConvertUnits(InputWorkspace=ws_name, OutputWorkspace=ws_name, Target="dSpacing", EMode="Elastic")

                # Mask noisy detectors
                if len(masked_detectors) > 0:
                    mask_detectors(ws_name, masked_detectors)

                # Handle rebinning
                rebin_reduction(ws_name, self._rebin_string, rebin_string_2, num_bins)

                # Group spectra
                grouped = group_spectra(ws_name, self._grouping_method)
                AnalysisDataService.addOrReplace(ws_name, grouped)

            if is_multi_frame:
                fold_chopped(c_ws_name)

            prog_reporter.report()

        # Rename output workspaces
        output_workspace_names = [rename_reduction(ws_name, self._sum_files) for ws_name in self._workspace_names]

        # Group result workspaces
        GroupWorkspaces(InputWorkspaces=output_workspace_names, OutputWorkspace=self._output_ws)

        self.setProperty("OutputWorkspace", self._output_ws)

    def _setup(self):
        """
        Gets algorithm properties.
        """
        self._instrument_name = "VESUVIO"
        self._mode = "diffspec"

        self._output_ws = self.getPropertyValue("OutputWorkspace")
        self._par_filename = self.getPropertyValue("InstrumentParFile")
        self._spectra_range = self.getProperty("SpectraRange").value
        self._rebin_string = self.getPropertyValue("RebinParam")
        self._grouping_method = self.getPropertyValue("GroupingPolicy")

        if self._rebin_string == "":
            self._rebin_string = None

        # Get the IPF filename
        self._ipf_filename = self._instrument_name + "_diffraction_" + self._mode + "_Parameters.xml"
        if not os.path.exists(self._ipf_filename):
            self._ipf_filename = os.path.join(config["instrumentDefinition.directory"], self._ipf_filename)
        logger.information("IPF filename is: %s" % self._ipf_filename)

        # Split up runs given as a range LoadVesuvio sums multiple runs on its own
        self._sum_files = self.getProperty("SumFiles").value
        user_input = self.getProperty("InputFiles").value
        single_files = []
        for run in user_input:
            try:
                number_generator = IntArrayProperty("array_generator", run)
                single_files.extend(number_generator.value.tolist())
            except RuntimeError as exc:
                raise RuntimeError("Could not generate run numbers from '{0}': '{1}'".format(run, str(exc)))
        # end
        self._data_files = single_files
        if self._sum_files and len(self._data_files) == 1:
            logger.warning("Ignoring SumFiles=True as only one file has been provided")


AlgorithmFactory.subscribe(VesuvioDiffractionReduction)
