# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name,bare-except
import math
import time
import mantid
from mantid.api import mtd, AnalysisDataService, AlgorithmFactory, FileAction, FileProperty, PythonAlgorithm
from mantid.kernel import logger, Direction, FloatArrayProperty, StringArrayProperty
from mantid.simpleapi import Rebin, Scale


class LRReflectivityOutput(PythonAlgorithm):
    def category(self):
        return "Reflectometry\\SNS"

    def name(self):
        return "LRReflectivityOutput"

    def version(self):
        return 1

    def summary(self):
        return "Produce a single reflectivity curve from multiple reflectivity ranges."

    def PyInit(self):
        self.declareProperty(
            StringArrayProperty("ReducedWorkspaces", [], direction=Direction.Input),
            "List of workspace names of reduced reflectivity parts to be put together",
        )
        self.declareProperty("SpecularCutoff", 0.01, "Q-value under which we should below the specular ridge")
        self.declareProperty("ScaleToUnity", True, "If true, the reflectivity under the Q given cutoff will be scaled to 1")
        self.declareProperty("ScalingWavelengthCutoff", 10.0, "Wavelength above which the scaling factors are assumed to be one")
        self.declareProperty(FloatArrayProperty("OutputBinning", [0.005, -0.01, 1.0], direction=Direction.Input))
        self.declareProperty("DQConstant", 0.0004, "Constant factor for the resolution dQ = dQ0 + Q dQ/Q")
        self.declareProperty("DQSlope", 0.025, "Slope for the resolution dQ = dQ0 + Q dQ/Q")
        self.declareProperty("ComputeDQ", True, "If true, the Q resolution will be computed")
        self.declareProperty("FrontSlitName", "S1", doc="Name of the front slit")
        self.declareProperty(
            FileProperty("OutputFilename", "", action=FileAction.Save, extensions=["txt"]), doc="Name of the reflectivity file output"
        )
        self.declareProperty("MetaData", "", "Additional meta-data to add to the top of the output file")

    def PyExec(self):
        # Check that all the input workspaces are scaled
        workspace_list = self.getProperty("ReducedWorkspaces").value
        if not self.check_scaling(workspace_list):
            logger.error("Absolute normalization not available!")

        # Put the workspaces together
        self.average_points_for_single_q(workspace_list)

    def compute_resolution(self, ws):
        """
        Compute the Q resolution from the meta data.
        """
        # We can't compute the resolution if the value of xi is not in the logs.
        # Since it was not always logged, check for it here.
        if not ws.getRun().hasProperty("BL4B:Mot:xi.RBV"):
            logger.notice("Could not find BL4B:Mot:xi.RBV: using supplied dQ/Q")
            return None

        # Xi reference would be the position of xi if the si slit were to be positioned
        # at the sample. The distance from the sample to si is then xi_reference - xi.
        xi_reference = 445
        if ws.getInstrument().hasParameter("xi-reference"):
            ws.getInstrument().getNumberParameter("xi-reference")[0]

        # Distance between the s1 and the sample
        s1_sample_distance = 1485
        if ws.getInstrument().hasParameter("s1-sample-distance"):
            ws.getInstrument().getNumberParameter("s1-sample-distance")[0]

        front_slit = self.getProperty("FrontSlitName").value
        s1h = abs(ws.getRun().getProperty("%sVHeight" % front_slit).value[0])
        ths = abs(ws.getRun().getProperty("ths").value[0])
        xi = abs(ws.getRun().getProperty("BL4B:Mot:xi.RBV").value[0])
        sample_si_distance = xi_reference - xi
        slit_distance = s1_sample_distance - sample_si_distance
        dq_over_q = s1h / slit_distance * 180 / 3.1416 / ths
        return dq_over_q

    def check_scaling(self, workspace_list):
        """
        Check that all the workspaces are on an absolute scale.
        @param workspace_list: list of workspaces to put together
        """
        scaling_cutoff = self.getProperty("ScalingWavelengthCutoff").value

        normalization_available = True
        for ws in workspace_list:
            if mtd[ws].getRun().hasProperty("isSFfound"):
                if mtd[ws].getRun().getProperty("isSFfound").value == "False":
                    try:
                        wl = mtd[ws].getRun().getProperty("LambdaRequest").value[0]
                        # Scaling factor above the wavelength cutoff are assumed to be 1
                        if wl > scaling_cutoff:
                            logger.notice("%s: no normalization needed for wl=%s" % (ws, str(wl)))
                        else:
                            logger.error("%s: normalization missing for wl=%s" % (ws, str(wl)))
                            normalization_available = False
                    except:
                        logger.error("%s: could not find LambdaRequest" % ws)
                        normalization_available = False
                else:
                    logger.notice("%s: normalization found" % ws)
            else:
                logger.error("%s: no normalization info" % ws)
                normalization_available = False
        return normalization_available

    # pylint: disable=too-many-locals,too-many-branches
    def average_points_for_single_q(self, scaled_ws_list):  # noqa
        """
        Take the point with the smalled error when multiple points are
        at the same q-value.

        This code was originally part of the REFL UI.

        @param scaled_ws_list: list of scaled workspaces to combine
        """
        # Get binning parameters
        binning_parameters = self.getProperty("OutputBinning").value
        header_list = (
            "DataRun",
            "NormRun",
            "TwoTheta(deg)",
            "LambdaMin(A)",
            "LambdaMax(A)",
            "Qmin(1/A)",
            "Qmax(1/A)",
            "SF_A",
            "SF_B",
            "PrimaryFrac",
        )
        header_info = "# %-9s %-9s %-14s %-14s %-12s %-12s %-12s %-12s %-12s %-12s\n" % header_list
        # Convert each histo to histograms and rebin to final binning
        for ws in scaled_ws_list:
            new_name = "%s_histo" % ws
            # ConvertToHistogram(InputWorkspace=ws, OutputWorkspace=new_name)
            mtd[ws].setDistribution(True)
            Rebin(InputWorkspace=ws, Params=binning_parameters, OutputWorkspace=new_name)

            # Gather info for meta data header
            def _get_value(name, default=None):
                if mtd[new_name].getRun().hasProperty(name):
                    return mtd[new_name].getRun().getProperty(name).value
                return default

            data_run = mtd[new_name].getRun().getProperty("run_number").value
            norm_run = mtd[new_name].getRun().getProperty("normalization_run").value
            two_theta = mtd[new_name].getRun().getProperty("two_theta").value
            lambda_min = mtd[new_name].getRun().getProperty("lambda_min").value
            lambda_max = mtd[new_name].getRun().getProperty("lambda_max").value
            data_q_min = mtd[new_name].getRun().getProperty("q_min").value
            data_q_max = mtd[new_name].getRun().getProperty("q_max").value
            primary_fraction = mtd[new_name].getRun().getProperty("primary_fraction").value
            scaling_factor_a = _get_value("scaling_factor_a", 1.0)
            scaling_factor_b = _get_value("scaling_factor_b", 0.0)
            value_list = (
                data_run,
                norm_run,
                two_theta,
                lambda_min,
                lambda_max,
                data_q_min,
                data_q_max,
                scaling_factor_a,
                scaling_factor_b,
                primary_fraction,
            )
            header_info += "# %-9s %-9s %-14.6g %-14.6g %-12.6g %-12.6s %-12.6s %-12.6s %-12.6s %-12.6s\n" % value_list

        # Take the first rebinned histo as our output
        data_x = mtd[scaled_ws_list[0] + "_histo"].dataX(0)
        data_y = mtd[scaled_ws_list[0] + "_histo"].dataY(0)
        data_e = mtd[scaled_ws_list[0] + "_histo"].dataE(0)

        # Skip first point and last one
        points_to_skip = 1
        for i in range(1, len(scaled_ws_list)):
            skipped_points = 0
            distribution_started = False

            data_y_i = mtd[scaled_ws_list[i] + "_histo"].dataY(0)
            data_e_i = mtd[scaled_ws_list[i] + "_histo"].dataE(0)
            for j in range(len(data_y_i) - 1):
                # Check whether we need to skip this point
                if data_y_i[j] > 0:
                    distribution_started = True
                    if skipped_points < points_to_skip:
                        skipped_points += 1
                        continue

                # If this is the last point of the distribution, skip it
                if distribution_started and data_y_i[j + 1] == 0 and data_e_i[j + 1] == 0:
                    break

                if data_y_i[j] > 0:
                    if data_y[j] > 0:
                        denom = 1.0 / data_e[j] ** 2 + 1.0 / data_e_i[j] ** 2
                        data_y[j] = (data_y[j] / data_e[j] ** 2 + data_y_i[j] / data_e_i[j] ** 2) / denom
                        data_e[j] = math.sqrt(1.0 / denom)

                        # data_y[j] = 0.5 * (data_y[j] + data_y_i[j])
                        # data_e[j] = 0.5 * math.sqrt(data_e[j] * data_e[j] + data_e_i[j] * data_e_i[j])
                    else:
                        data_y[j] = data_y_i[j]
                        data_e[j] = data_e_i[j]

        # Skip the first point
        for i in range(len(data_y)):
            if data_y[i] > 0:
                data_y[i] = 0.0
                break

        # Scale to unity
        scale_to_unity = self.getProperty("ScaleToUnity").value
        specular_cutoff = self.getProperty("SpecularCutoff").value
        scaling_factor = 1.0
        if scale_to_unity is True:
            y_values = []
            e_values = []
            for i in range(len(data_y)):
                if data_y[i] > 0 and data_x[i] < specular_cutoff:
                    y_values.append(data_y[i])
                    e_values.append(data_e[i])

            # Compute the scaling factor to bring the specular ridge to 1
            total = 0.0
            weights = 0.0
            for i in range(len(y_values)):
                w = 1.0 / e_values[i] ** 2
                total += w * y_values[i]
                weights += w
            if weights > 0:
                scaling_factor = total / weights

        Scale(
            InputWorkspace=scaled_ws_list[0] + "_histo",
            OutputWorkspace=scaled_ws_list[0] + "_scaled",
            Factor=1.0 / scaling_factor,
            Operation="Multiply",
        )

        # Save the data
        file_path = self.getProperty("OutputFilename").value
        dq0 = self.getProperty("DQConstant").value
        dq_over_q = self.getProperty("DQSlope").value

        # Check whether we want to compute the Q resolution
        compute_dq = self.getProperty("ComputeDQ").value
        if compute_dq:
            # Calibrated constant term for the resolution
            if mtd[scaled_ws_list[0]].getInstrument().hasParameter("dq-constant"):
                dq0 = mtd[scaled_ws_list[0]].getInstrument().getNumberParameter("dq-constant")[0]

            _dq_over_q = self.compute_resolution(mtd[scaled_ws_list[0]])
            if _dq_over_q:
                dq_over_q = _dq_over_q

        meta_data = self.getProperty("MetaData").value

        data_x = mtd[scaled_ws_list[0] + "_scaled"].dataX(0)
        data_y = mtd[scaled_ws_list[0] + "_scaled"].dataY(0)
        data_e = mtd[scaled_ws_list[0] + "_scaled"].dataE(0)

        start_time = mtd[scaled_ws_list[0] + "_scaled"].getRun().getProperty("start_time").value
        experiment = mtd[scaled_ws_list[0] + "_scaled"].getRun().getProperty("experiment_identifier").value
        run_number = mtd[scaled_ws_list[0] + "_scaled"].getRun().getProperty("run_number").value
        run_title = mtd[scaled_ws_list[0] + "_scaled"].getTitle()

        content = "# Experiment %s Run %s\n" % (experiment, run_number)
        content += "# Run title: %s\n" % run_title
        content += "# Run start time: %s\n" % start_time
        content += "# Reduction time: %s\n" % time.ctime()
        content += "# Mantid version: %s\n" % mantid.__version__
        content += "# Scaling factor: %s\n" % scaling_factor
        content += header_info

        try:
            if len(meta_data.strip()) > 0:
                content += "#\n"
                lines = meta_data.strip().split("\n")
                for l in lines:
                    content += "# %s\n" % l
                content += "#\n"
        except:
            logger.error("Could not write meta-data to reflectivity file.")

        content += "# dQ0[1/Angstrom] = %g\n" % dq0
        content += "# dQ/Q = %g\n" % dq_over_q
        content += "# Q[1/Angstrom] R delta_R Precision\n"

        for i in range(len(data_x)):
            # Skip point where the error is larger than the reflectivity value
            if data_y[i] > data_e[i]:
                content += str(data_x[i])
                content += " " + str(data_y[i])
                content += " " + str(data_e[i])
                _precision = str(dq0 + dq_over_q * data_x[i])
                content += " " + _precision
                content += "\n"

        f = open(file_path, "w")
        f.write(content)
        f.close()

        for ws in scaled_ws_list:
            if AnalysisDataService.doesExist(ws + "_histo"):
                AnalysisDataService.remove(ws + "_histo")
            if AnalysisDataService.doesExist(ws + "_scaled"):
                AnalysisDataService.remove(ws + "_scaled")


AlgorithmFactory.subscribe(LRReflectivityOutput)
