# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name, no-init
import os
import re
from mantid.api import mtd, AlgorithmFactory, FileAction, FileProperty, PythonAlgorithm
from mantid.kernel import logger, Direction, FloatArrayProperty, FloatArrayLengthValidator, IntArrayProperty, Property
from mantid.simpleapi import (
    CreateWorkspace,
    CropWorkspace,
    Divide,
    Fit,
    LoadEventNexus,
    LRSubtractAverageBackground,
    Multiply,
    NormaliseByCurrent,
    Rebin,
    ReplaceSpecialValues,
    SumSpectra,
)


class LRScalingFactors(PythonAlgorithm):
    """
    This algorithm runs through a sequence of direct beam data sets
    to extract scaling factors. The method was developed by J. Ankner (ORNL).

    As we loop through, we find matching data sets with the only
    difference between the two is an attenuator.
    The ratio of those matching data sets allows use to rescale
    a direct beam run taken with a larger number of attenuators
    to a standard data set taken with tighter slit settings and
    no attenuators.

    The normalization run for a data set taken in a given slit setting
    configuration can then be expressed in terms of the standard 0-attenuator
    data set with:
        D_i = F_i D_0

    Here's an example of runs and how they are related to F.

    run: 55889, att: 0, s1: 0.26, s2: 0.26
    run: 55890, att: 1, s1: 0.26, s2: 0.26
    run: 55891, att: 1, s1: 0.33, s2: 0.26 --> F = 55891 / 55890
    run: 55892, att: 1, s1: 0.45, s2: 0.26 --> F = 55892 / 55890
    run: 55895, att: 1, s1: 0.81, s2: 0.26
    run: 55896, att: 2, s1: 0.81, s2: 0.26
    run: 55897, att: 2, s1: 1.05, s2: 0.35 --> F = 55897 / 55896 * 55895 / 55890

    """

    tolerance = 0.0

    def category(self):
        return "Reflectometry\\SNS"

    def name(self):
        return "LiquidsReflectometryScalingFactors"

    def version(self):
        return 1

    def summary(self):
        return "Liquids Reflectometer (REFL) scaling factor calculation"

    def PyInit(self):
        self.declareProperty(IntArrayProperty("DirectBeamRuns", []), "Run number of the signal run to use")
        self.declareProperty(IntArrayProperty("Attenuators", []), "Number of attenuators for each run")
        self.declareProperty(
            FloatArrayProperty("TOFRange", [10000.0, 35000.0], FloatArrayLengthValidator(2), direction=Direction.Input), "TOF range to use"
        )
        self.declareProperty(IntArrayProperty("SignalPeakPixelRange", [150, 160]), "Pixel range defining the data peak")
        self.declareProperty(IntArrayProperty("SignalBackgroundPixelRange", [147, 163]), "Pixel range defining the background")
        self.declareProperty(
            IntArrayProperty("LowResolutionPixelRange", [Property.EMPTY_INT, Property.EMPTY_INT], direction=Direction.Input),
            "Pixel range defining the region to use in the low-resolution direction",
        )
        self.declareProperty("IncidentMedium", "Medium", doc="Name of the incident medium")
        self.declareProperty("FrontSlitName", "S1", doc="Name of the front slit")
        self.declareProperty("BackSlitName", "Si", doc="Name of the back slit")
        self.declareProperty("TOFSteps", 500.0, doc="TOF step size")
        self.declareProperty("SlitTolerance", 0.02, doc="Tolerance for matching slit positions")
        self.declareProperty(FileProperty("ScalingFactorFile", "", action=FileAction.Save, extensions=["cfg"]))

    # pylint: disable=too-many-locals,too-many-branches
    def PyExec(self):
        # Verify whether we have a sorted list of runs.
        data_runs = self.getProperty("DirectBeamRuns").value

        # We will be rejecting prompt pulses. We will store pulses here:
        self.prompt_pulses = None

        # Get a valid attenuator array
        self.read_attenuators_property(len(data_runs))

        # Get peak ranges
        peak_range = self.get_valid_pixel_range("SignalPeakPixelRange", len(data_runs))
        background_range = self.get_valid_pixel_range("SignalBackgroundPixelRange", len(data_runs))
        lowres_range = self.get_valid_pixel_range("LowResolutionPixelRange", len(data_runs))

        # Slit information for the previous run (see loop below)
        previous_slits = None

        # Previous processed workspace
        previous_ws = None

        # Number of attenuators for the run being considered
        n_attenuator = 0

        # Transition sreferences used to propagate the attenuation
        self.references = {}

        # Current wavelength value
        self.wavelength = None
        self.wavelength_tolerance = 0.2

        # Slit settings tolerance
        self.tolerance = self.getProperty("SlitTolerance").value

        # Scaling factor output
        self.scaling_factors = []

        # Run through the runs
        for i in range(len(data_runs)):
            run = data_runs[i]
            workspace_name = "REF_L_%s" % int(run)
            workspace = LoadEventNexus("REF_L_%s" % run, OutputWorkspace=workspace_name)

            # Get S1H, S2H, S1W, S2W
            s1h, s1w, s2h, s2w = self.get_slit_settings(workspace)

            # Get wavelength, to make sure they all match across runs
            self.validate_wavelength(workspace)

            # Get attenuators
            current_att = n_attenuator
            n_attenuator = self.get_attenuators(workspace, i)
            if current_att > n_attenuator:
                raise RuntimeError("Runs were not supplied in increasing number of attenuators.")

            self.process_data(
                workspace,
                peak_range=[int(peak_range[2 * i]), int(peak_range[2 * i + 1])],
                background_range=[int(background_range[2 * i]), int(background_range[2 * i + 1])],
                low_res_range=[int(lowres_range[2 * i]), int(lowres_range[2 * i + 1])],
            )

            is_reference = False

            # Matching slits with the previous run signals a reference run
            if (
                i > 0
                and previous_slits is not None
                and abs(previous_slits[0] - s1h) < self.tolerance
                and abs(previous_slits[1] - s1w) < self.tolerance
                and abs(previous_slits[2] - s2h) < self.tolerance
                and abs(previous_slits[3] - s2w) < self.tolerance
            ):
                is_reference = True

            previous_slits = [s1h, s1w, s2h, s2w]

            # If the number of attenuators is zero, skip.
            if n_attenuator == 0:
                if 0 in self.references:
                    raise RuntimeError("More than one run with zero attenuator was supplied.")
                self.references[0] = {"index": i, "run": run, "ref_ws": workspace_name, "ratio_ws": None, "diagnostics": str(run)}
                previous_ws = workspace_name
                continue

            if is_reference is True:
                self.references[n_attenuator] = {"index": i, "run": run, "ref_ws": workspace_name}
                # Compute ratio of the run with fewer attenuators and the
                # reference with the same number of attenuators as that run.
                Divide(
                    LHSWorkspace=previous_ws,
                    RHSWorkspace=self.references[n_attenuator - 1]["ref_ws"],
                    OutputWorkspace="ScalingRatio_%s" % n_attenuator,
                )
                self.references[n_attenuator]["diagnostics"] = "%s / %s" % (str(data_runs[i - 1]), self.references[n_attenuator - 1]["run"])
                # Multiply the result by the ratio for that run, and store
                if self.references[n_attenuator - 1]["ratio_ws"] is not None:
                    Multiply(
                        LHSWorkspace=self.references[n_attenuator - 1]["ratio_ws"],
                        RHSWorkspace="ScalingRatio_%s" % n_attenuator,
                        OutputWorkspace="ScalingRatio_%s" % n_attenuator,
                    )
                    self.references[n_attenuator]["diagnostics"] += " * %s" % self.references[n_attenuator - 1]["diagnostics"]
                self.references[n_attenuator]["ratio_ws"] = "ScalingRatio_%s" % n_attenuator

            # If this is not a reference run, compute F
            else:
                self.compute_scaling_factor(run, n_attenuator, workspace_name)

            previous_ws = workspace_name

        # Log some useful information to track what happened
        for item in self.scaling_factors:
            log_info = "LambdaRequested=%s " % item["LambdaRequested"]
            log_info += "S1H=%s " % item["S1H"]
            log_info += "S2iH=%s " % item["S2iH"]
            log_info += "S1W=%s " % item["S1W"]
            log_info += "S2iW=%s " % item["S2iW"]
            log_info += "a=%s " % item["a"]
            log_info += "b=%s " % item["b"]
            log_info += "  |  %s" % item["diagnostics"]
            logger.information(log_info)
        # Save the output in a configuration file
        self.save_scaling_factor_file()

    def validate_wavelength(self, workspace):
        """
        All supplied runs should have the same wavelength band.
        Verify that it is the case or raise an exception.

        @param workspace: data set we are checking
        """
        # Get the wavelength for the workspace being considered
        wl = workspace.getRun().getProperty("LambdaRequest").value[0]

        # If this is the first workspace we process, set the our
        # internal reference value to be used with the following runs.
        if self.wavelength is None:
            self.wavelength = wl
        # Check that the wavelength is the same as our reference within tolerence.
        elif abs(wl - self.wavelength) > self.wavelength_tolerance:
            raise RuntimeError("Supplied runs don't have matching wavelengths.")

    def read_attenuators_property(self, number_of_runs):
        """
        Return the number of attenuators for each run as an array.
        Check whether the supplied attenuation array is of the same length,
        otherwise we will read the number of attenuators from the logs.

        @param number_of_runs: number of runs we are processing
        """
        self.attenuators = self.getProperty("Attenuators").value
        self.have_attenuator_info = False
        if len(self.attenuators) == 0:
            logger.notice("No attenuator information supplied: will be determined.")
        elif not len(self.attenuators) == number_of_runs:
            logger.error("Attenuation list should be of the same length as the list of runs")
        else:
            self.have_attenuator_info = True

    def get_attenuators(self, workspace, expInfoIndex):
        """
        @param workspace: workspace we are determining the number of attenuators for
        @param expInfoIndex: index of the run in case we are getting the attenuators from the input properties
        """
        if self.have_attenuator_info:
            return self.attenuators[expInfoIndex]
        else:
            return int(workspace.getRun().getProperty("vAtt").value[0] - 1)

    def get_valid_pixel_range(self, property_name, number_of_runs):
        """
        Return a valid pixel range we can use in our calculations.
        The output is a list of length 2*number_of_runs.

        @param property_name: name of the algorithm property specifying a pixel range
        @param number_of_runs: number of runs we are processing
        """
        pixel_range = self.getProperty(property_name).value
        if len(pixel_range) == 2:
            x_min = int(pixel_range[0])
            x_max = int(pixel_range[1])
            pixel_range = 2 * number_of_runs * [0]
            for i in range(number_of_runs):
                pixel_range[2 * i] = x_min
                pixel_range[2 * i + 1] = x_max
        elif len(pixel_range) < 2:
            raise RuntimeError("%s should have a length of at least 2." % property_name)

        # Check that the peak range arrays are of the proper length
        if not len(pixel_range) == 2 * number_of_runs:
            raise RuntimeError("Supplied peak/background arrays should be of the same length as the run array.")

        return pixel_range

    def get_slit_settings(self, workspace):
        """
        Return the slit settings
        @param workspace: workspace to get the information from
        """
        # Get the slit information
        front_slit = self.getProperty("FrontSlitName").value
        back_slit = self.getProperty("BackSlitName").value

        # Get S1H, S2H, S1W, S2W
        s1h = abs(workspace.getRun().getProperty("%sVHeight" % front_slit).value[0])
        s1w = abs(workspace.getRun().getProperty("%sHWidth" % front_slit).value[0])
        try:
            s2h = abs(workspace.getRun().getProperty("%sVHeight" % back_slit).value[0])
            s2w = abs(workspace.getRun().getProperty("%sHWidth" % back_slit).value[0])
        except RuntimeError:
            # For backward compatibility with old code
            logger.error("Specified slit could not be found: %s  Trying S2" % back_slit)
            s2h = abs(workspace.getRun().getProperty("S2VHeight").value[0])
            s2w = abs(workspace.getRun().getProperty("S2HWidth").value[0])
        return s1h, s1w, s2h, s2w

    def compute_scaling_factor(self, run, n_attenuator, workspace_name):
        """
        Compute the scaling factor for this run.
        @param run: run number we are working with
        @param n_attenuator: number of attenuators for this run
        @param workspace_name: name of processed workspace
        """
        # Divide by the reference for this number of attenuators
        # and multiply by the reference ratio
        if n_attenuator not in self.references:
            raise RuntimeError("No reference for %s attenuators: check run ordering." % n_attenuator)
        f_ws = "F_%s_%s" % (run, n_attenuator)
        Divide(LHSWorkspace=workspace_name, RHSWorkspace=self.references[n_attenuator]["ref_ws"], OutputWorkspace=f_ws)
        Multiply(LHSWorkspace=self.references[n_attenuator]["ratio_ws"], RHSWorkspace=f_ws, OutputWorkspace=f_ws)
        # Store the final result for this setting
        ReplaceSpecialValues(
            InputWorkspace=f_ws, OutputWorkspace=f_ws, NaNValue=0.0, NaNError=1000.0, InfinityValue=0.0, InfinityError=1000.0
        )

        # Remove prompt pulse bin, replace the y value by the
        # average and give it a very large error.
        x_values = mtd[f_ws].readX(0)
        y_values = mtd[f_ws].dataY(0)
        e_values = mtd[f_ws].dataE(0)
        # We will create a cleaned up workspace without the bins
        # corresponding to the prompt pulses
        x_clean = []
        y_clean = []
        e_clean = []
        for i in range(len(y_values)):
            has_prompt_pulse = self.is_prompt_pulse_in_range(mtd[f_ws], x_values[i], x_values[i + 1])
            if has_prompt_pulse:
                logger.notice("Prompt pulse bin [%g, %g]" % (x_values[i], x_values[i + 1]))
            elif y_values[i] > 0.0:
                x_clean.append((x_values[i + 1] + x_values[i]) / 2.0)
                y_clean.append(y_values[i])
                e_clean.append(e_values[i])

        CreateWorkspace(OutputWorkspace=f_ws, DataX=x_clean, DataY=y_clean, DataE=e_clean, NSpec=1)

        Fit(InputWorkspace=f_ws, Function="name=UserFunction, Formula=a+b*x, a=1, b=0", Output="fit_result")
        a = mtd["fit_result_Parameters"].cell(0, 1)
        b = mtd["fit_result_Parameters"].cell(1, 1)
        error_a = mtd["fit_result_Parameters"].cell(0, 2)
        error_b = mtd["fit_result_Parameters"].cell(1, 2)

        medium = self.getProperty("IncidentMedium").value
        wl = mtd[workspace_name].getRun().getProperty("LambdaRequest").value[0]
        s1h, s1w, s2h, s2w = self.get_slit_settings(mtd[workspace_name])
        self.scaling_factors.append(
            {
                "IncidentMedium": medium,
                "LambdaRequested": wl,
                "S1H": s1h,
                "S1W": s1w,
                "S2iH": s2h,
                "S2iW": s2w,
                "a": a,
                "error_a": error_a,
                "b": b,
                "error_b": error_b,
                "diagnostics": "%s / %s * %s" % (run, self.references[n_attenuator]["run"], self.references[n_attenuator]["diagnostics"]),
            }
        )

    def is_prompt_pulse_in_range(self, workspace, x_min, x_max):
        """
        Returns True if a prompt pulse is in the given TOF range
        @param workspace: a data workspace to get the frequency from
        @param x_min: min TOF value
        @param x_max: max TOD value
        """
        # Initialize the prompt pulses if it hasn't been done
        if self.prompt_pulses is None:
            # Accelerator rep rate
            # Use max here because the first entry can be zero
            frequency = max(workspace.getRun().getProperty("frequency").value)

            # Go up to 4 frames - that should cover more than enough TOF
            self.prompt_pulses = [1.0e6 / frequency * i for i in range(4)]

        for peak_x in self.prompt_pulses:
            if peak_x > x_min and peak_x < x_max:
                return True
        return False

    def save_scaling_factor_file(self):
        """
        Save the output. First see if the scaling factor file exists.
        If it does, we need to update it.
        """
        scaling_file = self.getPropertyValue("ScalingFactorFile")
        # Extend the existing content of the scaling factor file
        scaling_file_content, scaling_file_meta = self.read_scaling_factor_file(scaling_file)
        scaling_file_content.extend(self.scaling_factors)

        direct_beams = list(self.getProperty("DirectBeamRuns").value)
        medium = self.getProperty("IncidentMedium").value
        scaling_file_meta[medium] = "# Medium=%s, runs: %s" % (medium, direct_beams)

        fd = open(scaling_file, "w")
        fd.write("# y=a+bx\n#\n")
        fd.write("# LambdaRequested[Angstroms] S1H[mm] (S2/Si)H[mm] S1W[mm] (S2/Si)W[mm] a b error_a error_b\n#\n")

        for k, v in scaling_file_meta.items():
            fd.write("%s\n" % v)
        for item in scaling_file_content:
            fd.write("IncidentMedium=%s " % item["IncidentMedium"])
            fd.write("LambdaRequested=%s " % item["LambdaRequested"])
            fd.write("S1H=%s " % item["S1H"])
            fd.write("S2iH=%s " % item["S2iH"])
            fd.write("S1W=%s " % item["S1W"])
            fd.write("S2iW=%s " % item["S2iW"])
            fd.write("a=%s " % item["a"])
            fd.write("b=%s " % item["b"])
            fd.write("error_a=%s " % item["error_a"])
            fd.write("error_b=%s\n" % item["error_b"])
        fd.close()

    def read_scaling_factor_file(self, scaling_file):
        """
        Read in a scaling factor file and return its content as a list of entries
        @param scaling_file: path of the scaling factor file to read
        """
        scaling_file_content = []
        scaling_file_meta = {}
        if os.path.isfile(scaling_file):
            fd = open(scaling_file, "r")
            content = fd.read()
            fd.close()
            for line in content.split("\n"):
                if line.startswith("# Medium="):
                    m = re.search("# Medium=(.+), runs", line)
                    if m is not None:
                        scaling_file_meta[m.group(1)] = line
                    continue
                elif line.startswith("#") or len(line.strip()) == 0:
                    continue
                toks = line.split()
                entry = {}
                for token in toks:
                    pair = token.split("=")
                    entry[pair[0].strip()] = pair[1].strip()
                # If we are about to update an entry, don't include it in the new file
                add_this_entry = True
                for new_entry in self.scaling_factors:
                    is_matching = entry["IncidentMedium"] == new_entry["IncidentMedium"]
                    for slit in ["LambdaRequested", "S1H", "S1W", "S2iH", "S2iW"]:
                        is_matching = is_matching and abs(float(entry[slit]) - float(new_entry[slit])) < self.tolerance
                    if is_matching:
                        add_this_entry = False
                if add_this_entry:
                    scaling_file_content.append(entry)
        return scaling_file_content, scaling_file_meta

    def process_data(self, workspace, peak_range, background_range, low_res_range):
        """
        Common processing for both sample data and normalization.
        @param workspace: name of the workspace to work with
        @param peak_range: range of pixels defining the peak
        @param background_range: range of pixels defining the background
        @param low_res_range: range of pixels in the x-direction
        """
        # Check low-res axis
        if low_res_range[0] == Property.EMPTY_INT:
            low_res_range[0] = 0
        if low_res_range[1] == Property.EMPTY_INT:
            low_res_range[1] = int(workspace.getInstrument().getNumberParameter("number-of-x-pixels")[0]) - 1

        # Rebin TOF axis
        tof_range = self.getProperty("TOFRange").value
        tof_step = self.getProperty("TOFSteps").value
        workspace = Rebin(
            InputWorkspace=workspace, Params=[tof_range[0], tof_step, tof_range[1]], PreserveEvents=False, OutputWorkspace=str(workspace)
        )

        # Subtract background
        workspace = LRSubtractAverageBackground(
            InputWorkspace=workspace,
            PeakRange=peak_range,
            BackgroundRange=background_range,
            LowResolutionRange=low_res_range,
            OutputWorkspace=str(workspace),
            ErrorWeighting=True,
        )

        # Normalize by current proton charge
        # Note that the background subtraction will use an error weighted mean
        # and use 1 as the error on counts of zero. We normalize by the integrated
        # current _after_ the background subtraction so that the 1 doesn't have
        # to be changed to a 1/Charge.
        workspace = NormaliseByCurrent(InputWorkspace=workspace, OutputWorkspace=str(workspace))

        # Crop to only the selected peak region
        workspace = CropWorkspace(
            InputWorkspace=workspace,
            StartWorkspaceIndex=int(peak_range[0]),
            EndWorkspaceIndex=int(peak_range[1]),
            OutputWorkspace=str(workspace),
        )
        workspace = SumSpectra(InputWorkspace=workspace, OutputWorkspace=str(workspace))

        return str(workspace)


AlgorithmFactory.subscribe(LRScalingFactors)
