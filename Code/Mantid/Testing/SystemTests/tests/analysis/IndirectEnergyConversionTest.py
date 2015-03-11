#pylint: disable=no-init
import stresstesting
from mantid.simpleapi import *

class IndirectEnergyConversionTest(stresstesting.MantidStressTest):

    def runTest(self):
        instrument = 'IRIS'
        analyser = 'graphite'
        reflection = '002'
        detector_range = [3, 53]
        files = 'irs21360.raw'
        rebin_string = '-0.5,0.005,0.5'

        InelasticIndirectReduction(InputFiles=files,
                                   RebiNString=rebin_string,
                                   DetectorRange=detector_range,
                                   Instrument=instrument,
                                   Analyser=analyser,
                                   Reflection=reflection,
                                   OutputWorkspace='__IndirectEnergyCOnversionTest_out_group')


    def validate(self):
        self.disableChecking.append('Instrument')
        self.disableChecking.append('SpectraMap')
        return 'irs21360_graphite002_red', 'IndirectEnergyConversionTest.nxs'
