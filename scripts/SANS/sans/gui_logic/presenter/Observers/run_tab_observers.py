# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.py36compat import dataclass
from mantidqt.utils.observer_pattern import GenericObserver


class SaveOptionsObserver(GenericObserver):
    pass


@dataclass
class RunTabObservers:
    save_options: SaveOptionsObserver
