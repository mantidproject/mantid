# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from mantid.simpleapi import SANSILLParameterScan
from os.path import basename


class ScanExplorerModel:

    def __init__(self, presenter=None):
        self.presenter = presenter

    def process_file(self, file):
        name = basename(file)[:-4] + "_scan"
        out = SANSILLParameterScan(SampleRuns=file, OutputWorkspace=name, NormaliseBy="None")
        self.presenter.create_slice_viewer(out)
