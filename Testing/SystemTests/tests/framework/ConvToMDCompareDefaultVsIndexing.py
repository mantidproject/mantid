# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
import sys
import systemtesting
from mantid.api import mtd
from mantid.simpleapi import BinMD, CompareMDWorkspaces, ConvertToMD, CreateSampleWorkspace


class ConvToMDCompareDefaultVsIndexing(systemtesting.MantidSystemTest):
    @staticmethod
    def name(params, postfix):
        name = ""
        for param in params:
            name = name + param + "_"
        return "{}_{}".format(name, postfix)

    @staticmethod
    def default_name(params):
        return ConvToMDCompareDefaultVsIndexing.name(params, "ws_default")

    @staticmethod
    def indexed_name(params):
        return ConvToMDCompareDefaultVsIndexing.name(params, "ws_indexed")

    test_functions = ["Multiple Peaks", "Exp Decay", "Flat background", "One Peak"]

    test_pix_width = ["10", "20"]

    def skipTests(self):
        return sys.platform.startswith("win")

    def runTest(self):
        for func in self.test_functions:
            for pw in self.test_pix_width:
                params = [func, pw]
                data_name = self.name(params, "data")
                CreateSampleWorkspace(
                    WorkspaceType="Event",
                    OutputWorkspace=data_name,
                    Function=func,
                    XMin="10000",
                    XMax="100000",
                    NumEvents="1000",
                    BankPixelWidth=pw,
                    Random=False,
                )

                ConvertToMD(
                    OutputWorkspace=self.indexed_name(params),
                    SplitInto="2",
                    SplitThreshold="10",
                    ConverterType="Indexed",
                    InputWorkspace=data_name,
                    QDimensions="Q3D",
                    dEAnalysisMode="Elastic",
                    Q3DFrames="Q_lab",
                )

                ConvertToMD(
                    OutputWorkspace=self.default_name(params),
                    SplitInto="2",
                    SplitThreshold="10",
                    ConverterType="Default",
                    InputWorkspace=data_name,
                    QDimensions="Q3D",
                    dEAnalysisMode="Elastic",
                    Q3DFrames="Q_lab",
                )

    def validate(self):
        out = True
        for func in self.test_functions:
            for pw in self.test_pix_width:
                params = [func, pw]
                md1 = mtd[self.indexed_name(params)]
                md2 = mtd[self.default_name(params)]
                min = []
                max = []
                for i in range(3):
                    min.append(md1.getDimension(0).getMinimum())
                    max.append(md1.getDimension(0).getMaximum())

                hmd1 = BinMD(
                    InputWorkspace=md1,
                    AxisAligned=True,
                    AlignedDim0="Q_lab_x,{},{},10000".format(min[0], max[0]),
                    AlignedDim1="Q_lab_y,{},{},100".format(min[1], max[1]),
                    AlignedDim2="Q_lab_z,{},{},10".format(min[2], max[2]),
                )

                hmd2 = BinMD(
                    InputWorkspace=md2,
                    AxisAligned=True,
                    AlignedDim0="Q_lab_x,{},{},10000".format(min[0], max[0]),
                    AlignedDim1="Q_lab_y,{},{},100".format(min[1], max[1]),
                    AlignedDim2="Q_lab_z,{},{},10".format(min[2], max[2]),
                )

                result = CompareMDWorkspaces(hmd1, hmd2, Tolerance="0.0001", IgnoreBoxID=True, CheckEvents=False)
                out = out and result.Equals
        return out
