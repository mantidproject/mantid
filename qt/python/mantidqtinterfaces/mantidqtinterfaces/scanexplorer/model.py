# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from mantid.simpleapi import SANSILLParameterScan
from pathlib import Path


class ScanExplorerModel:

    def __init__(self, presenter=None):
        self.presenter = presenter

    def process_file(self, file_name: str):
        """
        Process the given file without any special treatment, as a quick representation of the data.
        @param file_name: the name of the file to process, containing a scan with more than one point.
        """
        name = Path(file_name).stem + "_scan"
        out = SANSILLParameterScan(SampleRun=file_name, OutputWorkspace=name, NormaliseBy="None")
        self.presenter.create_slice_viewer(out)
