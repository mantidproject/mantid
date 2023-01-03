# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name

import mantid  # noqa
import ISISCommandInterface as i
import isis_reducer
import isis_instrument
import isis_reduction_steps
import SANS2DReductionGUI as sansgui

from ISIS.SANS.isis_sans_system_test import ISISSansSystemTest
from sans.common.enums import SANSInstrument


@ISISSansSystemTest(SANSInstrument.SANS2D)
class SANS2DGUISearchCentre(sansgui.SANS2DGUIReduction):
    def checkCentreResult(self):
        self.checkFloat(i.ReductionSingleton().get_beam_center("rear")[0], 0.15)
        self.checkFloat(i.ReductionSingleton().get_beam_center("rear")[1], -0.145)

    def runTest(self):
        self.singleModePrepare()

        i.FindBeamCentre(rlow=41, rupp=280, MaxIter=3, xstart=float(150) / 1000.0, ystart=float(-160) / 1000.0, tolerance=0.0001251)
        self.checkCentreResult()
        # clean up

        i.ReductionSingleton.clean(isis_reducer.ISISReducer)
        i.ReductionSingleton().set_instrument(isis_instrument.SANS2D())
        i.ReductionSingleton().user_settings = isis_reduction_steps.UserFile("MaskSANS2DReductionGUI.txt")
        i.ReductionSingleton().user_settings.execute(i.ReductionSingleton())

    def validate(self):
        # there is no workspace to be checked against
        return True


if __name__ == "__main__":
    test = SANS2DGUISearchCentre()
    test.execute()
