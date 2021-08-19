# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from Muon.GUI.FrequencyDomainAnalysis.frequency_context import FrequencyContext
from unittest import mock
from mantid.simpleapi import CreateWorkspace

FFT_NAME_RE_2 = "FFT; Re MUSR62260; Group; fwd; Asymmmetry; FD_Re"
FFT_NAME_RE_MOD = "FFT; Re MUSR62260; Group; fwd; Asymmmetry; FD_Mod"

FFT_NAME_COMPLEX_RE = "FFT; Re MUSR62260; Group; fwd; Asymmmetry; FD; Im MUSR62261; Group; top; Asymmetry; FD_Re"
FFT_NAME_COMPLEX_IM = "FFT; Re MUSR62260; Group; fwd; Asymmmetry; FD; Im MUSR62261; Group; top; Asymmetry; FD_Im"
FFT_NAME_COMPLEX_MOD = "FFT; Re MUSR62260; Group; fwd; Asymmmetry; FD; Im MUSR62261; Group; top; Asymmetry; FD_Mod"

PHASEQUAD_NAME_IM = "FFT; Re MUSR62261; PhaseQuad FD MUSR62260; PhaseTable FD; top; bkwd; Im MUSR62261; PhaseQuad FD " \
                    "MUSR62260; PhaseTable FD; top; bkwd_Mod"
PHASEQUAD_NAME_RE = "FFT; Re MUSR62261; PhaseQuad FD MUSR62260; PhaseTable FD; top; bkwd_Mod"


