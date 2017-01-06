#pylint: disable=no-init,attribute-defined-outside-init
import stresstesting
from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *

#------------------------------------------------------------------------------------


class DOSPhononTest(stresstesting.MantidStressTest):

    def runTest(self):
        file_name = 'squaricn.phonon'
        self.ouput_ws_name = 'squaricn'
        self.ref_result = 'II.DOSTest.nxs'

        SimulatedDensityOfStates(PHONONFile=file_name,
                                 OutputWorkspace=self.ouput_ws_name)

    def validate(self):
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Sample')

        return self.ouput_ws_name, self.ref_result

#------------------------------------------------------------------------------------


class DOSPhononCrossSectionScaleTest(stresstesting.MantidStressTest):

    def runTest(self):
        file_name = 'squaricn.phonon'
        self.ouput_ws_name = 'squaricn'
        self.ref_result = 'II.DOSCrossSectionScaleTest.nxs'

        SimulatedDensityOfStates(PHONONFile=file_name,
                                 ScaleByCrossSection='Incoherent',
                                 OutputWorkspace=self.ouput_ws_name)

    def validate(self):
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Sample')

        return self.ouput_ws_name, self.ref_result

#------------------------------------------------------------------------------------


class DOSCastepTest(stresstesting.MantidStressTest):

    def runTest(self):
        file_name = 'squaricn.castep'
        self.ouput_ws_name = 'squaricn'
        self.ref_result = 'II.DOSTest.nxs'

        SimulatedDensityOfStates(CASTEPFile=file_name,
                                 OutputWorkspace=self.ouput_ws_name)

    def validate(self):
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Sample')

        return self.ouput_ws_name, self.ref_result

#------------------------------------------------------------------------------------


class DOSRamanActiveTest(stresstesting.MantidStressTest):

    def runTest(self):
        file_name = 'squaricn.phonon'
        spec_type = 'Raman_Active'
        self.ouput_ws_name = 'squaricn'
        self.ref_result = 'II.DOSRamanTest.nxs'

        SimulatedDensityOfStates(PHONONFile=file_name,
                                 SpectrumType=spec_type,
                                 OutputWorkspace=self.ouput_ws_name)

    def validate(self):
        self.tolerance = 1e-3
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Sample')

        return self.ouput_ws_name, self.ref_result

#------------------------------------------------------------------------------------


class DOSIRActiveTest(stresstesting.MantidStressTest):

    def runTest(self):
        file_name = 'squaricn.phonon'
        spec_type = 'IR_Active'
        self.ouput_ws_name = 'squaricn'
        self.ref_result = 'II.DOSIRTest.nxs'

        SimulatedDensityOfStates(PHONONFile=file_name,
                                 SpectrumType=spec_type,
                                 OutputWorkspace=self.ouput_ws_name)

    def validate(self):
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Sample')

        return self.ouput_ws_name, self.ref_result

#------------------------------------------------------------------------------------


class DOSPartialTest(stresstesting.MantidStressTest):

    def runTest(self):
        file_name = 'squaricn.phonon'
        spec_type = 'DOS'
        self.ouput_ws_name = 'squaricn'
        self.ref_result = 'II.DOSPartialTest.nxs'

        SimulatedDensityOfStates(PHONONFile=file_name,
                                 SpectrumType=spec_type,
                                 Ions="H,C,O",
                                 OutputWorkspace=self.ouput_ws_name)

    def validate(self):
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Sample')

        return self.ouput_ws_name, self.ref_result

#------------------------------------------------------------------------------------


class DOSPartialSummedContributionsTest(stresstesting.MantidStressTest):
    """
      This test checks the reference result of the total DOS against
      the summed partial contributions of all elements. The two should be roughly
      equal to within a small degree of error.
    """

    def runTest(self):

        file_name = 'squaricn.phonon'
        spec_type = 'DOS'
        self.ouput_ws_name = 'squaricn'
        self.ref_result = 'II.DOSTest.nxs'

        SimulatedDensityOfStates(PHONONFile=file_name,
                                 SpectrumType=spec_type,
                                 Ions="H,C,O",
                                 SumContributions=True,
                                 OutputWorkspace=self.ouput_ws_name)

    def validate(self):
        self.tolerance = 1e-10
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Sample')

        return self.ouput_ws_name, self.ref_result

#------------------------------------------------------------------------------------


class DOSPartialCrossSectionScaleTest(stresstesting.MantidStressTest):

    def runTest(self):
        file_name = 'squaricn.phonon'
        spec_type = 'DOS'
        self.ouput_ws_name = 'squaricn'
        self.ref_result = 'II.DOSPartialCrossSectionScaleTest.nxs'

        SimulatedDensityOfStates(PHONONFile=file_name,
                                 SpectrumType=spec_type,
                                 Ions="H,C,O",
                                 ScaleByCrossSection='Incoherent',
                                 OutputWorkspace=self.ouput_ws_name)

    def validate(self):
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Sample')

        return self.ouput_ws_name, self.ref_result

#------------------------------------------------------------------------------------


class DOSPartialSummedContributionsCrossSectionScaleTest(stresstesting.MantidStressTest):
    """
      This test checks the reference result of the total DOS against
      the summed partial contributions of all elements. The two should be roughly
      equal to within a small degree of error.
    """

    def runTest(self):

        file_name = 'squaricn.phonon'
        spec_type = 'DOS'
        self.ouput_ws_name = 'squaricn'
        self.ref_result = 'II.DOSCrossSectionScaleTest.nxs'

        SimulatedDensityOfStates(PHONONFile=file_name,
                                 SpectrumType=spec_type,
                                 Ions="H,C,O",
                                 SumContributions=True,
                                 ScaleByCrossSection='Incoherent',
                                 OutputWorkspace=self.ouput_ws_name)

    def validate(self):
        self.tolerance = 1e-10
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Sample')

        return self.ouput_ws_name, self.ref_result
