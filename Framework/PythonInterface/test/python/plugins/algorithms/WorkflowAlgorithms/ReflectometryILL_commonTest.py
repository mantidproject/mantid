# -*- coding: utf-8 -*-# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

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
        instr_name = common.instrument_name(ws)
        slit2_width = run.get(common.slit_size_log_entry(instr_name, 1))
        slit3_width = run.get(common.slit_size_log_entry(instr_name, 2))
        common.slit_sizes(ws)
        self.assertEqual(slit2_width.value, run.getProperty(common.SampleLogs.SLIT2WIDTH).value)
        self.assertEqual(slit3_width.value, run.getProperty(common.SampleLogs.SLIT3WIDTH).value)
        mtd.clear()


if __name__ == "__main__":
    unittest.main()