class MuonFreqContextTest(unittest.TestCase):
    def setUp(self):
        self.ws_freq = "MUSR62260_raw_data FD; MaxEnt"
        CreateWorkspace([0], [0], OutputWorkspace=self.ws_freq)
        self.context = FrequencyContext()
        run = "62260"
        self.context.add_maxEnt(run, self.ws_freq)
        self.context.add_FFT(ws_freq_name=FFT_NAME_RE_2, Re_run="62260", Re="fwd", Im_run="", Im="")
        self.context.add_FFT(ws_freq_name=FFT_NAME_RE_MOD, Re_run="62260", Re="fwd", Im_run="", Im="")

    def test_widnow_title(self):
        self.assertEquals(self.context.window_title, "Frequency Domain Analysis")

    def test_add_maxEnt(self):
        self.assertEquals(list(self.context._maxEnt_freq.keys()), ['MUSR62260_raw_data FD; MaxEnt'])
        self.assertEquals(self.context._maxEnt_freq["MUSR62260_raw_data FD; MaxEnt"].run, "62260")
        self.assertEquals(self.context._maxEnt_freq["MUSR62260_raw_data FD; MaxEnt"].ws_freq.name(), self.ws_freq)

    def test_maxEnt_freq(self):
        self.assertEquals(self.context.maxEnt_freq, ["MUSR62260_raw_data FD; MaxEnt"])

    def test_add_FFT(self):
        self.assertCountEqual(list(self.context._FFT_freq.keys()), [FFT_NAME_RE_2, FFT_NAME_RE_MOD])
        self.assertEquals(self.context._FFT_freq[FFT_NAME_RE_2].Re_run, "62260")
        self.assertEquals(self.context._FFT_freq[FFT_NAME_RE_2].Re, "fwd")
        self.assertEquals(self.context._FFT_freq[FFT_NAME_RE_2].Im, None)
        self.assertEquals(self.context._FFT_freq[FFT_NAME_RE_2].Im_run, None)

    def test_add_complex_FFT(self):
        self.context.add_FFT(ws_freq_name=FFT_NAME_COMPLEX_RE, Re_run="62260", Re="top", Im_run="62261", Im="fwd")
        self.context.add_FFT(ws_freq_name=FFT_NAME_COMPLEX_IM, Re_run="62260", Re="top", Im_run="62261", Im="fwd")
        self.context.add_FFT(ws_freq_name=FFT_NAME_COMPLEX_MOD, Re_run="62260", Re="top", Im_run="62261", Im="fwd")

        self.assertCountEqual(list(self.context._FFT_freq.keys()),
                              [FFT_NAME_COMPLEX_RE, FFT_NAME_COMPLEX_IM, FFT_NAME_COMPLEX_MOD, FFT_NAME_RE_2, FFT_NAME_RE_MOD])
        self.assertEquals(self.context._FFT_freq[FFT_NAME_COMPLEX_RE].Re_run, "62260")
        self.assertEquals(self.context._FFT_freq[FFT_NAME_COMPLEX_RE].Re, "top")
        self.assertEquals(self.context._FFT_freq[FFT_NAME_COMPLEX_RE].Im, "fwd")
        self.assertEquals(self.context._FFT_freq[FFT_NAME_COMPLEX_RE].Im_run, "62261")

    def test_get_freq_names_maxEnt(self):
        output = self.context.get_frequency_workspace_names(run_list=[["62260", "62261", "62262"]],
                                                            group=["fwd", "top"], pair=[],
                                                            frequency_type="MaxEnt")
        self.assertEquals(output, ["MUSR62260_raw_data FD; MaxEnt"])

    def test_get_freq_names_FFT_no_Im_all_parts(self):
        output = self.context.get_frequency_workspace_names(run_list=[["62260"]], group=["fwd", "top"], pair=[],
                                                            frequency_type="FFT All")
        self.assertCountEqual(output, [FFT_NAME_RE_2, FFT_NAME_RE_MOD])

    def test_get_freq_names_FFT_no_Im_Re_parts(self):
        output = self.context.get_frequency_workspace_names(run_list=[["62260"]], group=["fwd", "top"], pair=[],
                                                            frequency_type="Re")
        self.assertCountEqual(output, [FFT_NAME_RE_2])

    def test_get_freq_names_FFT_no_Im_Mod_parts(self):
        output = self.context.get_frequency_workspace_names(run_list=[["62260"]], group=["fwd", "top"], pair=[],
                                                            frequency_type="Mod")
        self.assertCountEqual(output, [FFT_NAME_RE_MOD])

    def test_get_freq_names_FFT_no_Im_Im_parts(self):
        output = self.context.get_frequency_workspace_names(run_list=[["62260"]], group=["fwd", "top"], pair=[],
                                                            frequency_type="Im")
        self.assertCountEqual(output, [])

    def test_get_freq_names_FFT_run(self):
        self.context.add_FFT(ws_freq_name=FFT_NAME_COMPLEX_RE, Re_run="62260", Re="top", Im_run="62261", Im="fwd")
        self.context.add_FFT(ws_freq_name=FFT_NAME_COMPLEX_IM, Re_run="62260", Re="top", Im_run="62261", Im="fwd")
        self.context.add_FFT(ws_freq_name=FFT_NAME_COMPLEX_MOD, Re_run="62260", Re="top", Im_run="62261", Im="fwd")

        output = self.context.get_frequency_workspace_names(run_list=[["62261", "62262"]], group=["fwd", "top"],
                                                            pair=[], frequency_type="FFT All")
        self.assertCountEqual(output, [FFT_NAME_COMPLEX_RE, FFT_NAME_COMPLEX_IM, FFT_NAME_COMPLEX_MOD])

    def test_get_freq_names_FFT_group(self):
        self.context.add_FFT(ws_freq_name=FFT_NAME_COMPLEX_RE, Re_run="62260", Re="top", Im_run="62261", Im="fwd")
        self.context.add_FFT(ws_freq_name=FFT_NAME_COMPLEX_IM, Re_run="62260", Re="top", Im_run="62261", Im="fwd")
        self.context.add_FFT(ws_freq_name=FFT_NAME_COMPLEX_MOD, Re_run="62260", Re="top", Im_run="62261", Im="fwd")

        output = self.context.get_frequency_workspace_names(run_list=[["62260"]], group=["top"], pair=[],
                                                            frequency_type="FFT All")
        self.assertCountEqual(output, [FFT_NAME_COMPLEX_RE, FFT_NAME_COMPLEX_IM, FFT_NAME_COMPLEX_MOD])

    def test_get_freq_names_all(self):
        self.context.add_FFT(ws_freq_name=FFT_NAME_COMPLEX_RE, Re_run="62260", Re="top", Im_run="62261", Im="fwd")
        self.context.add_FFT(ws_freq_name=FFT_NAME_COMPLEX_IM, Re_run="62260", Re="top", Im_run="62261", Im="fwd")
        self.context.add_FFT(ws_freq_name=FFT_NAME_COMPLEX_MOD, Re_run="62260", Re="top", Im_run="62261", Im="fwd")

        output = self.context.get_frequency_workspace_names(run_list=[[62260], [62261]], group=["fwd"], pair=[],
                                                            frequency_type="All")
        self.assertCountEqual(output, [FFT_NAME_COMPLEX_RE, FFT_NAME_COMPLEX_IM, FFT_NAME_COMPLEX_MOD, FFT_NAME_RE_2,
                                       FFT_NAME_RE_MOD, "MUSR62260_raw_data FD; MaxEnt"])

    def mock_table(self,name):
        table = mock.Mock()
        type(table).workspace_name = mock.PropertyMock(return_value=name)
        return table

    def test_add_group_phase_table(self):
        groups_2 = self.mock_table("MUSR phase table 2 groups")
        two_groups = self.mock_table("MUSR phases table two groups")
        three_groups = self.mock_table("MUSR phases table 3 groups")

        self.assertEqual(self.context._group_phase_tables, {})

        self.context.add_group_phase_table(groups_2, 2)
        self.context.add_group_phase_table(two_groups, 2)
        self.context.add_group_phase_table(three_groups, 3)

        self.assertEqual(list(self.context._group_phase_tables.keys()), [2,3])
        self.assertEqual(self.context._group_phase_tables[2], [groups_2, two_groups])
        self.assertEqual(self.context._group_phase_tables[3], [three_groups])

    def test_add_group_phase_table_same_table_twice(self):
        groups_2 = self.mock_table("MUSR phase table 2 groups")
        two_groups = self.mock_table("MUSR phases table two groups")
        three_groups = self.mock_table("MUSR phases table 3 groups")

        self.assertEqual(self.context._group_phase_tables, {})

        self.context.add_group_phase_table(groups_2, 2)
        self.context.add_group_phase_table(two_groups, 2)
        self.context.add_group_phase_table(three_groups, 3)
        self.context.add_group_phase_table(three_groups, 3)

        self.assertEqual(list(self.context._group_phase_tables.keys()), [2,3])
        self.assertEqual(self.context._group_phase_tables[2], [groups_2, two_groups])
        self.assertEqual(self.context._group_phase_tables[3], [three_groups])

    def test_get_group_phase_table(self):
        groups_2 = self.mock_table("MUSR phase table 2 groups")
        two_groups = self.mock_table("MUSR phases table two groups")
        different_inst = self.mock_table("EMU phases table two groups")
        three_groups = self.mock_table("MUSR phases table 3 groups")

        self.assertEqual(self.context._group_phase_tables, {})

        self.context.add_group_phase_table(groups_2, 2)
        self.context.add_group_phase_table(two_groups, 2)
        self.context.add_group_phase_table(different_inst, 2)
        self.context.add_group_phase_table(three_groups, 3)

        self.assertEqual(self.context.get_group_phase_tables(2,"MUSR"),[groups_2.workspace_name, two_groups.workspace_name])
        self.assertEqual(self.context.get_group_phase_tables(3,"MUSR"),[three_groups.workspace_name])
        self.assertEqual(self.context.get_group_phase_tables(2,"EMU"),[different_inst.workspace_name])


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
