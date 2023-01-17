# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import numpy as np
import math
from scipy.special import lambertw
from mantid import mtd
from mantid.api import AlgorithmFactory, PythonAlgorithm, FileAction, FileProperty, WorkspaceProperty, Progress
from mantid.kernel import Direction


class SaveP2D(PythonAlgorithm):
    def category(self):
        return "Diffraction\\DataHandling"

    def summary(self):
        """
        summary of the algorithm
        :return:
        """
        return "The algorithm used to create a multidimensional '.p2d' file from a 2D workspace."

    def name(self):
        return "SaveP2D"

    def seeAlso(self):
        return ["Bin2DPowderDiffraction"]

    def PyInit(self):
        # Input file
        self.declareProperty(WorkspaceProperty("Workspace", "", direction=Direction.Input), doc="Workspace that should be used.")
        # Output File
        self.declareProperty(
            FileProperty("OutputFile", "", action=FileAction.Save, direction=Direction.Input), doc='Output File for ".p2d" Data.'
        )
        # Manipulating Data ranges
        self.declareProperty("RemoveNaN", True, direction=Direction.Input, doc="Remove DataPoints with NaN as intensity value")
        self.declareProperty("RemoveNegatives", True, direction=Direction.Input, doc="Remove data points with negative intensity values")
        self.declareProperty(
            "CutData", False, direction=Direction.Input, doc="Use the following inputs to limit data in Theta, lambda, d and dp"
        )
        self.declareProperty("TthMin", 50, direction=Direction.Input, doc="Minimum for tth values")
        self.declareProperty("TthMax", 120, direction=Direction.Input, doc="Maximum  for tth values")
        self.declareProperty("LambdaMin", 0.3, direction=Direction.Input, doc="Minimum  for lambda values")
        self.declareProperty("LambdaMax", 1.1, direction=Direction.Input, doc="Maximum  for lambda values")
        self.declareProperty("DMin", 0.11, direction=Direction.Input, doc="Minimum  for d values")
        self.declareProperty("DMax", 1.37, direction=Direction.Input, doc="Maximum  for d values")
        self.declareProperty("DpMin", 0.48, direction=Direction.Input, doc="Minimum  for dp values")
        self.declareProperty("DpMax", 1.76, direction=Direction.Input, doc="Maximum  for dp values")

    def set_data_bounds(self):
        self.lambdaMin = self.getProperty("LambdaMin").value
        self.lambdaMax = self.getProperty("LambdaMax").value
        self.dMin = self.getProperty("DMin").value
        self.dMax = self.getProperty("DMax").value
        self.dpMin = self.getProperty("DpMin").value
        self.dpMax = self.getProperty("DpMax").value
        self.tthMin = self.getProperty("TthMin").value
        self.tthMax = self.getProperty("TthMax").value

    def check_data_ranges(self, d, dp, lhkl, thkl):
        return self.check_d_bounds(d) & self.check_dp_bounds(dp) & self.check_lambda_bounds(lhkl) & self.check_tth_bounds(thkl)

    def check_d_bounds(self, d):
        return self.dMin < d < self.dMax

    def check_dp_bounds(self, dp):
        return self.dpMin < dp < self.dpMax

    def check_lambda_bounds(self, lhkl):
        return self.lambdaMin < lhkl < self.lambdaMax

    def check_tth_bounds(self, thkl):
        return self.tthMin < thkl < self.tthMax

    # Create output file
    def PyExec(self):
        def wo_real(x):
            return np.real(lambertw(np.exp(x), 0))

        if self.getPropertyValue("CutData") == "1":
            self.set_data_bounds()

        # Workspace name
        Workspace = self.getPropertyValue("Workspace")

        # Create Output File
        OutFile = self.getPropertyValue("OutputFile") + ".p2d"
        # Load Workspace data
        Data = mtd[Workspace]
        # output style
        form = "{:12.7f}"
        lform = "{:12.7f}   {:12.7f}   {:12.7f}   {:12.7f}   {:12.7f}\n"

        with open(OutFile, "w") as of:
            print("Exporting: " + OutFile + "\n")
            # Create File header with additional information
            of.write("#Title: " + Data.getTitle() + "\n")
            of.write("#Inst: " + Data.getInstrument().getName() + ".prm\n")
            binning = form.format(Data.getDimension(0).getBinWidth()) + " " + form.format(Data.getDimension(1).getBinWidth()) + "\n"
            of.write("#Binning: ddperp" + binning)
            of.write("#Bank: 1\n")
            of.write("#2theta   lambda   d-value   dp-value   counts\n")
            # Get number of bins
            ndp = Data.getDimension(1).getNBins()
            # create progress reporter
            pr = Progress(None, start=0.0, end=1.0, nreports=ndp)
            last_dp = -1
            last_d = -1

            # iterate over all dPerpendicular cuts
            for cdp in range(ndp):
                dp = Data.getDimension(1).getX(cdp)
                # skip if no new dPerpendicular cut
                if dp == last_dp:
                    continue
                # create value of bin center from left and right border
                dp_center = (dp + Data.getDimension(1).getX(cdp + 1)) / 2.0
                last_dp = dp
                pr.report("Processing")
                print("{:4.0f}%".format(cdp * 100.0 / ndp))
                # iterate over all dSpacing values for the selected dPerpendicular value
                for cd in range(Data.getDimension(0).getNBins()):
                    d = Data.dataX(cdp)[cd]
                    # skip if d is the same as before
                    if d == last_d:
                        break
                    last_d = d
                    Y = Data.dataY(cdp)[cd]
                    # skip NaN values for intensity if option is activated
                    if self.getPropertyValue("RemoveNaN") == "1":
                        if math.isnan(Y):
                            continue
                    # skip intensities <= 0 if option is selected
                    if self.getPropertyValue("RemoveNegatives") == "1":
                        if Y <= 0:
                            continue
                    # calculate 2theta and lambda from d and dPerpendicular
                    dsq = d**2
                    lhkl = np.sqrt(4.0 * dsq - wo_real(4.0 * dsq - np.log(0.25 / dsq) - dp_center**2))
                    thkl = 2.0 * np.arcsin(lhkl / 2.0 / d) / np.pi * 180.0
                    # skip Values outside of specified data ranges if the option is activated
                    if self.getPropertyValue("CutData") == "1":
                        if self.check_data_ranges(d, dp_center, lhkl, thkl):
                            # write data into outfile after checking all data ranges positive
                            of.write(lform.format(thkl, lhkl, d, dp_center, Y))
                    else:
                        # Write data into outfile
                        of.write(lform.format(thkl, lhkl, d, dp_center, Y))
            print("\n\nExported: " + OutFile + "\n")


AlgorithmFactory.subscribe(SaveP2D)
