# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init

import systemtesting
from mantid.simpleapi import *


class ConvToMDCompareDefaultVsIndexung(systemtesting.MantidSystemTest):
    @staticmethod
    def indexed_name(func):
        return '{}_ws_indexed'.format(func)

    @staticmethod
    def default_name(func):
        return '{}_ws_default'.format(func)

    test_functions = ['Multiple Peaks',
                      'Exp Decay',
                      'Flat background',
                      'Multiple Peaks',
                      'One Peak']

    def runTest(self):
        for func in self.test_functions:
            data_name = '{}_data'.format(func)
            CreateSampleWorkspace(WorkspaceType='Event', OutputWorkspace=data_name, Function='Multiple Peaks',
                                  XMin='10000', XMax='100000', NumEvents='1000', Random=False)

            ConvertToMD(OutputWorkspace=self.indexed_name(func), SplitInto='2', SplitThreshold='10',
                        ConverterType='Indexed', InputWorkspace=data_name, QDimensions='Q3D', dEAnalysisMode='Elastic',
                        Q3DFrames='Q_lab')

            ConvertToMD(OutputWorkspace=self.default_name(func), SplitInto='2', SplitThreshold='10',
                        ConverterType='Default', InputWorkspace=data_name, QDimensions='Q3D', dEAnalysisMode='Elastic',
                        Q3DFrames='Q_lab')

    def validate(self):
        out = True
        for func in self.test_functions:
            md1 = mtd[self.indexed_name(func)]
            md2 = mtd[self.default_name(func)]
            min = []
            max = []
            for i in range(3):
                min.append(md1.getDimension(0).getMinimum())
                max.append(md1.getDimension(0).getMaximum())

            hmd1 = BinMD(InputWorkspace=md1, AxisAligned=True, AlignedDim0='Q_lab_x,{},{},10000'.format(min[0], max[0]),
                         AlignedDim1='Q_lab_y,{},{},100'.format(min[1], max[1]),
                         AlignedDim2='Q_lab_z,{},{},10'.format(min[2], max[2]))

            hmd2 = BinMD(InputWorkspace=md2, AxisAligned=True, AlignedDim0='Q_lab_x,{},{},10000'.format(min[0], max[0]),
                         AlignedDim1='Q_lab_y,{},{},100'.format(min[1], max[1]),
                         AlignedDim2='Q_lab_z,{},{},10'.format(min[2], max[2]))

            result = CompareMDWorkspaces(hmd1, hmd2, Tolerance='0.0001', IgnoreBoxID=True, CheckEvents=False)
            out = out and result.Equals
        return out
