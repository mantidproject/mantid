# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from __future__ import (absolute_import, division, print_function)

from mantid.simpleapi import LoadEventNexus, CreateMDWorkspace
from mantidqt.widgets.samplelogs.model import SampleLogsModel

import unittest


class SampleLogsModelTest(unittest.TestCase):

    def test_model(self):
        ws = LoadEventNexus('CNCS_7860', MetaDataOnly=True)
        model = SampleLogsModel(ws)

        self.assertEqual(model.get_exp(), 0)
        self.assertEqual(model.get_name(), 'ws')
        self.assertEqual(model.getNumExperimentInfo(), 0)

        log = model.get_log("Speed5")
        self.assertEqual(log.name, "Speed5")
        self.assertEqual(log.size(), 4)

        log_names = model.get_log_names()
        self.assertEqual(len(log_names), 48)
        self.assertIn("Speed5", log_names)

        values = model.get_log_display_values("Speed5")
        self.assertEqual(values[0], "Speed5")
        self.assertEqual(values[1], "float series")
        self.assertEqual(values[2], "(4 entries)")
        self.assertEqual(values[3], "Hz")

        self.assertTrue(model.is_log_plottable("Speed5"))
        self.assertFalse(model.is_log_plottable("duration"))

        stats = model.get_statistics("Speed5")
        self.assertEqual(stats.maximum, 300.0)

        self.assertFalse(model.isMD())

        itemModel = model.getItemModel()
        self.assertEqual(itemModel.horizontalHeaderItem(0).text(), "Name")
        self.assertEqual(itemModel.horizontalHeaderItem(1).text(), "Type")
        self.assertEqual(itemModel.horizontalHeaderItem(2).text(), "Value")
        self.assertEqual(itemModel.horizontalHeaderItem(3).text(), "Units")
        self.assertEqual(itemModel.rowCount(), 48)
        self.assertEqual(itemModel.item(0,0).text(), "ChopperStatus1")
        self.assertEqual(itemModel.item(0,1).text(), "float series")
        self.assertEqual(itemModel.item(0,2).text(), "4.0 (2 entries)")
        self.assertEqual(itemModel.item(0,3).text(), "")

    def test_model_MD(self):
        ws1 = LoadEventNexus("CNCS_7860", MetaDataOnly=True)
        ws2 = LoadEventNexus("VIS_19351", MetaDataOnly=True)
        md = CreateMDWorkspace(Dimensions=1, Extents='-1,1', Names='A', Units='U')
        md.addExperimentInfo(ws1)
        md.addExperimentInfo(ws2)
        model = SampleLogsModel(md)

        self.assertEqual(model.get_exp(), 0)
        self.assertEqual(model.get_name(), 'md')
        self.assertEqual(model.getNumExperimentInfo(), 2)

        values = model.get_log_display_values("duration")
        self.assertEqual(values[0], "duration")
        self.assertEqual(values[1], "number")
        self.assertEqual(values[2], 148.0)
        self.assertEqual(values[3], "second")

        # Change exp
        model.set_exp(1)
        self.assertEqual(model.get_exp(), 1)
        values = model.get_log_display_values("duration")
        self.assertEqual(values[0], "duration")
        self.assertEqual(values[1], "number")
        self.assertEqual(values[2], 4.616606712341309)
        self.assertEqual(values[3], "second")


if __name__ == '__main__':
    unittest.main()
