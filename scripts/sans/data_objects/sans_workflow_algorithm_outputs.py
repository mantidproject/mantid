# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
from mantid.py36compat import dataclass, field
from typing import List
from mantid.dataobjects import Workspace2D


@dataclass
class SANSWorkflowAlgorithmOutputs:
    lab_output: List[Workspace2D] = field(default_factory=list)
    hab_output: List[Workspace2D] = field(default_factory=list)
    scaled_hab_output: List[Workspace2D] = field(default_factory=list)
    merged_output: List[Workspace2D] = field(default_factory=list)
