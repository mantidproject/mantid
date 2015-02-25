from stresstesting import MantidStressTest
from mantid.simpleapi import *
from mantid.kernel import PropertyManager
from mantid import config
import os

def MAX_DBL():
    import sys
    return sys.float_info[0]/2

def getNamedParameter(ws, name):
    return ws.getInstrument().getNumberParameter(name)[0]

class DirectInelasticDiagnostic2(MantidStressTest):
    
    def requiredMemoryMB(self):
        """Requires 4Gb"""
        return 4000

    
    def runTest(self):
        red_man = PropertyManager()
        red_man_name = "__dgs_reduction_properties"
        pmds[red_man_name] = red_man
        
        if 'detvan' in mtd:
            detvan = mtd['detvan']
        else:
            detvan = Load('MAP17186.raw')        
        if 'sample' in mtd:
            sample = mtd['sample']
        else:
            sample = Load('MAP17269.raw')        
        
        # Libisis values to check against
        # All PropertyManager properties need to be set
        red_man["LowCounts"] = 1e-10
        red_man["HighCounts"] = 1e10
        red_man["LowOutlier"] = 0.01
        red_man["HighOutlier"] = 100.
        red_man["ErrorBarCriterion"] = 0.0
        red_man["MedianTestLow"] = 0.1
        red_man["MedianTestHigh"] = 2.0
        red_man["SamBkgMedianTestLow"] = 0.0
        red_man["SamBkgMedianTestHigh"] = 1.5
        red_man["SamBkgErrorbarCriterion"] = 3.3
        red_man["RejectZeroBackground"] = True
        # Things needed to run vanadium reduction
        red_man["IncidentBeamNormalisation"] = "ToMonitor"
        red_man["DetVanIntRangeUnits"] = "Energy"
        #  properties affecting diagnostics:

        #reducer.wb_integr_range = [20,300]
        red_man["DetVanIntRangeLow"] = 20.
        red_man["DetVanIntRangeHigh"] = 300.
        red_man["BackgroundCheck"] = True
        red_man["BackgroundTofStart"]=12000.
        red_man["BackgroundTofEnd"]=18000.
        #reducer.bkgd_range=[12000,18000]


        diag_mask = DgsDiagnose(DetVanWorkspace=detvan, SampleWorkspace=sample,
                                ReductionProperties=red_man_name)
        
        MaskDetectors(sample, MaskedWorkspace=diag_mask)
        # Save the masked spectra numbers to a simple ASCII file for comparison
        self.saved_diag_file = os.path.join(config['defaultsave.directory'], 
                                            'CurrentDirectInelasticDiag2.txt')
        handle = file(self.saved_diag_file, 'w')
        for index in range(sample.getNumberHistograms()):
            if sample.getDetector(index).isMasked():
                spec_no = sample.getSpectrum(index).getSpectrumNo()
                handle.write(str(spec_no) + '\n')
        handle.close
        
    def cleanup(self):
        if os.path.exists(self.saved_diag_file):
            if self.succeeded():
                os.remove(self.saved_diag_file)
            else:
                os.rename(self.saved_diag_file, 
                          os.path.join(config['defaultsave.directory'], 
                                       'DirectInelasticDiag2-Mismatch.txt'))
        
    def validateMethod(self):
        return 'validateASCII'
        
    def validate(self):
        return self.saved_diag_file, \
            os.path.join(os.path.dirname(__file__), 
                         'ReferenceResults', 'DirectInelasticDiagnostic.txt')
