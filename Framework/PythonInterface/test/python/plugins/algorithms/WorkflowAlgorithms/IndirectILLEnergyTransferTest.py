# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import os
import unittest
from mantid.simpleapi import *
from mantid.api import MatrixWorkspace,WorkspaceGroup
from mantid import config


class IndirectILLEnergyTransferTest(unittest.TestCase):

    _runs = dict([('one_wing_QENS', '090661'),
                  ('one_wing_EFWS', '083072'),
                  ('one_wing_IFWS', '083073'),
                  ('two_wing_QENS', '136558-136559'),
                  ('two_wing_EFWS', '143720'),
                  ('two_wing_IFWS', '170300'),
                  ('bats', '215962')])

    # cache the def instrument and data search dirs
    _def_fac = config['default.facility']
    _def_inst = config['default.instrument']
    _data_dirs = config['datasearch.directories']

    def setUp(self):
        # set instrument and append datasearch directory
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'IN16B'
        config.appendDataSearchSubDir('ILL/IN16B/')

    def tearDown(self):
        # set cached facility and datasearch directory
        config['default.facility'] = self._def_fac
        config['default.instrument'] = self._def_inst
        config['datasearch.directories'] = self._data_dirs

    def test_complete_options(self):
        # Tests for map file, no verbose, multiple runs, and crop dead channels for two wing QENS data

        # manually get name of grouping file from parameter file
        idf = os.path.join(config['instrumentDefinition.directory'], "IN16B_Definition.xml")
        ipf = os.path.join(config['instrumentDefinition.directory'], "IN16B_Parameters.xml")
        ws = LoadEmptyInstrument(Filename=idf)
        LoadParameterFile(ws, Filename=ipf)
        instrument = ws.getInstrument()
        grouping_filename = instrument.getStringParameter('Workflow.GroupingFile')[0]
        DeleteWorkspace(ws)

        args = {'Run': self._runs['two_wing_QENS'],
                'MapFile': os.path.join(config['groupingFiles.directory'], grouping_filename),
                'CropDeadMonitorChannels': True,
                'OutputWorkspace': 'red'}

        IndirectILLEnergyTransfer(**args)

        self._check_workspace_group(mtd['red'], 2, 18, 1017)

    def test_one_wing_QENS(self):
        # tests one wing QENS with PSD range
        args = {'Run': self._runs['one_wing_QENS'],
                'ManualPSDIntegrationRange': [20,100]}
        res = IndirectILLEnergyTransfer(**args)
        self._check_workspace_group(res, 1, 18, 1024)

    def test_one_wing_EFWS(self):
        args = {'Run': self._runs['one_wing_EFWS']}
        res = IndirectILLEnergyTransfer(**args)
        self._check_workspace_group(res, 1, 18, 256)

    def test_one_wing_IFWS(self):
        args = {'Run': self._runs['one_wing_IFWS']}
        res = IndirectILLEnergyTransfer(**args)
        self._check_workspace_group(res, 1, 18, 256)

    def test_two_wing_EFWS(self):
        args = {'Run': self._runs['two_wing_EFWS']}
        res = IndirectILLEnergyTransfer(**args)
        self._check_workspace_group(res, 2, 18, 8)

    def test_two_wing_IFWS(self):
        args = {'Run': self._runs['two_wing_IFWS']}
        res = IndirectILLEnergyTransfer(**args)
        self._check_workspace_group(res, 2, 18, 512)

    def test_spectrum_axis(self):
        args = {'Run': self._runs['one_wing_EFWS'], 'SpectrumAxis': '2Theta'}
        res = IndirectILLEnergyTransfer(**args)
        self.assertTrue(res.getItem(0).getAxis(1).getUnit().unitID(), "Theta")

    def test_bats(self):
        args = {'Run': self._runs['bats'], 'PulseChopper': '34'}
        res = IndirectILLEnergyTransfer(**args)
        self._check_workspace_group(res, 1, 2050, 1121)

    def _check_workspace_group(self, wsgroup, nentries, nspectra, nbins):

        self.assertTrue(isinstance(wsgroup, WorkspaceGroup))

        self.assertEquals(wsgroup.getNumberOfEntries(),nentries)

        item = wsgroup.getItem(0)

        self.assertTrue(isinstance(item, MatrixWorkspace))

        self.assertEqual(item.getAxis(0).getUnit().unitID(), "DeltaE")

        self.assertEquals(item.getNumberHistograms(),nspectra)

        self.assertEquals(item.blocksize(), nbins)

        self.assertTrue(item.getSampleDetails())

        self.assertTrue(item.getHistory().lastAlgorithm())

if __name__ == '__main__':
    unittest.main()
