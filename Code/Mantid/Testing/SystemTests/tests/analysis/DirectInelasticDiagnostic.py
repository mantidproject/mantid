#pylint: disable=no-init
from stresstesting import MantidStressTest
from mantid.simpleapi import MaskDetectors, mtd, config
import Direct.DirectEnergyConversion as reduction
import os

class DirectInelasticDiagnostic(MantidStressTest):

    def requiredMemoryMB(self):
        """Requires 4Gb"""
        return 4000

    def runTest(self):
        white = 'MAP17186.raw'
        sample = 'MAP17269.raw'

        # Libisis values to check against
        tiny=1e-10
        huge=1e10

        v_out_lo = 0.01
        v_out_hi = 100.

        vv_lo = 0.1
        vv_hi = 2.0
        vv_sig = 0.0

        sv_sig = 3.3
        sv_hi = 1.5
        sv_lo = 0.0
        s_zero = True

        reducer = reduction.setup_reducer('MAPS')
        # parameters which explicitly affect diagnostics
        #
        reducer.prop_man.wb_integr_range = [20,300]
        reducer.prop_man.bkgd_range=[12000,18000]
        diag_mask = reducer.diagnose(white, sample, tiny=tiny, huge=huge,
                                     van_out_lo=v_out_lo, van_out_hi=v_out_hi,
                                     van_lo=vv_lo, van_hi=vv_hi, van_sig=vv_sig,
                                     samp_lo=sv_lo, samp_hi=sv_hi, samp_sig=sv_sig, samp_zero=s_zero,hard_mask_file=None)

        sample = reducer.get_run_descriptor(sample)
        sample_ws = sample.get_workspace()
        MaskDetectors(Workspace=sample_ws, MaskedWorkspace=diag_mask)

        # Save the masked spectra nmubers to a simple ASCII file for comparison
        self.saved_diag_file = os.path.join(config['defaultsave.directory'], 'CurrentDirectInelasticDiag.txt')
        handle = file(self.saved_diag_file, 'w')
        for index in range(sample_ws.getNumberHistograms()):
            if sample_ws.getDetector(index).isMasked():
                spec_no = sample_ws.getSpectrum(index).getSpectrumNo()
                handle.write(str(spec_no) + '\n')
        handle.close

    def cleanup(self):
        if os.path.exists(self.saved_diag_file):
            if self.succeeded():
                os.remove(self.saved_diag_file)
            else:
                os.rename(self.saved_diag_file, os.path.join(config['defaultsave.directory'], 'DirectInelasticDiag-Mismatch.txt'))

    def validateMethod(self):
        return 'validateASCII'

    def validate(self):
        return (self.saved_diag_file,'DirectInelasticDiagnostic.txt')
