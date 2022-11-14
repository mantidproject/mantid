# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from enum import Enum

from mantid.api import PreviewType
from mantid.kernel import logger


class AcquisitionType(Enum):
    """
    Different acquisition types that can be found.
    """
    DEFAULT = 0
    MONO = 1
    TOF = 2
    SCAN = 3


class PreviewFinder:
    """
    Utility class to determine which preview should be used in which case.
    """

    """
    Officially supported instruments
    """
    D11 = "D11"
    D11lr = "D11lr"
    D11B = "D11B"
    D16 = "D16"
    D22 = "D22"
    D22B = "D22B"
    D33 = "D33"
    PANTHER = "PANTHER"
    SHARP = "SHARP"
    IN5 = "IN5"
    IN4 = "IN4"
    IN6 = "IN6"
    D17 = "D17"
    FIGARO = "FIGARO"
    IN16B = "IN16B"
    D1B = "D1B"
    D2B = "D2B"
    D20 = "D20"
    D7 = "D7"
    D9 = "D9"
    D10 = "D10"
    D19 = "D19"

    """
    Dictionary linking each instrument to its preview(s), depending on the acquisition mode
    """
    prev = {D11:     {AcquisitionType.DEFAULT: PreviewType.IVIEW},
            D11lr:   {AcquisitionType.DEFAULT: PreviewType.IVIEW},
            D11B:    {AcquisitionType.DEFAULT: PreviewType.IVIEW},
            D16:     {AcquisitionType.DEFAULT: PreviewType.IVIEW},
            D22:     {AcquisitionType.DEFAULT: PreviewType.IVIEW},
            D22B:    {AcquisitionType.DEFAULT: PreviewType.IVIEW},
            D33:     {AcquisitionType.DEFAULT: PreviewType.IVIEW},
            PANTHER: {AcquisitionType.DEFAULT: PreviewType.IVIEW},
            SHARP:   {AcquisitionType.DEFAULT: PreviewType.IVIEW},
            IN5:     {AcquisitionType.DEFAULT: PreviewType.IVIEW},
            IN4:     {AcquisitionType.DEFAULT: PreviewType.IVIEW},
            IN6:     {AcquisitionType.DEFAULT: PreviewType.IVIEW},
            D17:     {AcquisitionType.DEFAULT: PreviewType.PLOT2D},
            FIGARO:  {AcquisitionType.DEFAULT: PreviewType.PLOT2D},
            IN16B:   {AcquisitionType.DEFAULT: PreviewType.PLOT2D},
            D1B:     {AcquisitionType.DEFAULT: PreviewType.PLOT1D},
            D2B:     {AcquisitionType.DEFAULT: PreviewType.SVIEW},
            D20:     {AcquisitionType.MONO: PreviewType.PLOT1D, AcquisitionType.SCAN: PreviewType.PLOT2D},
            D7:      {AcquisitionType.MONO: PreviewType.PLOT1D, AcquisitionType.TOF: PreviewType.PLOT2D},
            D9:      {AcquisitionType.DEFAULT: PreviewType.SVIEW},
            D10:     {AcquisitionType.DEFAULT: PreviewType.SVIEW},
            D19:     {AcquisitionType.DEFAULT: PreviewType.SVIEW}
            }

    def get_preview(self, instrument_name: str, acquisition_mode: AcquisitionType = AcquisitionType.DEFAULT,
                    is_group: bool = False) -> PreviewType:
        """
        Get the preview associated with an instrument with a given acquisition mode.
        @param instrument_name: the name of instrument to look for
        @param acquisition_mode: the acquisition mode used
        @param is_group: whether the data comes from a group. Used to determine default.
        @return the preview to use
        """
        if instrument_name in self.prev:
            if acquisition_mode in self.prev[instrument_name]:
                return self.prev[instrument_name][acquisition_mode]

        logger.warning("Could not find appropriate preview for data. Resorting to default.")
        if is_group:
            # default to 1D plot
            return PreviewType.PLOT1D
        else:
            # default to instrument view and hope for the best
            return PreviewType.IVIEW

    def need_acquisition_mode(self, instrument_name: str):
        """
        Checks whether an instrument needs an acquisition mode to determine the preview to use
        @param instrument_name: the name of the instrument to look for
        """
        if instrument_name in self.prev.keys():
            return any(acquisition is not AcquisitionType.DEFAULT for acquisition in self.prev[instrument_name].keys())
        return False
