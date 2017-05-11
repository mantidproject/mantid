from __future__ import (absolute_import, division, print_function)

import mantid
from mantid.api import mtd
from mantid.simpleapi import CreateWorkspace, CreateSampleWorkspace, SaveYDA, ConvertSpectrumAxis, \
    LoadInstrument, AddSampleLog
import numpy as np
import os
import unittest


class SaveYDATest(unittest.TestCase):

    propn = 3
    propt = "PropTitle"
    expt = "Experiment Team"
    temperature = 100.0
    Ei = 1.0
    datax = [1, 2, 3, 4]
    datay = [2.0, 3.0, 4.0]
    _n_file = None
    _n_ws = None
    _no_sample_file = None

    def setUp(self):
        self._n_ws = self._createWorkspace()
        self._n_file = self._file(self._n_ws, 'File')
        ws = self._createWorkspace(sample=False)
        self._no_sample_file = self._file(ws, 'noSampleFile')

    def cleanup(self, ws_name, filename):
        if os.path.exists(filename):
            os.remove(filename)
        if mantid.mtd.doesExist(ws_name):
            mantid.api.AnalysisDataService.remove(ws_name)

    def test_meta_data(self):
        meta = []
        for i in range(3):
            meta.append(self._n_file.readline())

        self.assertEqual(meta[0], "Meta:\n")
        self.assertEqual(meta[1], "    format: yaml/frida 2.0\n")
        self.assertEqual(meta[2], "    type: gerneric tabular data\n")

    def test_history(self):
        history = []
        for i in range(0, 8):
            s= self._n_file.readline()
            if i >= 3:
                history.append(s)

        self.assertEqual(history[0], "History:\n")
        self.assertEqual(history[1], "  - Proposal number " + str(self.propn) + "\n")
        self.assertEqual(history[2], "  - " + self.propt + "\n")
        self.assertEqual(history[3], "  - " + self.expt + "\n")
        self.assertEqual(history[4], "  - data reduced with mantid\n")

        history = []
        for i in range(0, 5):
            s = self._no_sample_file.readline()
            if i >= 3:
                history.append(s)

        self.assertEqual(history[0], "History:\n")
        self.assertEqual(history[1], "  - data reduced with mantid\n")

    def test_coord(self):
        coord = []
        for i in range(0, 12):
            s = self._n_file.readline()
            if i >= 8:
                coord.append(s)

        self.assertEqual(coord[0], "Coord:\n")
        self.assertEqual(coord[1], "    x: {name: w, unit: meV}\n")
        self.assertEqual(coord[2], "    y: {name: \'S(q,w)\', unit: meV-1}\n")
        self.assertEqual(coord[3], "    z: {name: 2th, unit: deg}\n")

        ws = self._createWorkspace(yAxSpec=False)
        f = self._file(ws, 'File')
        coord = []
        for i in range(0, 12):
            s = f.readline()
            if i >= 8:
                coord.append(s)

        self.assertEqual(coord[0], "Coord:\n")
        self.assertEqual(coord[1], "    x: {name: w, unit: meV}\n")
        self.assertEqual(coord[2], "    y: {name: \'S(q,w)\', unit: meV-1}\n")
        self.assertEqual(coord[3], "    z: {name: q, unit: A-1}\n")

    def test_rpar(self):
        rpar = []
        for i in range(21):
            s = self._n_file.readline()
            if i >= 12:
                rpar.append(s)
        self.assertEqual(rpar[0], "RPar:\n")
        self.assertEqual(rpar[1], "  - name: T\n")
        self.assertEqual(rpar[2], "    unit: K\n")
        self.assertEqual(rpar[3], "    val: " + str(self.temperature) + "\n")
        self.assertEqual(rpar[4], "    stdv: 0\n")
        self.assertEqual(rpar[5], "  - name: Ei\n")
        self.assertEqual(rpar[6], "    unit: meV\n")
        self.assertEqual(rpar[7], "    val: " + str(self.Ei) + "\n")
        self.assertEqual(rpar[8], "    stdv: 0\n")

        rpar = []
        for i in range(10):
            s = self._no_sample_file.readline()
            if i >= 9:
                rpar.append(s)
        self.assertEqual(rpar[0], "RPar: []\n")

    def test_slices(self):
        slices = []
        for i in range(15):
            s = self._no_sample_file.readline()
            if i >= 10:
                slices.append(s)
        self.assertEqual(slices[0], "Slices:\n")
        self.assertEqual(slices[1], "  - j: 0\n")
        self.assertEqual(slices[2], "    z: [{val: 14.149999999999967}]\n")
        self.assertEqual(slices[3], "    x: [" + str((self.datax[0] + self.datax[1])/2 ) + ", "
                         + str((self.datax[1] + self.datax[2])/2) + ", " + str((self.datax[2] + self.datax[3])/2)
                         + "]" + "\n")
        self.assertEqual(slices[4], "    y: " + str(self.datay) + "\n")

    def test_event_ws(self):
        ws = self._createWorkspace(False)
        self.assertRaises(RuntimeError, SaveYDA, InputWorkspace= ws, Filename='File')

    def test_x_not_detaE(self):
        ws = self._createWorkspace(xAx=False)
        self.assertRaises(ValueError, SaveYDA, InputWorkspace= ws, Filename='File')

    def test_no_Instrument(self):
        ws = self._createWorkspace(instrument=False)
        self.assertRaises(ValueError, SaveYDA, InputWorkspace= ws, Filename='File')

    def test_y_not_mt_or_spec(self):
        ws = self._createWorkspace(yAxMt=False,yAxSpec=False)
        self.assertRaises(RuntimeError, SaveYDA, InputWorkspace= ws, Filename='File')

    def _init_ws_normal(self):

        self._n_ws = self._createWorkspace()
        self._n_file = self._file(self._n_ws, "normalFile")

    def _add_all_sample_logs(self, ws):
        AddSampleLog(ws, "proposal_number", str(self.propn))
        AddSampleLog(ws, "proposal_title", self.propt)
        AddSampleLog(ws, "experiment_team", self.expt)
        AddSampleLog(ws, "temperature", str(self.temperature), LogUnit="F")
        AddSampleLog(ws, "Ei", str(self.Ei), LogUnit="meV")

    def _file(self, ws, filename):
        path = os.path.expanduser("~/" + filename + ".yaml")
        SaveYDA(InputWorkspace=ws, Filename=path)
        f = open(path, 'r')
        return f

    def _createWorkspace(self, ws_2D=True, sample=True, xAx=True, yAxSpec=True,
                         yAxMt=True, instrument=True, temperature=True):
        if not ws_2D:
            ws = CreateSampleWorkspace('Event', 'One Peak', XUnit='DeltaE')
            return ws
        if not xAx:
            ws = CreateWorkspace(DataX=self.datax, DataY=self.datay, DataE=np.sqrt(self.datay), NSpec=1, UnitX="TOF")
            return ws
        if not instrument:
            ws = CreateWorkspace(DataX=self.datax, DataY=self.datay, DataE=np.sqrt(self.datay), NSpec=1, UnitX="DeltaE")
            return ws
        if not yAxMt and not yAxSpec:
            ws = CreateWorkspace(DataX=self.datax, DataY=self.datay, DataE=np.sqrt(self.datay), NSpec=1, UnitX="DeltaE")
            LoadInstrument(ws,False,InstrumentName="TOFTOF")
            ConvertSpectrumAxis(InputWorkspace=ws,OutputWorkspace=ws,Target ='theta', EMode="Direct")
            return ws
        if not yAxSpec and yAxMt:
            ws = CreateWorkspace(DataX=self.datax, DataY=self.datay, DataE=np.sqrt(self.datay), NSpec=1, UnitX="DeltaE")
            LoadInstrument(ws,False,InstrumentName="TOFTOF")
            self._add_all_sample_logs(ws)
            ConvertSpectrumAxis(InputWorkspace=ws, OutputWorkspace='ws2', Target ='ElasticQ', EMode="Direct")
            ws2 = mtd['ws2']
            return ws2
        if not sample:
            ws = CreateWorkspace(DataX=self.datax, DataY=self.datay, DataE=np.sqrt(self.datay), NSpec=1, UnitX="DeltaE")
            LoadInstrument(ws,False,InstrumentName="TOFTOF")
            return ws
        else:
            ws = CreateWorkspace(DataX=self.datax, DataY=self.datay, DataE=np.sqrt(self.datay), NSpec=1, UnitX="DeltaE")
            LoadInstrument(ws,False,InstrumentName="TOFTOF")
            self._add_all_sample_logs(ws)
            return ws

if __name__ == '__main__':
    unittest.main()
