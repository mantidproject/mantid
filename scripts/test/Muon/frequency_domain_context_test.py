# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import sys
import unittest
import six

from Muon.GUI.FrequencyDomainAnalysis.frequency_context import MaxEnt, FFT, FrequencyContext, FREQUENCY_EXTENSIONS
from mantid.simpleapi import CreateWorkspace, AnalysisDataService

if sys.version_info.major < 2:
    from unittest import mock
else:
    import mock


FFT_NAME_RE_2 = "FFT; Re MUSR62260; Group; fwd; Asymmmetry; FD_Re"
FFT_NAME_RE_MOD = "FFT; Re MUSR62260; Group; fwd; Asymmmetry; FD_Mod"

FFT_NAME_COMPLEX_RE= "FFT; Re MUSR62260; Group; fwd; Asymmmetry; FD; Im MUSR62261; Group; top; Asymmetry; FD_Re"
FFT_NAME_COMPLEX_IM= "FFT; Re MUSR62260; Group; fwd; Asymmmetry; FD; Im MUSR62261; Group; top; Asymmetry; FD_Im"
FFT_NAME_COMPLEX_MOD= "FFT; Re MUSR62260; Group; fwd; Asymmmetry; FD; Im MUSR62261; Group; top; Asymmetry; FD_Mod"

PHASEQUAD_NAME_IM = "FFT; Re MUSR62261; PhaseQuad FD MUSR62260; PhaseTable FD; top; bkwd; Im MUSR62261; PhaseQuad FD MUSR62260; PhaseTable FD; top; bkwd_Mod"
PHASEQUAD_NAME_RE = "FFT; Re MUSR62261; PhaseQuad FD MUSR62260; PhaseTable FD; top; bkwd_Mod"

