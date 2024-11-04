# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# Algorithm to start Bayes programs
from mantid.api import mtd, DataProcessorAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty, NumericAxis, Progress
from mantid.kernel import logger, Direction

import numpy as np


class SofQWMoments(DataProcessorAlgorithm):
    def category(self):
        return "Workflow\\MIDAS"

    def summary(self):
        return "Calculates the nth moment of y(q,w)"

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "", Direction.Input), doc="Input workspace to use.")
        self.declareProperty(name="EnergyMin", defaultValue=-0.5, doc="Minimum energy for fit. Default=-0.5")
        self.declareProperty(name="EnergyMax", defaultValue=0.5, doc="Maximum energy for fit. Default=0.5")
        self.declareProperty(name="Scale", defaultValue=1.0, doc="Scale factor to multiply y(Q,w). Default=1.0")
        self.declareProperty(
            MatrixWorkspaceProperty("OutputWorkspace", "", Direction.Output), doc="Workspace that includes all calculated moments."
        )

    def PyExec(self):
        workflow_prog = Progress(self, start=0.0, end=1.0, nreports=20)
        self._setup()

        workflow_prog.report("Validating input")
        input_workspace = mtd[self._input_ws]
        num_spectra, num_w = self._CheckHistZero(self._input_ws)
        logger.information("Sample %s has %d Q values & %d w values" % (self._input_ws, num_spectra, num_w))
        self._CheckElimits([self._energy_min, self._energy_max], self._input_ws)

        workflow_prog.report("Cropping Workspace")
        input_ws = "__temp_sqw_moments_cropped"
        crop_alg = self.createChildAlgorithm("CropWorkspace", enableLogging=False)
        crop_alg.setProperty("InputWorkspace", input_workspace)
        crop_alg.setProperty("XMin", self._energy_min)
        crop_alg.setProperty("XMax", self._energy_max)
        crop_alg.setProperty("OutputWorkspace", input_ws)
        crop_alg.execute()
        mtd.addOrReplace(input_ws, crop_alg.getProperty("OutputWorkspace").value)

        logger.information("Energy range is %f to %f" % (self._energy_min, self._energy_max))

        if self._factor > 0.0:
            workflow_prog.report("Scaling Workspace by factor %f" % self._factor)
            scale_alg = self.createChildAlgorithm("Scale", enableLogging=False)
            scale_alg.setProperty("InputWorkspace", input_ws)
            scale_alg.setProperty("Factor", self._factor)
            scale_alg.setProperty("Operation", "Multiply")
            scale_alg.setProperty("OutputWorkspace", input_ws)
            scale_alg.execute()
            logger.information("y(q,w) scaled by %f" % self._factor)

        # calculate delta x
        workflow_prog.report("Converting to point data")
        convert_point_alg = self.createChildAlgorithm("ConvertToPointData", enableLogging=False)
        convert_point_alg.setProperty("InputWorkspace", input_ws)
        convert_point_alg.setProperty("OutputWorkspace", input_ws)
        convert_point_alg.execute()
        mtd.addOrReplace(input_ws, convert_point_alg.getProperty("OutputWorkspace").value)
        x_data = np.asarray(mtd[input_ws].readX(0))
        workflow_prog.report("Creating temporary data workspace")
        x_workspace = "__temp_sqw_moments_x"
        create_alg = self.createChildAlgorithm("CreateWorkspace", enableLogging=False)
        create_alg.setProperty("DataX", x_data)
        create_alg.setProperty("DataY", x_data)
        create_alg.setProperty("UnitX", "DeltaE")
        create_alg.setProperty("OutputWorkspace", x_workspace)
        create_alg.execute()
        mtd.addOrReplace(x_workspace, create_alg.getProperty("OutputWorkspace").value)

        # calculate moments
        multiply_alg = self.createChildAlgorithm("Multiply", enableLogging=False)

        workflow_prog.report("Multiplying Workspaces by moments")
        moments_0 = self._output_ws + "_M0"
        moments_1 = self._output_ws + "_M1"
        multiply_alg.setProperty("LHSWorkspace", x_workspace)
        multiply_alg.setProperty("RHSWorkspace", input_ws)
        multiply_alg.setProperty("OutputWorkspace", moments_1)
        multiply_alg.execute()
        mtd.addOrReplace(moments_1, multiply_alg.getProperty("OutputWorkspace").value)

        moments_2 = self._output_ws + "_M2"
        multiply_alg.setProperty("LHSWorkspace", x_workspace)
        multiply_alg.setProperty("RHSWorkspace", moments_1)
        multiply_alg.setProperty("OutputWorkspace", moments_2)
        multiply_alg.execute()
        mtd.addOrReplace(moments_2, multiply_alg.getProperty("OutputWorkspace").value)

        moments_3 = self._output_ws + "_M3"
        multiply_alg.setProperty("LHSWorkspace", x_workspace)
        multiply_alg.setProperty("RHSWorkspace", moments_2)
        multiply_alg.setProperty("OutputWorkspace", moments_3)
        multiply_alg.execute()
        mtd.addOrReplace(moments_3, multiply_alg.getProperty("OutputWorkspace").value)

        moments_4 = self._output_ws + "_M4"
        multiply_alg.setProperty("LHSWorkspace", x_workspace)
        multiply_alg.setProperty("RHSWorkspace", moments_3)
        multiply_alg.setProperty("OutputWorkspace", moments_4)
        multiply_alg.execute()
        mtd.addOrReplace(moments_4, multiply_alg.getProperty("OutputWorkspace").value)

        workflow_prog.report("Converting to Histogram")
        convert_hist_alg = self.createChildAlgorithm("ConvertToHistogram", enableLogging=False)
        convert_hist_alg.setProperty("InputWorkspace", input_ws)
        convert_hist_alg.setProperty("OutputWorkspace", input_ws)
        convert_hist_alg.execute()

        workflow_prog.report("Integrating result")
        integration_alg = self.createChildAlgorithm("Integration", enableLogging=False)
        integration_alg.setProperty("InputWorkspace", convert_hist_alg.getProperty("OutputWorkspace").value)
        integration_alg.setProperty("OutputWorkspace", moments_0)
        integration_alg.execute()
        mtd.addOrReplace(moments_0, integration_alg.getProperty("OutputWorkspace").value)

        moments = [moments_1, moments_2, moments_3, moments_4]
        divide_alg = self.createChildAlgorithm("Divide", enableLogging=False)
        for moment_ws in moments:
            workflow_prog.report("Processing workspace %s" % moment_ws)
            convert_hist_alg.setProperty("InputWorkspace", moment_ws)
            convert_hist_alg.setProperty("OutputWorkspace", moment_ws)
            convert_hist_alg.execute()

            integration_alg.setProperty("InputWorkspace", convert_hist_alg.getProperty("OutputWorkspace").value)
            integration_alg.setProperty("OutputWorkspace", moment_ws)
            integration_alg.execute()

            divide_alg.setProperty("LHSWorkspace", integration_alg.getProperty("OutputWorkspace").value)
            divide_alg.setProperty("RHSWorkspace", moments_0)
            divide_alg.setProperty("OutputWorkspace", moment_ws)
            divide_alg.execute()
            mtd.addOrReplace(moment_ws, divide_alg.getProperty("OutputWorkspace").value)

        workflow_prog.report("Deleting Workspaces")
        delete_alg = self.createChildAlgorithm("DeleteWorkspace", enableLogging=False)
        delete_alg.setProperty("Workspace", input_ws)
        delete_alg.execute()
        delete_alg.setProperty("Workspace", x_workspace)
        delete_alg.execute()

        # create output workspace
        extensions = ["_M0", "_M1", "_M2", "_M3", "_M4"]
        transpose_alg = self.createChildAlgorithm("Transpose", enableLogging=False)
        convert_hist_alg = self.createChildAlgorithm("ConvertToHistogram", enableLogging=False)
        convert_units_alg = self.createChildAlgorithm("ConvertUnits", enableLogging=False)
        for ext in extensions:
            ws_name = self._output_ws + ext
            workflow_prog.report("Processing Workspace %s" % ext)
            transpose_alg.setProperty("InputWorkspace", ws_name)
            transpose_alg.setProperty("OutputWorkspace", ws_name)
            transpose_alg.execute()
            convert_hist_alg.setProperty("InputWorkspace", transpose_alg.getProperty("OutputWorkspace").value)
            convert_hist_alg.setProperty("OutputWorkspace", ws_name)
            convert_hist_alg.execute()
            convert_units_alg.setProperty("InputWorkspace", convert_hist_alg.getProperty("OutputWorkspace").value)
            convert_units_alg.setProperty("Target", "MomentumTransfer")
            convert_units_alg.setProperty("EMode", "Indirect")
            convert_units_alg.setProperty("OutputWorkspace", ws_name)
            convert_units_alg.execute()
            mtd.addOrReplace(ws_name, convert_units_alg.getProperty("OutputWorkspace").value)

            workflow_prog.report("Adding Sample logs to %s" % ws_name)
            copy_alg = self.createChildAlgorithm("CopyLogs", enableLogging=False)
            copy_alg.setProperty("InputWorkspace", input_workspace)
            copy_alg.setProperty("OutputWorkspace", ws_name)
            copy_alg.execute()
            add_sample_log_alg = self.createChildAlgorithm("AddSampleLog", enableLogging=False)
            add_sample_log_alg.setProperty("Workspace", ws_name)
            add_sample_log_alg.setProperty("LogName", "energy_min")
            add_sample_log_alg.setProperty("LogType", "Number")
            add_sample_log_alg.setProperty("LogText", str(self._energy_min))
            add_sample_log_alg.execute()
            add_sample_log_alg.setProperty("Workspace", ws_name)
            add_sample_log_alg.setProperty("LogName", "energy_max")
            add_sample_log_alg.setProperty("LogType", "Number")
            add_sample_log_alg.setProperty("LogText", str(self._energy_max))
            add_sample_log_alg.execute()
            add_sample_log_alg.setProperty("Workspace", ws_name)
            add_sample_log_alg.setProperty("LogName", "scale_factor")
            add_sample_log_alg.setProperty("LogType", "Number")
            add_sample_log_alg.setProperty("LogText", str(self._factor))
            add_sample_log_alg.execute()

        # Group output workspace
        workflow_prog.report("Appending moments")
        append_alg = self.createChildAlgorithm("AppendSpectra", enableLogging=False)
        append_alg.setProperty("InputWorkspace1", self._output_ws + "_M0")
        append_alg.setProperty("InputWorkspace2", self._output_ws + "_M1")
        append_alg.setProperty("ValidateInputs", False)
        append_alg.setProperty("OutputWorkspace", self._output_ws)
        append_alg.execute()
        append_alg.setProperty("InputWorkspace1", append_alg.getProperty("OutputWorkspace").value)
        append_alg.setProperty("InputWorkspace2", self._output_ws + "_M2")
        append_alg.setProperty("ValidateInputs", False)
        append_alg.setProperty("OutputWorkspace", self._output_ws)
        append_alg.execute()
        append_alg.setProperty("InputWorkspace1", append_alg.getProperty("OutputWorkspace").value)
        append_alg.setProperty("InputWorkspace2", self._output_ws + "_M3")
        append_alg.setProperty("ValidateInputs", False)
        append_alg.setProperty("OutputWorkspace", self._output_ws)
        append_alg.execute()
        append_alg.setProperty("InputWorkspace1", append_alg.getProperty("OutputWorkspace").value)
        append_alg.setProperty("InputWorkspace2", self._output_ws + "_M4")
        append_alg.setProperty("ValidateInputs", False)
        append_alg.setProperty("OutputWorkspace", self._output_ws)
        append_alg.execute()
        mtd.addOrReplace(self._output_ws, append_alg.getProperty("OutputWorkspace").value)
        delete_alg.setProperty("Workspace", self._output_ws + "_M0")
        delete_alg.execute()
        delete_alg.setProperty("Workspace", self._output_ws + "_M1")
        delete_alg.execute()
        delete_alg.setProperty("Workspace", self._output_ws + "_M2")
        delete_alg.execute()
        delete_alg.setProperty("Workspace", self._output_ws + "_M3")
        delete_alg.execute()
        delete_alg.setProperty("Workspace", self._output_ws + "_M4")
        delete_alg.execute()

        # Create a new vertical axis for the Q and Q**2 workspaces
        y_axis = NumericAxis.create(5)
        for idx in range(5):
            y_axis.setValue(idx, idx)
        mtd[self._output_ws].replaceAxis(1, y_axis)

        self.setProperty("OutputWorkspace", self._output_ws)

    def _setup(self):
        """
        Gets algorithm properties.
        """

        self._input_ws = self.getPropertyValue("InputWorkspace")
        self._factor = self.getProperty("Scale").value
        self._energy_min = self.getProperty("EnergyMin").value
        self._energy_max = self.getProperty("EnergyMax").value
        self._output_ws = self.getPropertyValue("OutputWorkspace")

    def _CheckHistZero(self, ws):
        """
        Retrieves basic info on a workspace

        Checks the workspace is not empty, then returns the number of histogram and
        the number of X-points, which is the number of bin boundaries minus one

        Args:
          @param ws  2D workspace

        Returns:
          @return num_hist - number of histograms in the workspace
          @return ntc - number of X-points in the first histogram, which is the number of bin
           boundaries minus one. It is assumed all histograms have the same
           number of X-points.

        Raises:
          @exception ValueError - Workspace has no histograms
        """
        num_hist = mtd[ws].getNumberHistograms()  # no. of hist/groups in WS
        if num_hist == 0:
            raise ValueError("Workspace %s has NO histograms" % ws)
        x_in = mtd[ws].readX(0)
        ntc = len(x_in) - 1  # no. points from length of x array
        if ntc == 0:
            raise ValueError("Workspace %s has NO points" % ws)
        return num_hist, ntc

    def _CheckElimits(self, erange, ws):
        import math

        x_data = np.asarray(mtd[ws].readX(0))
        len_x = len(x_data) - 1

        if math.fabs(erange[0]) < 1e-5:
            raise ValueError("Elimits - input emin (%f) is Zero" % (erange[0]))
        if erange[0] < x_data[0]:
            raise ValueError("Elimits - input emin (%f) < data emin (%f)" % (erange[0], x_data[0]))
        if math.fabs(erange[1]) < 1e-5:
            raise ValueError("Elimits - input emax (%f) is Zero" % (erange[1]))
        if erange[1] > x_data[len_x]:
            raise ValueError("Elimits - input emax (%f) > data emax (%f)" % (erange[1], x_data[len_x]))
        if erange[1] < erange[0]:
            raise ValueError("Elimits - input emax (%f) < emin (%f)" % (erange[1], erange[0]))


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SofQWMoments)
