# -*- coding: utf-8 -*-
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)

from mantid.simpleapi import mtd
from testhelpers import (assertRaisesNothing, create_algorithm, illhelpers)
import unittest
import ReflectometryILL_common as common


class ReflectometryILL_commonTest(unittest.TestCase):

    def tearDown(self):
        mtd.clear()

    def testD17SlitWidthLogEntry(self):
        ws = illhelpers.create_poor_mans_d17_workspace()
        mtd.add('ws', ws)
        illhelpers.add_slit_configuration_D17(ws, 0.03, 0.02)
        run = ws.run()
        instrName = common.instrumentName(ws)
        slit2width = run.get(common.slitSizeLogEntry(instrName, 1))
        slit3width = run.get(common.slitSizeLogEntry(instrName, 2))
        common.slitSizes(ws)
        self.assertEquals(slit2width.value, run.getProperty(common.SampleLogs.SLIT2WIDTH).value)
        self.assertEquals(slit3width.value, run.getProperty(common.SampleLogs.SLIT3WIDTH).value)
        mtd.clear()


if __name__ == "__main__":
    unittest.main()