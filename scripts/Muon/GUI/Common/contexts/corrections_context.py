# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.py36compat import dataclass, field


@dataclass
class CorrectionsContext:
    run_numbers: list = field(default_factory=list, init=[])
    current_run_number_index: int = None

    dead_time_source: str = None
    dead_time_table_name: str = None

    def number_of_run_numbers(self) -> int:
        """Returns the number of run numbers stored by the corrections context."""
        return len(self.run_numbers)
