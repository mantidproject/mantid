# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import numpy
from mantid import mtd
from mantid.kernel import StringListValidator, Direction, FloatBoundedValidator, FloatArrayMandatoryValidator, IntBoundedValidator
from mantid.api import AlgorithmFactory, PythonAlgorithm, MultipleFileProperty, FileProperty, FileAction, WorkspaceGroupProperty, Progress
from mantid.simpleapi import (
    CloneWorkspace,
    DeleteWorkspace,
    Divide,
    GroupWorkspaces,
    IndirectILLEnergyTransfer,
    Integration,
    MaskBins,
    MatchPeaks,
    Minus,
    Plus,
    RenameWorkspace,
    Scale,
    SelectNexusFilesByMetadata,
)


class IndirectILLReductionQENS(PythonAlgorithm):
    _sample_files = None
    _alignment_files = None
    _background_files = None
    _calibration_files = None
    _background_calib_files = None
    _sum_all_runs = None
    _unmirror_option = None
    _back_scaling = None
    _back_calib_scaling = None
    _criteria = None
    _progress = None
    _red_ws = None
    _common_args = {}
    _peak_range = []
    _runs = None
    _spectrum_axis = None
    _discard_sds = None
    _group_detectors = None

    def category(self):
        return "Workflow\\MIDAS;Workflow\\Inelastic;Inelastic\\Indirect;Inelastic\\Reduction;ILL\\Indirect"

    def summary(self):
        return "Performs quasi-elastic neutron scattering (QENS) multiple file reduction for ILL indirect geometry data, instrument IN16B."

    def seeAlso(self):
        return ["IndirectILLReductionFWS", "IndirectILLEnergyTransfer"]

    def name(self):
        return "IndirectILLReductionQENS"

    def PyInit(self):
        self.declareProperty(MultipleFileProperty("Run", extensions=["nxs"]), doc="Run number(s) of sample run(s).")

        self.declareProperty(
            MultipleFileProperty("BackgroundRun", action=FileAction.OptionalLoad, extensions=["nxs"]),
            doc="Run number(s) of background (empty can) run(s).",
        )

        self.declareProperty(
            MultipleFileProperty("CalibrationRun", action=FileAction.OptionalLoad, extensions=["nxs"]),
            doc="Run number(s) of vanadium calibration run(s).",
        )

        self.declareProperty(
            MultipleFileProperty("CalibrationBackgroundRun", action=FileAction.OptionalLoad, extensions=["nxs"]),
            doc="Run number(s) of background (empty can) run(s) for vanadium run.",
        )

        self.declareProperty(
            MultipleFileProperty("AlignmentRun", action=FileAction.OptionalLoad, extensions=["nxs"]),
            doc="Run number(s) of vanadium run(s) used for peak alignment for UnmirrorOption=[5, 7]",
        )

        self.declareProperty(name="SumRuns", defaultValue=False, doc="Whether to sum all the input runs.")

        self.declareProperty(
            name="CropDeadMonitorChannels",
            defaultValue=False,
            doc="Whether or not to exclude the first and last few channels with 0 monitor count in the energy transfer formula.",
        )

        self.declareProperty(
            name="UnmirrorOption",
            defaultValue=6,
            validator=IntBoundedValidator(lower=0, upper=7),
            doc="Unmirroring options : \n"
            "0 no unmirroring\n"
            "1 sum of left and right\n"
            "2 left\n"
            "3 right\n"
            "4 shift right according to left and sum\n"
            "5 like 4, but use alignment run for peak positions\n"
            "6 center both left and right at zero and sum\n"
            "7 like 6, but use alignment run for peak positions",
        )

        self.declareProperty(
            name="BackgroundScalingFactor",
            defaultValue=1.0,
            validator=FloatBoundedValidator(lower=0),
            doc="Scaling factor for background subtraction",
        )

        self.declareProperty(
            name="CalibrationBackgroundScalingFactor",
            defaultValue=1.0,
            validator=FloatBoundedValidator(lower=0),
            doc="Scaling factor for background subtraction for vanadium calibration",
        )

        self.declareProperty(
            name="CalibrationPeakRange",
            defaultValue=[-0.003, 0.003],
            validator=FloatArrayMandatoryValidator(),
            doc="Peak range for integration over calibration file peak (in mev)",
        )

        self.declareProperty(
            FileProperty("MapFile", "", action=FileAction.OptionalLoad, extensions=["map", "xml"]),
            doc="Filename of the detector grouping map file to use. \n"
            "By default all the pixels will be summed per each tube. \n"
            "Use .map or .xml file (see GroupDetectors documentation) "
            "only if different range is needed for each tube.",
        )

        self.declareProperty(
            name="ManualPSDIntegrationRange",
            defaultValue=[1, 128],
            doc="Integration range of vertical pixels in each PSD tube. \n"
            "By default all the pixels will be summed per each tube. \n"
            "Use this option if the same range (other than default) "
            "is needed for all the tubes.",
        )

        self.declareProperty(name="Analyser", defaultValue="silicon", validator=StringListValidator(["silicon"]), doc="Analyser crystal.")

        self.declareProperty(
            name="Reflection", defaultValue="111", validator=StringListValidator(["111", "311"]), doc="Analyser reflection."
        )

        self.declareProperty(
            WorkspaceGroupProperty("OutputWorkspace", "", direction=Direction.Output), doc="Group name for the reduced workspace(s)."
        )

        self.declareProperty(
            name="SpectrumAxis",
            defaultValue="SpectrumNumber",
            validator=StringListValidator(["SpectrumNumber", "2Theta", "Q", "Q2"]),
            doc="The spectrum axis conversion target.",
        )

        self.declareProperty(name="DiscardSingleDetectors", defaultValue=False, doc="Whether to discard the spectra of single detectors.")

        self.declareProperty(
            name="GroupDetectors",
            defaultValue=True,
            doc="Group the pixels using the range, tube-by-tube (default) or in a custom way; \n"
            "it is not recommended to group the detectors at this stage, \n"
            "in order to get absorption corrections right, \n"
            "however the default value is True for backwards compatibility.",
        )

    def validateInputs(self):
        issues = dict()

        uo = self.getProperty("UnmirrorOption").value

        if (uo == 5 or uo == 7) and not self.getPropertyValue("AlignmentRun"):
            issues["AlignmentRun"] = "Given UnmirrorOption requires alignment run to be set"

        if (uo != 0) and not self.getProperty("GroupDetectors").value:
            issues["UnmirrorOption"] = "Currently UnmirrorOption=0 is required when GroupDetectors is switched off"

        if self.getPropertyValue("CalibrationRun"):
            range = self.getProperty("CalibrationPeakRange").value
            if len(range) != 2:
                issues["CalibrationPeakRange"] = "Please provide valid calibration range (comma separated 2 energy values)."
            elif range[0] >= range[1]:
                issues["CalibrationPeakRange"] = "Please provide valid calibration range. Start energy is bigger than end energy."

        if self.getPropertyValue("CalibrationBackgroundRun") and not self.getPropertyValue("CalibrationRun"):
            issues["CalibrationRun"] = "Calibration run is required when calibration background is given."

        return issues

    def setUp(self):
        self._sample_file = self.getPropertyValue("Run")
        self._alignment_file = self.getPropertyValue("AlignmentRun").replace(",", "+")  # automatic summing
        self._background_file = self.getPropertyValue("BackgroundRun").replace(",", "+")  # automatic summing
        self._calibration_file = self.getPropertyValue("CalibrationRun").replace(",", "+")  # automatic summing
        self._background_calib_files = self.getPropertyValue("CalibrationBackgroundRun").replace(",", "+")  # automatic summing
        self._sum_all_runs = self.getProperty("SumRuns").value
        self._unmirror_option = self.getProperty("UnmirrorOption").value
        self._back_scaling = self.getProperty("BackgroundScalingFactor").value
        self._back_calib_scaling = self.getProperty("CalibrationBackgroundScalingFactor").value
        self._peak_range = self.getProperty("CalibrationPeakRange").value
        self._spectrum_axis = self.getPropertyValue("SpectrumAxis")
        self._discard_sds = self.getProperty("DiscardSingleDetectors").value
        self._group_detectors = self.getProperty("GroupDetectors").value
        self._red_ws = self.getPropertyValue("OutputWorkspace")

        suffix = ""
        if self._spectrum_axis == "SpectrumNumber":
            suffix = "_red"
        elif self._spectrum_axis == "2Theta":
            suffix = "_2theta"
        elif self._spectrum_axis == "Q":
            suffix = "_q"
        elif self._spectrum_axis == "Q2":
            suffix = "_q2"

        self._red_ws += suffix

        # arguments to pass to IndirectILLEnergyTransfer
        self._common_args["MapFile"] = self.getPropertyValue("MapFile")
        self._common_args["Analyser"] = self.getPropertyValue("Analyser")
        self._common_args["Reflection"] = self.getPropertyValue("Reflection")
        self._common_args["ManualPSDIntegrationRange"] = self.getProperty("ManualPSDIntegrationRange").value
        self._common_args["CropDeadMonitorChannels"] = self.getProperty("CropDeadMonitorChannels").value
        self._common_args["SpectrumAxis"] = self._spectrum_axis
        self._common_args["DiscardSingleDetectors"] = self._discard_sds
        self._common_args["GroupDetectors"] = self._group_detectors

        if self._sum_all_runs is True:
            self.log().notice("All the sample runs will be summed")
            self._sample_file = self._sample_file.replace(",", "+")

        # Nexus metadata criteria for QENS type of data
        self._criteria = "$/entry0/instrument/Doppler/maximum_delta_energy$ != 0. and $/entry0/instrument/Doppler/velocity_profile$ == 0"

        # empty list to store all final workspaces to group
        self._ws_list = []

    def _mask(self, ws, xstart, xend):
        """
        Masks the first and last bins
        @param   ws           :: input workspace name
        @param   xstart       :: MaskBins between x[0] and x[xstart]
        @param   xend         :: MaskBins between x[xend] and x[-1]
        """
        x_values = mtd[ws].readX(0)

        if xstart > 0:
            self.log().debug("Mask bins smaller than {0}".format(xstart))
            MaskBins(InputWorkspace=ws, OutputWorkspace=ws, XMin=x_values[0], XMax=x_values[int(xstart)])

        if xend < len(x_values) - 1:
            self.log().debug("Mask bins larger than {0}".format(xend))
            MaskBins(InputWorkspace=ws, OutputWorkspace=ws, XMin=x_values[int(xend) + 1], XMax=x_values[-1])

    def _filter_files(self, files, label):
        """
        Filters the given list of files according to nexus criteria
        @param  files :: list of input files (i.e. , and + separated string)
        @param  label :: label of error message if nothing left after filtering
        @throws RuntimeError :: when nothing left after filtering
        @return :: the list of input files that passsed the criteria
        """

        files = SelectNexusFilesByMetadata(files, self._criteria)

        if not files:
            raise RuntimeError("None of the {0} runs are of QENS type. Check the files or reduction type.".format(label))
        else:
            self.log().information("Filtered {0} runs are: {1} \\n".format(label, files.replace(",", "\\n")))

        return files

    def _filter_all_input_files(self):
        """
        Filters all the lists of input files needed for the reduction.
        """

        self._sample_file = self._filter_files(self._sample_file, "sample")

        if self._background_file:
            self._background_file = self._filter_files(self._background_file, "background")

        if self._calibration_file:
            self._calibration_file = self._filter_files(self._calibration_file, "calibration")

        if self._background_calib_files:
            self._background_calib_files = self._filter_files(self._background_calib_files, "calibration background")

        if self._alignment_file:
            self._alignment_file = self._filter_files(self._alignment_file, "alignment")

    def _warn_negative_integral(self, ws, message):
        """
        Raises an error if an integral of the given workspace is <= 0
        @param ws :: input workspace name
        @param message :: message suffix for the error
        @throws RuntimeError :: on non-positive integral found
        """

        tmp_int = "__tmp_int" + ws
        Integration(InputWorkspace=ws, OutputWorkspace=tmp_int)

        for item in mtd[tmp_int]:
            for index in range(item.getNumberHistograms()):
                if item.readY(index)[0] <= 0:
                    self.log().warning("Negative or 0 integral in spectrum #{0} {1}".format(index, message))

        DeleteWorkspace(tmp_int)

    def PyExec(self):
        self.setUp()

        self._filter_all_input_files()

        if self._background_file:
            background = "__background_" + self._red_ws
            IndirectILLEnergyTransfer(Run=self._background_file, OutputWorkspace=background, **self._common_args)
            Scale(InputWorkspace=background, Factor=self._back_scaling, OutputWorkspace=background)

        if self._calibration_file:
            calibration = "__calibration_" + self._red_ws
            IndirectILLEnergyTransfer(Run=self._calibration_file, OutputWorkspace=calibration, **self._common_args)

            if self._background_calib_files:
                back_calibration = "__calibration_back_" + self._red_ws
                IndirectILLEnergyTransfer(Run=self._background_calib_files, OutputWorkspace=back_calibration, **self._common_args)
                Scale(InputWorkspace=back_calibration, Factor=self._back_calib_scaling, OutputWorkspace=back_calibration)
                Minus(LHSWorkspace=calibration, RHSWorkspace=back_calibration, OutputWorkspace=calibration)

            # MatchPeaks does not play nicely with the ws groups
            for ws in mtd[calibration]:
                MatchPeaks(InputWorkspace=ws.getName(), OutputWorkspace=ws.getName(), MaskBins=True, BinRangeTable="")

            Integration(
                InputWorkspace=calibration, RangeLower=self._peak_range[0], RangeUpper=self._peak_range[1], OutputWorkspace=calibration
            )
            self._warn_negative_integral(calibration, "in calibration run.")

        if self._unmirror_option == 5 or self._unmirror_option == 7:
            alignment = "__alignment_" + self._red_ws
            IndirectILLEnergyTransfer(Run=self._alignment_file, OutputWorkspace=alignment, **self._common_args)

        runs = self._sample_file.split(",")

        self._progress = Progress(self, start=0.0, end=1.0, nreports=len(runs))

        for run in runs:
            self._reduce_run(run)

        if self._background_file:
            DeleteWorkspace(background)

        if self._calibration_file:
            DeleteWorkspace(calibration)

        if self._background_calib_files:
            DeleteWorkspace(back_calibration)

        if self._unmirror_option == 5 or self._unmirror_option == 7:
            DeleteWorkspace(alignment)

        GroupWorkspaces(InputWorkspaces=self._ws_list, OutputWorkspace=self._red_ws)

        for ws in mtd[self._red_ws]:
            if ws.getRun().hasProperty("NormalisedTo"):
                if ws.getRun().getLogData("NormalisedTo").value == "Monitor":
                    ws.setDistribution(True)
            # unhide the final workspaces, i.e. remove __ prefix
            RenameWorkspace(InputWorkspace=ws, OutputWorkspace=ws.getName()[2:])

        self.setProperty("OutputWorkspace", self._red_ws)

    def _reduce_run(self, run):
        """
        Reduces the given (single or summed multiple) run
        @param run :: run path
        @throws RuntimeError : if inconsistent mirror sense is found in container or calibration run
        """

        runs_list = run.split("+")

        runnumber = os.path.basename(runs_list[0]).split(".")[0]

        self._progress.report("Reducing run #" + runnumber)

        ws = "__" + runnumber

        if len(runs_list) > 1:
            ws += "_multiple"

        ws += "_" + self._red_ws

        back_ws = "__background_" + self._red_ws

        calib_ws = "__calibration_" + self._red_ws

        IndirectILLEnergyTransfer(Run=run, OutputWorkspace=ws, **self._common_args)

        wings = mtd[ws].getNumberOfEntries()

        if self._background_file:
            if wings == mtd[back_ws].getNumberOfEntries():
                Minus(LHSWorkspace=ws, RHSWorkspace=back_ws, OutputWorkspace=ws)
                self._warn_negative_integral(ws, "after background subtraction.")
            else:
                raise RuntimeError("Inconsistent mirror sense in background run. Unable to perform subtraction.")

        if self._calibration_file:
            if wings == mtd[calib_ws].getNumberOfEntries():
                Divide(LHSWorkspace=ws, RHSWorkspace=calib_ws, OutputWorkspace=ws)
                self._scale_calibration(ws, calib_ws)
            else:
                raise RuntimeError("Inconsistent mirror sense in calibration run. Unable to perform calibration.")

        self._perform_unmirror(ws, runnumber)

        # register to reduced runs list
        self._ws_list.append(ws)

    def _scale_calibration(self, ws, calib_ws):
        """
        Scales the wings of calibrated sample ws with the maximum
        of the integrated intensities in each wing of calib ws
        @param ws       :: calibrated sample workspace
        @param calib_ws :: calibration workspace
        """

        # number of wings are checked to be the same in ws and calib_ws here already

        for wing in range(mtd[ws].getNumberOfEntries()):
            sample = mtd[ws].getItem(wing).getName()
            integral = mtd[calib_ws].getItem(wing).getName()
            scale = numpy.max(mtd[integral].extractY()[:, 0])
            self.log().information("Wing {0} will be scaled up with {1} after calibration".format(wing, scale))
            Scale(InputWorkspace=sample, Factor=scale, OutputWorkspace=sample, Operation="Multiply")

    def _perform_unmirror(self, ws, run):
        """
        Performs unmirroring, i.e. summing of left and right wings
        for two-wing data or centering the one wing data
        @param ws  :: workspace
        @param run :: runnumber
        @throws RuntimeError : if the size of the left and right wings do not match in 2-wings case
                               and the unmirror option is 1 or >3
        @throws RuntimeError : if the mirros sense in the alignment run is inconsistent
        """

        outname = ws + "_tmp"

        wings = mtd[ws].getNumberOfEntries()

        self.log().information("Unmirroring workspace {0} with option {1}".format(ws, self._unmirror_option))

        alignment = "__alignment_" + self._red_ws

        # make sure the sample and alignment runs have the same mirror sense for unmirror 5,7
        if self._unmirror_option == 5 or self._unmirror_option == 7:
            if wings != mtd[alignment].getNumberOfEntries():
                raise RuntimeError("Inconsistent mirror sense in alignment run. Unable to perform unmirror.")

        if wings == 1:  # one wing
            name = mtd[ws].getItem(0).getName()

            if self._unmirror_option < 6:  # do unmirror 0, i.e. nothing
                CloneWorkspace(InputWorkspace=name, OutputWorkspace=outname)
            elif self._unmirror_option == 6:
                MatchPeaks(InputWorkspace=name, OutputWorkspace=outname, MaskBins=True, BinRangeTable="")
            elif self._unmirror_option == 7:
                MatchPeaks(
                    InputWorkspace=name,
                    InputWorkspace2=mtd[alignment].getItem(0).getName(),
                    MatchInput2ToCenter=True,
                    OutputWorkspace=outname,
                    MaskBins=True,
                    BinRangeTable="",
                )

        elif wings == 2:  # two wing
            left = mtd[ws].getItem(0).getName()
            right = mtd[ws].getItem(1).getName()

            mask_min = 0
            mask_max = mtd[left].blocksize()

            if (
                self._common_args["CropDeadMonitorChannels"]
                and (self._unmirror_option == 1 or self._unmirror_option > 3)
                and mtd[left].blocksize() != mtd[right].blocksize()
            ):
                raise RuntimeError(
                    "Different number of bins found in the left and right wings"
                    " after cropping the dead monitor channels. "
                    "Unable to perform the requested unmirror option, consider using option "
                    "0, 2 or 3 or switch off the CropDeadMonitorChannels."
                )

            if self._unmirror_option == 0:
                left_out = "__" + run + "_" + self._red_ws + "_left"
                right_out = "__" + run + "_" + self._red_ws + "_right"
                CloneWorkspace(InputWorkspace=left, OutputWorkspace=left_out)
                CloneWorkspace(InputWorkspace=right, OutputWorkspace=right_out)
                GroupWorkspaces(InputWorkspaces=[left_out, right_out], OutputWorkspace=outname)
            elif self._unmirror_option == 1:
                Plus(LHSWorkspace=left, RHSWorkspace=right, OutputWorkspace=outname)
                Scale(InputWorkspace=outname, OutputWorkspace=outname, Factor=0.5)
            elif self._unmirror_option == 2:
                CloneWorkspace(InputWorkspace=left, OutputWorkspace=outname)
            elif self._unmirror_option == 3:
                CloneWorkspace(InputWorkspace=right, OutputWorkspace=outname)
            elif self._unmirror_option == 4:
                bin_range_table = "__um4_" + right
                MatchPeaks(InputWorkspace=right, InputWorkspace2=left, OutputWorkspace=right, MaskBins=True, BinRangeTable=bin_range_table)
                mask_min = mtd[bin_range_table].row(0)["MinBin"]
                mask_max = mtd[bin_range_table].row(0)["MaxBin"]
                DeleteWorkspace(bin_range_table)
            elif self._unmirror_option == 5:
                bin_range_table = "__um5_" + right
                MatchPeaks(
                    InputWorkspace=right,
                    InputWorkspace2=mtd[alignment].getItem(0).getName(),
                    InputWorkspace3=mtd[alignment].getItem(1).getName(),
                    OutputWorkspace=right,
                    MaskBins=True,
                    BinRangeTable=bin_range_table,
                )
                mask_min = mtd[bin_range_table].row(0)["MinBin"]
                mask_max = mtd[bin_range_table].row(0)["MaxBin"]
                DeleteWorkspace(bin_range_table)
            elif self._unmirror_option == 6:
                bin_range_table_left = "__um6_" + left
                bin_range_table_right = "__um6_" + right
                MatchPeaks(InputWorkspace=left, OutputWorkspace=left, MaskBins=True, BinRangeTable=bin_range_table_left)
                MatchPeaks(InputWorkspace=right, OutputWorkspace=right, MaskBins=True, BinRangeTable=bin_range_table_right)
                mask_min = max(mtd[bin_range_table_left].row(0)["MinBin"], mtd[bin_range_table_right].row(0)["MinBin"])
                mask_max = min(mtd[bin_range_table_left].row(0)["MaxBin"], mtd[bin_range_table_right].row(0)["MaxBin"])
                DeleteWorkspace(bin_range_table_left)
                DeleteWorkspace(bin_range_table_right)
            elif self._unmirror_option == 7:
                bin_range_table_left = "__um7_" + left
                bin_range_table_right = "__um7_" + right
                MatchPeaks(
                    InputWorkspace=left,
                    InputWorkspace2=mtd[alignment].getItem(0).getName(),
                    OutputWorkspace=left,
                    MatchInput2ToCenter=True,
                    MaskBins=True,
                    BinRangeTable=bin_range_table_left,
                )
                MatchPeaks(
                    InputWorkspace=right,
                    InputWorkspace2=mtd[alignment].getItem(1).getName(),
                    OutputWorkspace=right,
                    MatchInput2ToCenter=True,
                    MaskBins=True,
                    BinRangeTable=bin_range_table_right,
                )
                mask_min = max(mtd[bin_range_table_left].row(0)["MinBin"], mtd[bin_range_table_right].row(0)["MinBin"])
                mask_max = min(mtd[bin_range_table_left].row(0)["MaxBin"], mtd[bin_range_table_right].row(0)["MaxBin"])
                DeleteWorkspace(bin_range_table_left)
                DeleteWorkspace(bin_range_table_right)

            if self._unmirror_option > 3:
                Plus(LHSWorkspace=left, RHSWorkspace=right, OutputWorkspace=outname)
                Scale(InputWorkspace=outname, OutputWorkspace=outname, Factor=0.5)
                self._mask(outname, mask_min, mask_max)

        DeleteWorkspace(ws)
        RenameWorkspace(InputWorkspace=outname, OutputWorkspace=ws)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(IndirectILLReductionQENS)
