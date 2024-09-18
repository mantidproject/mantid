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
from mantidqt.widgets.samplelogs.model import SampleLogsModel, get_value

import unittest


class SampleLogsModelTest(unittest.TestCase):
    def test_model(self):
        ws = Load("ENGINX00228061")
        model = SampleLogsModel(ws)

        self.assertEqual(model.get_exp(), 0)
        self.assertEqual(model.get_name(), "ws")
        self.assertEqual(model.getNumExperimentInfo(), 0)

        log = model.get_log("w")
        self.assertEqual(log.name, "w")
        self.assertEqual(log.size(), 4)

        log_names = model.get_log_names()
        self.assertEqual(len(log_names), 46)
        self.assertIn("w", log_names)

        values = model.get_log_display_values("w")
        self.assertEqual(values[0], "w")
        self.assertEqual(values[1], "float series")
        self.assertEqual(values[2], "(4 entries)")
        self.assertEqual(values[3], "degrees")

        self.assertTrue(model.is_log_plottable("w"))
        self.assertFalse(model.is_log_plottable("goodfrm"))
        self.assertTrue(model.are_any_logs_plottable())

        stats = model.get_statistics("w")
        self.assertAlmostEqual(0.00491808, stats.maximum, 6)
        self.assertFalse(model.isMD())

        itemModel = model.getItemModel()
        self.assertEqual(itemModel.horizontalHeaderItem(0).text(), "Name")
        self.assertEqual(itemModel.horizontalHeaderItem(1).text(), "Type")
        self.assertEqual(itemModel.horizontalHeaderItem(2).text(), "Value")
        self.assertEqual(itemModel.horizontalHeaderItem(3).text(), "Units")
        self.assertEqual(itemModel.rowCount(), 46)
        self.assertEqual(itemModel.item(0, 0).text(), "C6_MASTER_FREQUENCY")
        self.assertEqual(itemModel.item(0, 1).text(), "float series")
        self.assertEqual(itemModel.item(0, 2).text(), "50.0 (2 entries)")
        self.assertEqual(itemModel.item(0, 3).text(), "Hz")

    def test_model_no_time_series_logs(self):
        # test with some reactor based data without time series logs
        ws = Load("ILL/D22/192068.nxs")
        model = SampleLogsModel(ws)

        log_names = model.get_log_names()
        self.assertEqual(len(log_names), 290)
        self.assertIn("Beam.sample_pressure", log_names)

        values = model.get_log_display_values("Beam.sample_pressure")
        self.assertEqual(values[0], "Beam.sample_pressure")
        self.assertEqual(values[1], "number")
        self.assertEqual(values[2], "0.0")
        self.assertEqual(values[3], "Pa")

        self.assertFalse(model.is_log_plottable("Beam.sample_pressure"))
        self.assertFalse(model.are_any_logs_plottable())

    def test_model_with_filtered_data(self):
        wsTuple = Load("POLREF00014966.nxs")
        # get the first child workspace of the group
        ws = wsTuple[0]
        model = SampleLogsModel(ws)

        log_names = model.get_log_names()
        self.assertEqual(len(log_names), 140)
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

        stats = model.get_statistics("raw_uah_log", filtered=True)
        self.assertAlmostEqual(stats.maximum, 193.333, 3)
        stats = model.get_statistics("raw_uah_log", filtered=False)
        self.assertAlmostEqual(stats.maximum, 194.296, 3)

        self.assertFalse(model.isMD())

    def test_model_MD(self):
        ws1 = Load("ILL/D22/192068.nxs")
        ws2 = Load("ENGINX00228061")
        md = CreateMDWorkspace(Dimensions=1, Extents="-1,1", Names="A", Units="U")
        md.addExperimentInfo(ws1)
        md.addExperimentInfo(ws2)
        model = SampleLogsModel(md)

        self.assertEqual(model.get_exp(), 0)
        self.assertEqual(model.get_name(), "md")
        self.assertEqual(model.getNumExperimentInfo(), 2)

        values = model.get_log_display_values("duration")
        self.assertEqual(values[0], "duration")
        self.assertEqual(values[1], "number")
        self.assertEqual(values[2], "120.0")
        self.assertEqual(values[3], "Sec")

        # Change exp
        model.set_exp(1)
        self.assertEqual(model.get_exp(), 1)
        values = model.get_log_display_values("rb_proposal")
        self.assertEqual(values[0], "rb_proposal")
        self.assertEqual(values[1], "number")
        self.assertEqual(values[2], "1455001")
        self.assertEqual(values[3], "")

    def test_Invalid_data_logs(self):
        ws = Load("ENGINX00228061_log_alarm_data.nxs")

        model = SampleLogsModel(ws)
        log_names = model.get_log_names()
        self.assertEqual(len(log_names), 48)
        invalid_logs = model.get_logs_with_invalid_data()
        self.assertEqual(2, len(invalid_logs.keys()))
        self.assertIn("cryo_temp1", invalid_logs.keys())
        self.assertEqual(1, invalid_logs["cryo_temp1"])
        self.assertIn("cryo_temp2", invalid_logs.keys())
        self.assertEqual(-1, invalid_logs["cryo_temp2"])

        hidden_logs = model.get_hidden_logs()
        self.assertEqual(2, len(hidden_logs))
        self.assertIn("cryo_temp1_invalid_values", hidden_logs)
        self.assertIn("cryo_temp2_invalid_values", hidden_logs)

    def test_get_value_for_filtered(self):
        # Checks that table values and plot log stats agree, even when filtered.
        ws = Load("ENGINX00228061_log_alarm_data.nxs")

        run = ws.getRun()
        all_logs = run.getLogData()
        model = SampleLogsModel(ws)

        # Partially invalid log with one filtered entry
        self.assertEqual(get_value(all_logs[31]), "{} (1 entry)".format(model.get_statistics("cryo_temp1").mean))
        self.assertEqual(get_value(all_logs[31]), "{} (1 entry)".format(model.get_statistics("cryo_temp1").maximum))

        # Fully invalid log with multiple entries (not affected by filtering)
        self.assertEqual(get_value(all_logs[33]), "({} entries)".format(all_logs[32].size()))  # cryo_temp2

        # Valid log with one entry filtered by status, with a differently valued entry unfiltered
        self.assertEqual(get_value(all_logs[16]), "{} (1 entry)".format(model.get_statistics("C6_SLAVE_FREQUENCY").mean))
        self.assertEqual(get_value(all_logs[16]), "{} (1 entry)".format(model.get_statistics("C6_SLAVE_FREQUENCY").maximum))

        # Valid log with one entry filtered by status, with another same valued entries unfiltered
        self.assertEqual(get_value(all_logs[25]), "{} (1 entry)".format(model.get_statistics("SECI_OUT_OF_RANGE_BLOCK").mean))
        self.assertEqual(get_value(all_logs[25]), "{} (1 entry)".format(model.get_statistics("SECI_OUT_OF_RANGE_BLOCK").maximum))

        # Valid log with 2 identical value entries
        self.assertEqual(get_value(all_logs[21]), "{} (2 entries)".format(model.get_statistics("C9_SLAVE_PHASE").mean))
        self.assertEqual(get_value(all_logs[21]), "{} (2 entries)".format(model.get_statistics("C9_SLAVE_PHASE").maximum))

        # Valid log with 6 identical value entries
        self.assertEqual(get_value(all_logs[38]), "{} (6 entries)".format(model.get_statistics("x").mean))
        self.assertEqual(get_value(all_logs[38]), "{} (6 entries)".format(model.get_statistics("x").maximum))

        # Valid log with multiple different value entries
        self.assertEqual(get_value(all_logs[29]), "({} entries)".format(all_logs[29].size()))  # cryo_Sample

    def test_get_value_for_unfiltered(self):
        # Checks that filtered_value works for unfiltered logs, e.g. at SNS

        ws_T = Load("Training_Exercise3a_SNS.nxs")
        ws_C = Load("CNCS_7860_event.nxs")  # Has no single entries

        run_T = ws_T.getRun()
        all_logs_T = run_T.getLogData()
        model_T = SampleLogsModel(ws_T)

        run_C = ws_C.getRun()
        all_logs_C = run_C.getLogData()
        model_C = SampleLogsModel(ws_C)

        # Valid log with one entry
        self.assertEqual(get_value(all_logs_T[2]), "{} (1 entry)".format(model_T.get_statistics("ChopperStatus1").mean))
        self.assertEqual(get_value(all_logs_T[2]), "{} (1 entry)".format(model_T.get_statistics("ChopperStatus1").maximum))

        # Valid log with 2 identical value entries
        self.assertEqual(get_value(all_logs_C[2]), "{} (2 entries)".format(model_C.get_statistics("ChopperStatus1").mean))
        self.assertEqual(get_value(all_logs_C[2]), "{} (2 entries)".format(model_C.get_statistics("ChopperStatus1").maximum))

        # Valid log with multiple different value entries
        self.assertEqual(get_value(all_logs_C[13]), "({} entries)".format(all_logs_C[13].size()))  # Phase2

    def test_filter_logs_with_search_key(self):
        # Checks that the logs are filtered correctly
        ws = Load("ILL/D22/192068.nxs")
        model = SampleLogsModel(ws).getItemModel("flipper")
        # check if the model contains the expected number of logs
        self.assertEqual(model.rowCount(), 8)


if __name__ == "__main__":
    unittest.main()