class MuonFreqContextTest(unittest.TestCase):
    def setUp(self):
        self.ws_freq = "MUSR62260_raw_data FD; MaxEnt"
        CreateWorkspace([0], [0], OutputWorkspace=self.ws_freq)
        self.context = FrequencyContext()
        run = "62260"
        self.context.add_maxEnt(run, self.ws_freq)
        self.context.add_FFT(ws_freq_name=FFT_NAME_RE_2, Re_run="62260",Re= "fwd", Im_run="", Im="",phasequad=False)
        self.context.add_FFT(ws_freq_name=FFT_NAME_RE_MOD, Re_run="62260",Re= "fwd", Im_run="", Im="",phasequad=False)

    def test_widnow_title(self):
        self.assertEquals(self.context.window_title, "Frequency Domain Analysis")
 
    def test_add_maxEnt(self):
        self.assertEquals(list(self.context._maxEnt_freq.keys()),['MUSR62260_raw_data FD; MaxEnt'] )
        self.assertEquals(self.context._maxEnt_freq["MUSR62260_raw_data FD; MaxEnt"].run, "62260" )
        self.assertEquals(self.context._maxEnt_freq["MUSR62260_raw_data FD; MaxEnt"].ws_freq.name(), self.ws_freq)

    def test_maxEnt_freq(self):
        self.assertEquals(self.context.maxEnt_freq, ["MUSR62260_raw_data FD; MaxEnt"])

    def test_add_FFT(self):
        six.assertCountEqual(self,list(self.context._FFT_freq.keys()),[FFT_NAME_RE_2, FFT_NAME_RE_MOD] )
        self.assertEquals(self.context._FFT_freq[FFT_NAME_RE_2].Re_run, "62260")
        self.assertEquals(self.context._FFT_freq[FFT_NAME_RE_2].Re, "fwd" )
        self.assertEquals(self.context._FFT_freq[FFT_NAME_RE_2].Im, None )
        self.assertEquals(self.context._FFT_freq[FFT_NAME_RE_2].Im_run, None )
        self.assertEquals(self.context._FFT_freq[FFT_NAME_RE_2].phasequad, False )
 
    def test_add_complex_FFT(self):
        self.context.add_FFT(ws_freq_name=FFT_NAME_COMPLEX_RE, Re_run="62260",Re= "top", Im_run="62261", Im="fwd",phasequad=False)
        self.context.add_FFT(ws_freq_name=FFT_NAME_COMPLEX_IM, Re_run="62260",Re= "top", Im_run="62261", Im="fwd",phasequad=False)
        self.context.add_FFT(ws_freq_name=FFT_NAME_COMPLEX_MOD, Re_run="62260",Re= "top", Im_run="62261", Im="fwd",phasequad=False)
        six.assertCountEqual(self,list(self.context._FFT_freq.keys()),[FFT_NAME_COMPLEX_RE, FFT_NAME_COMPLEX_IM, FFT_NAME_COMPLEX_MOD, FFT_NAME_RE_2, FFT_NAME_RE_MOD] )
        self.assertEquals(self.context._FFT_freq[FFT_NAME_COMPLEX_RE].Re_run, "62260")
        self.assertEquals(self.context._FFT_freq[FFT_NAME_COMPLEX_RE].Re, "top" )
        self.assertEquals(self.context._FFT_freq[FFT_NAME_COMPLEX_RE].Im, "fwd" )
        self.assertEquals(self.context._FFT_freq[FFT_NAME_COMPLEX_RE].Im_run, "62261" )
        self.assertEquals(self.context._FFT_freq[FFT_NAME_COMPLEX_RE].phasequad, False )

    def test_get_freq_names_maxEnt(self):
        output = self.context.get_frequency_workspace_names(run_list=[["62260","62261","62262"]],group=["fwd","top"],pair=[],phasequad=False,frequency_type="MaxEnt")
        self.assertEquals(output, ["MUSR62260_raw_data FD; MaxEnt"])

    def test_get_freq_names_FFT_no_Im_all_parts(self):
        output = self.context.get_frequency_workspace_names(run_list=[["62260"]],group=["fwd","top"],pair=[],phasequad=False,frequency_type="FFT All")
        six.assertCountEqual(self,output,[FFT_NAME_RE_2, FFT_NAME_RE_MOD])

    def test_get_freq_names_FFT_no_Im_Re_parts(self):
        output = self.context.get_frequency_workspace_names(run_list=[["62260"]],group=["fwd","top"],pair=[],phasequad=False,frequency_type="Re")
        six.assertCountEqual(self,output,[FFT_NAME_RE_2])

    def test_get_freq_names_FFT_no_Im_Mod_parts(self):
        output = self.context.get_frequency_workspace_names(run_list=[["62260"]],group=["fwd","top"],pair=[],phasequad=False,frequency_type="Mod")
        six.assertCountEqual(self,output,[FFT_NAME_RE_MOD])

    def test_get_freq_names_FFT_no_Im_Im_parts(self):
        output = self.context.get_frequency_workspace_names(run_list=[["62260"]],group=["fwd","top"],pair=[],phasequad=False,frequency_type="Im")
        six.assertCountEqual(self,output,[])

    def test_get_freq_names_FFT_run(self):
        self.context.add_FFT(ws_freq_name=FFT_NAME_COMPLEX_RE, Re_run="62260",Re= "top", Im_run="62261", Im="fwd",phasequad=False)
        self.context.add_FFT(ws_freq_name=FFT_NAME_COMPLEX_IM, Re_run="62260",Re= "top", Im_run="62261", Im="fwd",phasequad=False)
        self.context.add_FFT(ws_freq_name=FFT_NAME_COMPLEX_MOD, Re_run="62260",Re= "top", Im_run="62261", Im="fwd",phasequad=False)

        output = self.context.get_frequency_workspace_names(run_list=[["62261","62262"]],group=["fwd","top"],pair=[],phasequad=False,frequency_type="FFT All")
        six.assertCountEqual(self,output,[FFT_NAME_COMPLEX_RE, FFT_NAME_COMPLEX_IM, FFT_NAME_COMPLEX_MOD])

    def test_get_freq_names_FFT_group(self):
        self.context.add_FFT(ws_freq_name=FFT_NAME_COMPLEX_RE, Re_run="62260",Re= "top", Im_run="62261", Im="fwd",phasequad=False)
        self.context.add_FFT(ws_freq_name=FFT_NAME_COMPLEX_IM, Re_run="62260",Re= "top", Im_run="62261", Im="fwd",phasequad=False)
        self.context.add_FFT(ws_freq_name=FFT_NAME_COMPLEX_MOD, Re_run="62260",Re= "top", Im_run="62261", Im="fwd",phasequad=False)

        output = self.context.get_frequency_workspace_names(run_list=[["62260"]],group=["top"],pair=[],phasequad=False,frequency_type="FFT All")
        six.assertCountEqual(self,output,[FFT_NAME_COMPLEX_RE, FFT_NAME_COMPLEX_IM, FFT_NAME_COMPLEX_MOD])

    def test_get_freq_names_all(self):
        self.context.add_FFT(ws_freq_name=FFT_NAME_COMPLEX_RE, Re_run="62260",Re= "top", Im_run="62261", Im="fwd",phasequad=False)
        self.context.add_FFT(ws_freq_name=FFT_NAME_COMPLEX_IM, Re_run="62260",Re= "top", Im_run="62261", Im="fwd",phasequad=False)
        self.context.add_FFT(ws_freq_name=FFT_NAME_COMPLEX_MOD, Re_run="62260",Re= "top", Im_run="62261", Im="fwd",phasequad=False)

        output = self.context.get_frequency_workspace_names(run_list=[[62260],[62261]],group=["fwd"],pair=[],phasequad=False,frequency_type="All")
        six.assertCountEqual(self,output,[FFT_NAME_COMPLEX_RE, FFT_NAME_COMPLEX_IM, FFT_NAME_COMPLEX_MOD, FFT_NAME_RE_2, FFT_NAME_RE_MOD, "MUSR62260_raw_data FD; MaxEnt"])

    def test_get_freq_names_all_with_phasequad(self):
        self.context.add_FFT(ws_freq_name=PHASEQUAD_NAME_RE, Re_run="62261",Re= "", Im_run="", Im="",phasequad=True)
        self.context.add_FFT(ws_freq_name=PHASEQUAD_NAME_IM, Re_run="62261",Re= "", Im_run="62260", Im="",phasequad=True)

        output = self.context.get_frequency_workspace_names(run_list=[[62260],[62261]],group=["fwd"],pair=[],phasequad=True,frequency_type="All")
        six.assertCountEqual(self,output,[PHASEQUAD_NAME_RE, PHASEQUAD_NAME_IM, FFT_NAME_RE_2, FFT_NAME_RE_MOD, "MUSR62260_raw_data FD; MaxEnt"])

    def test_get_freq_names_all_with_phasequad_Re_run(self):
        self.context.add_FFT(ws_freq_name=PHASEQUAD_NAME_RE, Re_run="62261",Re= "", Im_run="", Im="",phasequad=True)
        self.context.add_FFT(ws_freq_name=PHASEQUAD_NAME_IM, Re_run="62261",Re= "", Im_run="62260", Im="",phasequad=True)

        output = self.context.get_frequency_workspace_names(run_list=[[62261]],group=["fwd"],pair=[],phasequad=True,frequency_type="All")
        six.assertCountEqual(self,output,[PHASEQUAD_NAME_RE, PHASEQUAD_NAME_IM])

    def test_get_freq_names_all_with_phasequad_Im_run(self):
        self.context.add_FFT(ws_freq_name=PHASEQUAD_NAME_RE, Re_run="62261",Re= "", Im_run="", Im="",phasequad=True)
        self.context.add_FFT(ws_freq_name=PHASEQUAD_NAME_IM, Re_run="62261",Re= "", Im_run="62260", Im="",phasequad=True)

        output = self.context.get_frequency_workspace_names(run_list=[[62260]],group=["fwd"],pair=[],phasequad=True,frequency_type="All")
        six.assertCountEqual(self,output,[PHASEQUAD_NAME_IM, FFT_NAME_RE_2, FFT_NAME_RE_MOD, "MUSR62260_raw_data FD; MaxEnt"])


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
