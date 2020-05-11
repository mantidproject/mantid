# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from mantid.simpleapi import Load, CreateMDWorkspace
from mantidqt.widgets.samplelogs.model import SampleLogsModel

import unittest


class SampleLogsModelTest(unittest.TestCase):

    def test_model(self):
        ws = Load('ENGINX00228061')
        model = SampleLogsModel(ws)

        self.assertEqual(model.get_exp(), 0)
        self.assertEqual(model.get_name(), 'ws')
        self.assertEqual(model.getNumExperimentInfo(), 0)

        log = model.get_log("w")
        self.assertEqual(log.name, "w")
        self.assertEqual(log.size(), 4)

        log_names = model.get_log_names()
        self.assertEqual(len(log_names), 58)
        self.assertIn("w", log_names)

        values = model.get_log_display_values("w")
        self.assertEqual(values[0], "w")
        self.assertEqual(values[1], "float series")
        self.assertEqual(values[2], "(4 entries)")
        self.assertEqual(values[3], "degrees")

        self.assertTrue(model.is_log_plottable("w"))
        self.assertFalse(model.is_log_plottable("dur"))
        self.assertTrue(model.are_any_logs_plottable())

        stats = model.get_statistics("w")
        self.assertAlmostEqual(0.00491808, stats.maximum, 6)
        self.assertFalse(model.isMD())

        itemModel = model.getItemModel()
        self.assertEqual(itemModel.horizontalHeaderItem(0).text(), "Name")
        self.assertEqual(itemModel.horizontalHeaderItem(1).text(), "Type")
        self.assertEqual(itemModel.horizontalHeaderItem(2).text(), "Value")
        self.assertEqual(itemModel.horizontalHeaderItem(3).text(), "Units")
        self.assertEqual(itemModel.rowCount(), 58)
        self.assertEqual(itemModel.item(0,0).text(), "C6_MASTER_FREQUENCY")
        self.assertEqual(itemModel.item(0,1).text(), "float series")
        self.assertEqual(itemModel.item(0,2).text(), "50.0 (2 entries)")
        self.assertEqual(itemModel.item(0,3).text(), "Hz")

    def test_model_no_time_series_logs(self):
        #test with some reactor based data without time series logs
        ws = Load('ILL/D22/192068.nxs')
        model = SampleLogsModel(ws)

        log_names = model.get_log_names()
        self.assertEqual(len(log_names), 293)
        self.assertIn("Beam.sample_pressure", log_names)

        values = model.get_log_display_values("Beam.sample_pressure")
        self.assertEqual(values[0], "Beam.sample_pressure")
        self.assertEqual(values[1], "number")
        self.assertEqual(values[2], 0.0)
        self.assertEqual(values[3], "Pa")

        self.assertFalse(model.is_log_plottable("Beam.sample_pressure"))
        self.assertFalse(model.are_any_logs_plottable())

    def test_model_with_filtered_data(self):
        wsTuple = Load('POLREF00014966.nxs')
        #get the first child workspace of the group
        ws = wsTuple[0]
        model = SampleLogsModel(ws)

        log_names = model.get_log_names()
        self.assertEqual(len(log_names), 152)
        self.assertIn("raw_uah_log", log_names)
        self.assertIn("current_period", log_names)
        self.assertIn("period", log_names)

        values = model.get_log_display_values("raw_uah_log")
        self.assertEqual(values[0], "raw_uah_log")
        self.assertEqual(values[1], "float series")
        self.assertEqual(values[2], "(429 entries)")
        self.assertEqual(values[3], "uAh")

        self.assertTrue(model.is_log_plottable("raw_uah_log"))
        self.assertFalse(model.is_log_plottable("current_period"))
        self.assertTrue(model.is_log_plottable("period"))

        self.assertTrue(model.get_is_log_filtered("raw_uah_log"))
        self.assertFalse(model.get_is_log_filtered("period"))
        self.assertFalse(model.get_is_log_filtered("current_period"))

        stats = model.get_statistics("raw_uah_log", filtered = True)
        self.assertAlmostEqual(stats.maximum, 193.333,3)
        stats = model.get_statistics("raw_uah_log", filtered = False)
        self.assertAlmostEqual(stats.maximum, 194.296,3)

        self.assertFalse(model.isMD())

    def test_model_MD(self):
        ws1 = Load("ILL/D22/192068.nxs")
        ws2 = Load("ENGINX00228061")
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
        self.assertEqual(values[2], 120.0)
        self.assertEqual(values[3], "Sec")

        # Change exp
        model.set_exp(1)
        self.assertEqual(model.get_exp(), 1)
        values = model.get_log_display_values("dur")
        self.assertEqual(values[0], "dur")
        self.assertEqual(values[1], "number")
        self.assertEqual(values[2], 12)
        self.assertEqual(values[3], "")


if __name__ == '__main__':
    unittest.main()
