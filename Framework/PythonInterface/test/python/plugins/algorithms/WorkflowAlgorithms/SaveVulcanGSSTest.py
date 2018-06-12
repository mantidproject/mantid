from __future__ import (absolute_import, division, print_function)
from six.moves import range

import unittest
import math
import numpy
import mantid.simpleapi as api
from mantid.kernel import *
from mantid.api import *
from testhelpers import run_algorithm
from mantid.api import AnalysisDataService

import os


class SaveVulcanGSSTest(unittest.TestCase):

    def test_save_gss_simple(self):
        """ Test to Save a single spectrum GSAS file to a basic binning parameters
        """
        # Create a testing data file and workspace
        data_ws_name = "Test_1Spec_Workspace"
        self._create_data_workspace(data_ws_name, num_spec=1)
        # data_ws = AnalysisDataService.retrieve(data_ws_name)
        # print ('VecX: {0}'.format(data_ws.readX(0)))

        bin_ws_name = 'SimpleBinRefTable'
        bin_table = self._create_simple_binning_table(bin_ws_name)

        # Execute
        alg_test = run_algorithm("SaveVulcanGSS",
                                 InputWorkspace=data_ws_name,
                                 BinningTable=bin_ws_name,
                                 OutputWorkspace=data_ws_name + "_rebinned",
                                 GSSFilename="/tmp/tempout.gda",
                                 IPTS=12345,
                                 GSSParmFileName='test.prm')

        # check & verify
        self.assertTrue(alg_test.isExecuted())

        # check outputs
        output_workspace = AnalysisDataService.retrieve(data_ws_name+"_rebinned")

        self.assertEqual(output_workspace.getNumberHistograms(), 1)
        self.assertEqual(len(output_workspace.readX(0)), 132)

        # Delete the TESTING file
        AnalysisDataService.remove("InputWorkspace")
        AnalysisDataService.remove(bin_ws_name)
        AnalysisDataService.remove(data_ws_name+"_rebinned")

        return

    def test_save_gss_vdrive(self):
        """ Test to save a multiple spectra GSS file with various binning workspace
        Data:   tof0 = 4900.
                delta = 0.001
                num_pts = 200
        :return:
        """
        # Create a testing data file and workspace
        data_ws_name = "Test_3Spec_Workspace"
        self._create_data_workspace(data_ws_name, num_spec=3)

        # create binning workspaces
        low_res_bin_name = 'LowerBankBinning'
        self._create_binning_workspace(low_res_bin_name, tof0=4900, delta=0.002, num_pts=100)

        high_res_bin_name = 'HigherBankBinning'
        self._create_binning_workspace(high_res_bin_name, tof0=4500, delta=0.0005, num_pts=400)

        bin_table = self._create_vulcan_binning_table('vulcan_sim_table', low_res_bin_name, high_res_bin_name)

        # Execute
        alg_test = run_algorithm("SaveVulcanGSS",
                                 InputWorkspace=data_ws_name,
                                 BinningTable=bin_table,
                                 OutputWorkspace=data_ws_name + "_rebinned",
                                 GSSFilename="tempout.gda",
                                 IPTS=12345,
                                 GSSParmFileName='test.prm')

        # check & verify
        self.assertTrue(alg_test.isExecuted())

        # check outputs
        output_workspace = AnalysisDataService.retrieve(data_ws_name+"_rebinned")

        self.assertEqual(output_workspace.getNumberHistograms(), 3)
        self.assertEqual(len(output_workspace.readX(0)), 99)

        # Delete the TESTING file
        AnalysisDataService.remove("InputWorkspace")
        AnalysisDataService.remove('vulcan_sim_table')
        AnalysisDataService.remove(data_ws_name+"_rebinned")


    # def test_saveGSS_no_binning(self):
    #     """ Test to Save a GSAS file without rebin to Vdrive's standard binning
    #     """
    #     # Create a test data file and workspace
    #     binfilename = "testbin.dat"
    #     self._createBinFile(binfilename)
    #
    #     datawsname = "TestInputWorkspace"
    #     self._create_data_workspace(datawsname)
    #
    #     # Execute
    #     alg_test = run_algorithm("SaveVulcanGSS",
    #             InputWorkspace = datawsname,
    #             OutputWorkspace = datawsname+"_rebinned",
    #             GSSFilename = "tempout.gda")
    #
    #     self.assertTrue(alg_test.isExecuted())
    #
    #     # Verify ....
    #     outputws = AnalysisDataService.retrieve(datawsname+"_rebinned")
    #     #self.assertEqual(4, tablews.rowCount())
    #
    #     # Delete the test hkl file
    #     os.remove(binfilename)
    #     AnalysisDataService.remove("InputWorkspace")
    #     AnalysisDataService.remove(datawsname+"_rebinned")
    #
    #     return

    @staticmethod
    def _create_binning_workspace(bin_ws_name, tof0, delta, num_pts):
        """
        creating reference binning workspace
        :param bin_ws_name:
        :param tof0:
        :param delta: log binning
        :param num_pts:
        :return:
        """
        list_x = list()

        tof = tof0
        delta = abs(delta)
        for n in range(num_pts):
            x_n = math.log(tof)/math.log(10.)
            tof *= (1 + delta)
            list_x.append(x_n)
        # END-FOR

        vec_tof = numpy.array(list_x)
        vec_y = numpy.array(list_x)

        # create workspace
        api.CreateWorkspace(DataX=vec_tof, DataY=vec_y, DataE=vec_y, NSpec=1, UnitX='TOF',
                            OutputWorkspace=bin_ws_name)

        return

    @staticmethod
    def _create_simple_binning_table(binning_table_name):
        """
        create a binning table
        :return:
        """
        # TODO FIXME : more flexible!
        """
        tof0 = 10000.
        delta = 0.001
        num_pts = 200
        """
        # create a TableWorkspace
        api.CreateEmptyTableWorkspace(OutputWorkspace=binning_table_name)

        bin_table_ws = AnalysisDataService.retrieve(binning_table_name)
        bin_table_ws.addColumn('str', 'WorkspaceIndexes')
        bin_table_ws.addColumn('str', 'BinningParameters')

        # add a row for simple case
        bin_table_ws.addRow(['0', '10000, -0.002, 13000'])

        return bin_table_ws

    @staticmethod
    def _create_vulcan_binning_table(binning_table_name, binning_workspace_low_res, binning_workspace_high_res):
        """ create a binning table for binning data into various resolution
        :param binning_table_name:
        :param binning_workspace_low_res:
        :param binning_workspace_high_res:
        :return:
        """
        # create a TableWorkspace
        api.CreateEmptyTableWorkspace(OutputWorkspace=binning_table_name)

        bin_table_ws = AnalysisDataService.retrieve(binning_table_name)
        bin_table_ws.addColumn('str', 'WorkspaceIndexes')
        bin_table_ws.addColumn('str', 'BinningParameters')

        # add a row for simple case
        bin_table_ws.addRow(['0, 1', '{0}: {1}'.format(binning_workspace_low_res, 0)])
        bin_table_ws.addRow(['2', '{0}: {1}'.format(binning_workspace_high_res, 0)])

        return bin_table_ws

    @staticmethod
    def _create_data_workspace(data_ws_name, num_spec, tof0=None, delta=None, num_pts=None):
        """
        Create a multiple spectra data workspace
        :param data_ws_name:
        :param num_spec:
        :return:
        """
        # get base data sets for the workspace as Histograms
        tof0 = 10000.
        delta = 0.001
        num_pts = 200

        list_x = list()
        list_y = list()
        list_e = list()

        tof = tof0
        for n in range(num_pts):
            list_x.append(tof)
            list_y.append(math.sin(tof0))
            list_e.append(1.)

            tof *= 1+delta
        # END-FOR
        list_x.append(tof)

        vec_x = numpy.array(list_x)
        vec_y = numpy.array(list_y)
        vec_e = numpy.array(list_e)

        # expand to multiple spectra
        if num_spec > 1:
            vec_x_orig = vec_x[:]
            vec_y_orig = vec_y[:]
            vec_e_orig = vec_e[:]

            for spec_index in range(1, num_spec):
                vec_x = numpy.append(vec_x, vec_x_orig)
                vec_i = vec_y_orig[:]
                vec_i *= 2 * (spec_index + 1)
                vec_y = numpy.append(vec_y, vec_i)
                vec_e = numpy.append(vec_e, vec_e_orig)
        # END-FOR

        data_ws = api.CreateWorkspace(DataX=vec_x, DataY=vec_y, DataE=vec_e, NSpec=num_spec, UnitX="TOF")

        # Add to data service
        AnalysisDataService.addOrReplace(data_ws_name, data_ws)

        return data_ws


if __name__ == '__main__':
    unittest.main()
