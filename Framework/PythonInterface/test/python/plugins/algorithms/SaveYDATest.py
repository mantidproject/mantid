from __future__ import (absolute_import, division, print_function)

import mantid
from mantid.api import mtd
from mantid.simpleapi import CreateWorkspace, CreateSampleWorkspace, SaveYDA, ConvertSpectrumAxis, \
    LoadInstrument, AddSampleLog
import numpy as np
import os
import unittest


class SaveYDATest(unittest.TestCase):

    def setUp(self):

        self.prop_num = 3
        self.prop_title = "PropTitle"
        self.exp_team = "Experiment Team"
        self.temperature = 100.0
        self.Ei = 1.0
        self.data_x = range(1, 5)
        self.data_y = [2.0, 3.0, 4.0]
        self._n_ws = self._create_workspace()
        self._n_file = self._file(self._n_ws, "File")
        ws = self._create_workspace(sample=False)
        self._no_sample_file = self._file(ws, "noSampleFile")

    def cleanup(self, ws_name, filename):

        if os.path.exists(filename):
            os.remove(filename)
        if mantid.mtd.doesExist(ws_name):
            mantid.api.AnalysisDataService.remove(ws_name)

    def test_meta_data(self):
        """ Test to save Meta data from workspace with all sample logs needed by SaveYDA
        """
        meta = []
        # read form file
        for i in range(3):
            meta.append(self._n_file.readline())
        # verify values
        self.assertEqual(meta[0], "Meta:\n")
        self.assertEqual(meta[1], "  format: yaml/frida 2.0\n")
        self.assertEqual(meta[2], "  type: generic tabular data\n")

    def test_history_all_samples(self):
        """ Test to save history from workspace with all sample logs
        """
        history = []
        for i in range(0, 8):
            s = self._n_file.readline()
            if i >= 3:
                history.append(s)

        self.assertEqual(history[0], "History:\n")
        self.assertEqual(history[1], "  - Proposal number " + str(self.prop_num) + "\n")
        self.assertEqual(history[2], "  - " + self.prop_title + "\n")
        self.assertEqual(history[3], "  - " + self.exp_team + "\n")
        self.assertEqual(history[4], "  - data reduced with mantid\n")

    def test_history_no_samples(self):
        """ Test save history from workspace without needed sample logs
        """
        history = []
        for i in range(0, 5):
            s = self._no_sample_file.readline()
            if i >= 3:
                history.append(s)

        self.assertEqual(history[0], "History:\n")
        self.assertEqual(history[1], "  - data reduced with mantid\n")

    def test_coord(self):
        """ Test save coordinates from workspace with all sample logs
        """
        coord = []
        # Y axis is SpectrumAxis
        for i in range(0, 12):
            s = self._n_file.readline()
            if i >= 8:
                coord.append(s)

        self.assertEqual(coord[0], "Coord:\n")
        self.assertEqual(coord[1], "  x: {name: w, unit: meV}\n")
        self.assertEqual(coord[2], "  y: {name: \'S(q,w)\', unit: meV-1}\n")
        self.assertEqual(coord[3], "  z: [{name: 2th, unit: deg}]\n")

        ws = self._create_workspace(yAxSpec=False)
        f = self._file(ws, "File")
        coord = []
        # Y axis is NumericAxis in q units
        for i in range(0, 12):
            s = f.readline()
            if i >= 8:
                coord.append(s)

        self.assertEqual(coord[0], "Coord:\n")
        self.assertEqual(coord[1], "  x: {name: w, unit: meV}\n")
        self.assertEqual(coord[2], "  y: {name: \'S(q,w)\', unit: meV-1}\n")
        self.assertEqual(coord[3], "  z: [{name: q, unit: A-1}]\n")

    def test_rpar(self):
        """ Test save RPar from workspace with and without sample logs
        """
        r_par = []
        # workspace with all ample logs
        for i in range(21):
            s = self._n_file.readline()
            if i >= 12:
                r_par.append(s)

        self.assertEqual(r_par[0], "RPar:\n")
        self.assertEqual(r_par[1], "  - name: T\n")
        self.assertEqual(r_par[2], "    unit: K\n")
        self.assertEqual(r_par[3], "    val: " + str(self.temperature) + "\n")
        self.assertEqual(r_par[4], "    stdv: 0\n")
        self.assertEqual(r_par[5], "  - name: Ei\n")
        self.assertEqual(r_par[6], "    unit: meV\n")
        self.assertEqual(r_par[7], "    val: " + str(self.Ei) + "\n")
        self.assertEqual(r_par[8], "    stdv: 0\n")

        r_par = []
        # workspace with no sample logs
        for i in range(10):
            s = self._no_sample_file.readline()
            if i >= 9:
                r_par.append(s)

        self.assertEqual(r_par[0], "RPar: []\n")

    def test_slices(self):
        """ Test save slices from workspace with no sample logs
        """
        slices = []
        for i in range(15):
            s = self._no_sample_file.readline()
            if i >= 10:
                slices.append(s)

        self.assertEqual(slices[0], "Slices:\n")
        self.assertEqual(slices[1], "  - j: 0\n")
        self.assertTrue(slices[2].startswith("    z: [{val: 14.1499"))
        self.assertTrue(slices[2].endswith("}]\n"))
        self.assertEqual(slices[3], "    x: [" + str((self.data_x[0] + self.data_x[1]) / 2) + ", "
                         + str((self.data_x[1] + self.data_x[2]) / 2) + ", " + str((self.data_x[2] + self.data_x[3]) / 2)
                         + "]" + "\n")
        self.assertEqual(slices[4], "    y: " + str(self.data_y) + "\n")

    def test_event_ws(self):
        """ Test algorithm is not running with EventWorkspace
        """
        ws = self._create_workspace(False)
        self.assertRaises(RuntimeError, SaveYDA, InputWorkspace=ws, Filename="File")

    def test_x_not_detaE(self):
        """ Test algorithm is not running if X axis is not DeltaE
        """
        ws = self._create_workspace(xAx=False)
        self.assertRaises(ValueError, SaveYDA, InputWorkspace=ws, Filename="File")

    def test_no_Instrument(self):
        """ Test algorithm is not running is workspace has no instrument
        """
        ws = self._create_workspace(instrument=False)
        self.assertRaises(ValueError, SaveYDA, InputWorkspace=ws, Filename="File")

    def test_y_not_mt_or_spec(self):
        """ Test algorithm is not running if Y axis is not SpectrumAxis or MomentumTransfer
        """
        ws = self._create_workspace(yAxMt=False, yAxSpec=False)
        self.assertRaises(RuntimeError, SaveYDA, InputWorkspace=ws, Filename="File")

    def _init_ws_normal(self):
        """ init normal workspace, normal workspace is workspace with all sample logs and save file from workspace
        """
        self._n_ws = self._create_workspace()
        self._n_file = self._file(self._n_ws, "normalFile")

    def _add_all_sample_logs(self, ws):
        """ add all sample logs to a workspace
        :param ws: workspace where sample logs should be added
        """
        AddSampleLog(ws, "proposal_number", str(self.prop_num))
        AddSampleLog(ws, "proposal_title", self.prop_title)
        AddSampleLog(ws, "experiment_team", self.exp_team)
        AddSampleLog(ws, "temperature", str(self.temperature), LogUnit="F")
        AddSampleLog(ws, "Ei", str(self.Ei), LogUnit="meV")

    def _file(self, ws, filename):
        """ create file form workspace and open to read from the file
        :param ws: workspace file will be saved from
        :param filename: name of the file to save
        :return f: open file
        """
        SaveYDA(InputWorkspace=ws, Filename=filename)
        f = open(filename, "r")
        return f

    def _create_workspace(self, ws_2D=True, sample=True, xAx=True, yAxSpec=True,
                          yAxMt=True, instrument=True):
        """ create Workspace
        :param ws_2D: should workspace be 2D?
        :param sample: should workspace have sample logs?
        :param xAx: should x axis be DeltaE?
        :param yAxMt: should y axis be MomentumTransfer?
        :param yAxSpec: should y axis be SpectrumAxis?
        :param instrument: should workspace have a instrument?
        """
        # Event Workspace
        if not ws_2D:
            ws = CreateSampleWorkspace("Event", "One Peak", XUnit="DeltaE")
            return ws
        if not xAx:
            ws = CreateWorkspace(DataX=self.data_x, DataY=self.data_y, DataE=np.sqrt(self.data_y), NSpec=1, UnitX="TOF")
            return ws
        if not instrument:
            ws = CreateWorkspace(DataX=self.data_x, DataY=self.data_y, DataE=np.sqrt(self.data_y), NSpec=1, UnitX="DeltaE")
            return ws
        if not yAxMt and not yAxSpec:
            ws = CreateWorkspace(DataX=self.data_x, DataY=self.data_y, DataE=np.sqrt(self.data_y), NSpec=1, UnitX="DeltaE")
            LoadInstrument(ws, False, InstrumentName="TOFTOF")
            ConvertSpectrumAxis(InputWorkspace=ws, OutputWorkspace=ws, Target="theta", EMode="Direct")
            return ws
        if not yAxSpec and yAxMt:
            ws = CreateWorkspace(DataX=self.data_x, DataY=self.data_y, DataE=np.sqrt(self.data_y), NSpec=1, UnitX="DeltaE")
            LoadInstrument(ws, False, InstrumentName="TOFTOF")
            self._add_all_sample_logs(ws)
            ConvertSpectrumAxis(InputWorkspace=ws, OutputWorkspace="ws2", Target ="ElasticQ", EMode="Direct")
            ws2 = mtd["ws2"]
            return ws2
        if not sample:
            ws = CreateWorkspace(DataX=self.data_x, DataY=self.data_y, DataE=np.sqrt(self.data_y), NSpec=1, UnitX="DeltaE")
            LoadInstrument(ws, False, InstrumentName="TOFTOF")
            return ws
        else:
            ws = CreateWorkspace(DataX=self.data_x, DataY=self.data_y, DataE=np.sqrt(self.data_y), NSpec=1, UnitX="DeltaE")
            LoadInstrument(ws, False, InstrumentName="TOFTOF")
            self._add_all_sample_logs(ws)
            return ws

if __name__ == '__main__':
    unittest.main()
