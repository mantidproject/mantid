# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from mantid.simpleapi import SANSILLParameterScan


class ScanExplorerModel:

    def __init__(self, presenter=None):
        self.presenter = presenter

    def process_files(self, files):
        out = SANSILLParameterScan(SampleRuns=files, OutputWorkspace="out", NormaliseBy="None")
        self.presenter.create_slice_viewer(out)
