# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
import tempfile
import shutil
import os
import numpy as np

from mantid.simpleapi import SaveP2D, CreateWorkspace, DeleteWorkspace


class SaveP2DTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls._test_dir = tempfile.mkdtemp()

    @classmethod
    def teatDownClass(cls):
        shutil.rmtree(cls._test_dir)

    def setUp(self):
        self._workspace = self._create_workspace()

    def tearDown(self):
        DeleteWorkspace(self._workspace)

    def _create_workspace(self):
        """Create a dummy workspace for testing purposes"""
        xData = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0]  # d values for one spectrum (one dPerpendicular value)
        yData = ["1", "2", "3"]  # dPerpendicular binedges
        zData = [1.0, 2.0, 3.0, -1.0, 0.0, np.nan, 3.0, 1.0, 4.0]  # intensity values
        eData = [0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1]  # error values

        # used to join all spectra
        xDataTotal = []  # d Values for all spectra
        zDataTotal = []  # intensity values for all spectra
        eDataTotal = []  # error values for all spectra
        nSpec = len(yData) - 1  # number of spectra

        # Create d and intensity lists for workspace
        for i in range(0, nSpec):
            xDataTotal.extend(xData)  # extends the list of x values in accordance to the number of spectra used
            zDataTotal.extend(zData)  # extends the list of intensity values in accordance to the number of spectra used
            eDataTotal.extend(eData)  # extends the list of error values in accordance to the number of spectra used

        # Create a 2D Workspace containing d and dPerpendicular values with intensities
        ws = CreateWorkspace(
            DataX=xDataTotal,
            DataY=zDataTotal,
            DataE=eDataTotal,
            WorkspaceTitle="test",
            NSpec=nSpec,
            UnitX="dSpacing",
            VerticalAxisUnit="dSpacingPerpendicular",
            VerticalAxisValues=yData,
        )
        return ws

    def test_savep2d_raw(self):
        SaveP2D(
            Workspace=self._workspace,
            OutputFile=os.path.join(self._test_dir, "test_savep2d_raw"),
            RemoveNaN=False,
            RemoveNegatives=False,
            CutData=False,
        )
        text = (
            "#Title: test\n#Inst: .prm\n#Binning: ddperp   0.8888889    1.0000000\n"
            "#Bank: 1\n#2theta   lambda   d-value   dp-value   counts\n  81.3046911      1.3029352      "
            "1.0000000      1.5000000      1.0000000\n  42.5730378      1.4521280      2.0000000      "
            "1.5000000      2.0000000\n  28.5401669      1.4789581      3.0000000      1.5000000      "
            "3.0000000\n  21.4420009      1.4882141      4.0000000      1.5000000     -1.0000000\n  "
            "17.1666094      1.4924723      5.0000000      1.5000000      0.0000000\n  14.3112545      "
            "1.4947782      6.0000000      1.5000000            nan\n  12.2697184      1.4961662      "
            "7.0000000      1.5000000      3.0000000\n  10.7376523      1.4970660      8.0000000      "
            "1.5000000      1.0000000\n   9.5455787      1.4976825      9.0000000      1.5000000      "
            "4.0000000\n 147.7039064      1.9210925      1.0000000      2.5000000      1.0000000\n  "
            "74.0366222      2.4082809      2.0000000      2.5000000      2.0000000\n  48.4687709      "
            "2.4628222      3.0000000      2.5000000      3.0000000\n  36.1141714      2.4797153      "
            "4.0000000      2.5000000     -1.0000000\n  28.8035116      2.4871957      5.0000000      "
            "2.5000000      0.0000000\n  23.9632304      2.4911738      6.0000000      2.5000000            "
            "nan\n  20.5194188      2.4935442      7.0000000      2.5000000      3.0000000\n  17.9428625      "
            "2.4950714      8.0000000      2.5000000      1.0000000\n  15.9421282      2.4961135      "
            "9.0000000      2.5000000      4.0000000\n"
        )
        with open(os.path.join(self._test_dir, "ref_savep2d_raw.txt"), "w") as of:
            of.write(text)
        self._assert_file_content_equal(
            os.path.join(self._test_dir, "ref_savep2d_raw.txt"), os.path.join(self._test_dir, "test_savep2d_raw.p2d")
        )

    def test_savep2d_remove_negatives(self):
        SaveP2D(
            Workspace=self._workspace,
            OutputFile=os.path.join(self._test_dir, "test_savep2d_remove_negatives"),
            RemoveNaN=False,
            RemoveNegatives=True,
            CutData=False,
        )
        text = (
            "#Title: test\n#Inst: .prm\n#Binning: ddperp   0.8888889    1.0000000\n"
            "#Bank: 1\n#2theta   lambda   d-value   dp-value   counts\n  81.3046911      1.3029352      "
            "1.0000000      1.5000000      1.0000000\n  42.5730378      1.4521280      2.0000000      "
            "1.5000000      2.0000000\n  28.5401669      1.4789581      3.0000000      1.5000000      "
            "3.0000000\n  14.3112545      1.4947782      6.0000000      1.5000000            nan\n  "
            "12.2697184      1.4961662      7.0000000      1.5000000      3.0000000\n  10.7376523      "
            "1.4970660      8.0000000      1.5000000      1.0000000\n   9.5455787      1.4976825      "
            "9.0000000      1.5000000      4.0000000\n 147.7039064      1.9210925      1.0000000      "
            "2.5000000      1.0000000\n  74.0366222      2.4082809      2.0000000      2.5000000      "
            "2.0000000\n  48.4687709      2.4628222      3.0000000      2.5000000      3.0000000\n  "
            "23.9632304      2.4911738      6.0000000      2.5000000            nan\n  20.5194188      "
            "2.4935442      7.0000000      2.5000000      3.0000000\n  17.9428625      2.4950714      "
            "8.0000000      2.5000000      1.0000000\n  15.9421282      2.4961135      9.0000000      "
            "2.5000000      4.0000000\n"
        )
        with open(os.path.join(self._test_dir, "ref_savep2d_remove_negatives.txt"), "w") as of:
            of.write(text)
        self._assert_file_content_equal(
            os.path.join(self._test_dir, "ref_savep2d_remove_negatives.txt"),
            os.path.join(self._test_dir, "test_savep2d_remove_negatives.p2d"),
        )

    def test_savep2d_remove_NaN(self):
        SaveP2D(
            Workspace=self._workspace,
            OutputFile=os.path.join(self._test_dir, "test_savep2d_remove_NaN"),
            RemoveNaN=True,
            RemoveNegatives=False,
            CutData=False,
        )
        text = (
            "#Title: test\n#Inst: .prm\n#Binning: ddperp   0.8888889    1.0000000\n"
            "#Bank: 1\n#2theta   lambda   d-value   dp-value   counts\n  81.3046911      1.3029352      "
            "1.0000000      1.5000000      1.0000000\n  42.5730378      1.4521280      2.0000000      "
            "1.5000000      2.0000000\n  28.5401669      1.4789581      3.0000000      1.5000000      "
            "3.0000000\n  21.4420009      1.4882141      4.0000000      1.5000000     -1.0000000\n  "
            "17.1666094      1.4924723      5.0000000      1.5000000      0.0000000\n  12.2697184      "
            "1.4961662      7.0000000      1.5000000      3.0000000\n  10.7376523      1.4970660      "
            "8.0000000      1.5000000      1.0000000\n   9.5455787      1.4976825      9.0000000      "
            "1.5000000      4.0000000\n 147.7039064      1.9210925      1.0000000      2.5000000      "
            "1.0000000\n  74.0366222      2.4082809      2.0000000      2.5000000      2.0000000\n  48.4687709      "
            "2.4628222      3.0000000      2.5000000      3.0000000\n  36.1141714      2.4797153      4.0000000      "
            "2.5000000     -1.0000000\n  28.8035116      2.4871957      5.0000000      2.5000000      0.0000000\n  "
            "20.5194188      2.4935442      7.0000000      2.5000000      3.0000000\n  17.9428625      2.4950714      "
            "8.0000000      2.5000000      1.0000000\n  15.9421282      2.4961135      9.0000000      2.5000000      4.0000000\n"
        )
        with open(os.path.join(self._test_dir, "ref_savep2d_remove_NaN.txt"), "w") as of:
            of.write(text)
        self._assert_file_content_equal(
            os.path.join(self._test_dir, "ref_savep2d_remove_NaN.txt"), os.path.join(self._test_dir, "test_savep2d_remove_NaN.p2d")
        )

    def test_savep2d_cut_data(self):
        SaveP2D(
            Workspace=self._workspace,
            OutputFile=os.path.join(self._test_dir, "test_savep2d_cut_data"),
            RemoveNaN=False,
            RemoveNegatives=False,
            CutData=True,
            TthMin=10,
            TthMax=140,
            LambdaMin=1.4,
            LambdaMax=2.496,
            DMin=2.5,
            DMax=7.5,
            DpMin=1,
            DpMax=3,
        )
        text = (
            "#Title: test\n#Inst: .prm\n#Binning: ddperp   0.8888889    1.0000000\n#Bank: 1\n"
            "#2theta   lambda   d-value   dp-value   counts\n  28.5401669      1.4789581      "
            "3.0000000      1.5000000      3.0000000\n  21.4420009      1.4882141      4.0000000      "
            "1.5000000     -1.0000000\n  17.1666094      1.4924723      5.0000000      1.5000000      "
            "0.0000000\n  14.3112545      1.4947782      6.0000000      1.5000000            nan\n  "
            "12.2697184      1.4961662      7.0000000      1.5000000      3.0000000\n  48.4687709      "
            "2.4628222      3.0000000      2.5000000      3.0000000\n  36.1141714      2.4797153      "
            "4.0000000      2.5000000     -1.0000000\n  28.8035116      2.4871957      5.0000000      "
            "2.5000000      0.0000000\n  23.9632304      2.4911738      6.0000000      2.5000000            "
            "nan\n  20.5194188      2.4935442      7.0000000      2.5000000      3.0000000\n"
        )
        with open(os.path.join(self._test_dir, "ref_savep2d_cut_data.txt"), "w") as of:
            of.write(text)
        self._assert_file_content_equal(
            os.path.join(self._test_dir, "ref_savep2d_cut_data.txt"), os.path.join(self._test_dir, "test_savep2d_cut_data.p2d")
        )

    def test_savep2d_cut_data_dp(self):
        SaveP2D(
            Workspace=self._workspace,
            OutputFile=os.path.join(self._test_dir, "test_savep2d_cut_data_dp"),
            RemoveNaN=False,
            RemoveNegatives=False,
            CutData=True,
            TthMin=10,
            TthMax=140,
            LambdaMin=1.4,
            LambdaMax=2.496,
            DMin=2.5,
            DMax=7.5,
        )
        text = (
            "#Title: test\n#Inst: .prm\n#Binning: ddperp   0.8888889    1.0000000\n#Bank: 1\n"
            "#2theta   lambda   d-value   dp-value   counts\n  28.5401669      1.4789581      "
            "3.0000000      1.5000000      3.0000000\n  21.4420009      1.4882141      4.0000000      "
            "1.5000000     -1.0000000\n  17.1666094      1.4924723      5.0000000      1.5000000      "
            "0.0000000\n  14.3112545      1.4947782      6.0000000      1.5000000            nan\n  "
            "12.2697184      1.4961662      7.0000000      1.5000000      3.0000000\n"
        )
        with open(os.path.join(self._test_dir, "ref_savep2d_cut_data_dp.txt"), "w") as of:
            of.write(text)
        self._assert_file_content_equal(
            os.path.join(self._test_dir, "ref_savep2d_cut_data_dp.txt"), os.path.join(self._test_dir, "test_savep2d_cut_data_dp.p2d")
        )

    def _assert_file_content_equal(self, reference_file, result_file):
        with open(reference_file, "r") as ref_file:
            reference = ref_file.read()
        with open(result_file, "r") as res_file:
            result = res_file.read()
            self.maxDiff = 10000
        self.assertMultiLineEqual(reference, result)


if __name__ == "__main__":
    unittest.main()
