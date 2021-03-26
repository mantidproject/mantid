# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import numpy as np

from mantid import config
from mantid.api import AnalysisDataService
from mantid.simpleapi import (ConvertUnits, CreateSampleWorkspace, ElasticWindowMultiple, GroupWorkspaces, Rebin,
                              SetInstrumentParameter, LoadInstrument)


def _create_single_test_workspace(fwhm, output_name, i):
    function = "name=Lorentzian,Amplitude=100,PeakCentre=27500,FWHM=" + str(fwhm)

    CreateSampleWorkspace(Function='User Defined',
                          UserDefinedFunction=function,
                          XMin=27000,
                          XMax=28000,
                          BinWidth=10,
                          NumBanks=1,
                          OutputWorkspace=output_name)
    ConvertUnits(InputWorkspace=output_name, OutputWorkspace=output_name, Target='DeltaE', EMode='Indirect', EFixed=1.5)
    Rebin(InputWorkspace=output_name, OutputWorkspace=output_name, Params=[-0.2, 0.004, 0.2])
    LoadInstrument(Workspace=output_name, InstrumentName='IRIS', RewriteSpectraMap=True)
    SetInstrumentParameter(Workspace=output_name,
                           ParameterName='Efixed',
                           DetectorList=range(1, 113),
                           ParameterType='Number',
                           Value='1.5')

    output = AnalysisDataService.retrieve(output_name)
    output.mutableRun()['run_number'] = i + 1
    output.mutableRun()['sample'] = [1, 2, 3]
    output.mutableRun()['sample'].units = " "


def _create_group_test_workspace(output_name):
    widths = [15.0, 17.5, 20.0, 22.5, 25.0, 27.5, 30.0]
    workspace_names = ["workspace_" + str(i) for i in range(len(widths))]

    for i, (fwhm, name) in enumerate(zip(widths, workspace_names)):
        _create_single_test_workspace(fwhm, name, i)

    GroupWorkspaces(InputWorkspaces=workspace_names, OutputWorkspace=output_name)


class ElasticWindowMultipleTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        config['default.facility'] = 'ISIS'

        _create_group_test_workspace('__testData')

        ElasticWindowMultiple(InputWorkspaces='__testData',
                              IntegrationRangeStart=-0.1,
                              IntegrationRangeEnd=0.1,
                              OutputInQ='__q',
                              OutputInQSquared='__q2',
                              OutputELF='__elf')

    @classmethod
    def tearDownClass(cls):
        AnalysisDataService.clear()

    def setUp(self):
        self.q = AnalysisDataService.retrieve('__q')
        self.q2 = AnalysisDataService.retrieve('__q2')
        self.elf = AnalysisDataService.retrieve('__elf')

    def test_ElasticWindowMultiple_returns_the_expected_q_data(self):
        np.testing.assert_allclose(np.array(self.q.readX(0)[:5]),
                                   np.array([0, 0, 0.32031, 0.36003, 0.39825]),
                                   atol=1e-4)
        np.testing.assert_allclose(np.array(self.q.readY(0)[:5]),
                                   np.array([9.88673, 9.88670, 9.88577, 9.88557, 9.88662]),
                                   atol=1e-4)
        np.testing.assert_allclose(np.array(self.q.readE(0)[:5]),
                                   np.array([3.14432, 3.14431, 3.14416, 3.14413, 3.14430]),
                                   atol=1e-4)

    def test_ElasticWindowMultiple_returns_the_expected_q2_data(self):
        np.testing.assert_allclose(np.array(self.q2.readX(0)[:5]),
                                   np.array([0, 0, 0.10260, 0.12963, 0.15860]),
                                   atol=1e-4)
        np.testing.assert_allclose(np.array(self.q2.readY(0)[:5]),
                                   np.array([2.29119, 2.29119, 2.29110, 2.29108, 2.29118]),
                                   atol=1e-4)
        np.testing.assert_allclose(np.array(self.q2.readE(0)[:5]),
                                   np.array([0.31803, 0.31804, 0.31805, 0.31805, 0.31804]),
                                   atol=1e-4)

    def test_ElasticWindowMultiple_returns_the_expected_elf_data(self):
        np.testing.assert_allclose(np.array(self.elf.readX(0)[:5]), np.array([3, 3, 3, 3, 3]), atol=1e-4)
        np.testing.assert_allclose(np.array(self.elf.readY(0)[:5]),
                                   np.array([9.88673, 9.73880, 9.64518, 9.576120, 9.51828]),
                                   atol=1e-4)
        np.testing.assert_allclose(np.array(self.elf.readE(0)[:5]),
                                   np.array([3.14432, 3.12071, 3.10567, 3.09453, 3.08517]),
                                   atol=1e-4)

    def test_ElasticWindowMultiple_returns_data_with_the_expected_number_of_histograms(self):
        self.assertEqual(self.q.getNumberHistograms(), 7)
        self.assertEqual(self.q2.getNumberHistograms(), 7)
        self.assertEqual(self.elf.getNumberHistograms(), 100)

    def test_ElasticWindowMultiple_returns_a_q_workspace_with_the_expected_axis_labels_and_units(self):
        q_x = self.q.getAxis(0).getUnit()
        q_y = self.q.getAxis(1).getUnit()
        self.assertEqual(q_x.unitID(), "MomentumTransfer")
        self.assertEqual(str(q_x.symbol()), "Angstrom^-1")
        self.assertEqual(q_y.unitID(), "Label")
        self.assertEqual(str(q_y.symbol()), "")

    def test_ElasticWindowMultiple_returns_a_q2_workspace_with_the_expected_axis_labels_and_units(self):
        q2_x = self.q2.getAxis(0).getUnit()
        q2_y = self.q2.getAxis(1).getUnit()
        self.assertEqual(q2_x.unitID(), "QSquared")
        self.assertEqual(str(q2_x.symbol()), "Angstrom^-2")
        self.assertEqual(q2_y.unitID(), "Label")
        self.assertEqual(str(q2_y.symbol()), "")

    def test_ElasticWindowMultiple_returns_an_elf_workspace_with_the_expected_axis_labels_and_units(self):
        elf_x = self.elf.getAxis(0).getUnit()
        elf_y = self.elf.getAxis(1).getUnit()
        self.assertEqual(elf_x.unitID(), "Label")
        self.assertEqual(str(elf_x.symbol()), "")
        self.assertEqual(elf_y.unitID(), "MomentumTransfer")
        self.assertEqual(str(elf_y.symbol()), "Angstrom^-1")


if __name__ == '__main__':
    unittest.main()
