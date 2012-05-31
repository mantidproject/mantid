"""*WIKI* 

Workflow algorithm used to set up the live reduction of EQSANS data.

*WIKI*"""

import os
from MantidFramework import *
from mantidsimple import *

class EQSANSLiveSetup(PythonAlgorithm):
    """
        Workflow algorithm used to set up the live reduction of EQSANS data
        
        This algorithm should disappear once the EQSANS reduction is completely
        moved to the C++ reduction API that uses workflow algorithms.
    """
    
    def category(self):
        return "Workflow\\SANS;PythonAlgorithms"

    def name(self):
        return "EQSANSLiveReduce"

    def PyInit(self):
        self.declareProperty("BeamCenterX", 89.6749,
                             Description="Beam center in X [pixel]")
        self.declareProperty("BeamCenterY", 129.693,
                             Description="Beam center in Y [pixel]")
        self.declareProperty("DetectorDistance", 4000.0,
                             Description="Sample to detector distance [mm]")
        self.declareFileProperty("SensitivityFile", "",
                                 FileAction.Load, ['.nxs'],
                                 Description="Pre-calculated sensitivity file")
        self.declareProperty("TransmissionDirectBeam", "",
                             Description="Direct beam data file used to compute transmission")
        self.declareProperty("TransmissionEmptyBeam", "",
                             Description="Empty beam data file used to compute transmission")
        #self.declareProperty("DarkCurrent", "",
        #                     Description="Dark current data file")

    def PyExec(self):
        # EQSANS reduction script
        import reduction.instruments.sans.sns_command_interface as cmd

        # Beam center
        center_x = self.getProperty("BeamCenterX")
        center_y = self.getProperty("BeamCenterY")
        
        # Sample-detector distance
        sdd = self.getProperty("DetectorDistance")
        
        # Pre-calculated sensitivity
        sensitivity = self.getProperty("SensitivityFile")
        
        # Transmission data files
        direct_beam = self.getProperty("TransmissionDirectBeam")
        empty_beam = self.getProperty("TransmissionEmptyBeam")

        cmd.EQSANS()
        cmd.SetBeamCenter(center_x, center_y)
        cmd.SetSampleDetectorDistance(sdd)
        cmd.SensitivityCorrection(sensitivity)
        cmd.UseConfigMask(True)
        
        if direct_beam != "" and empty_beam != "":
            cmd.DirectBeamTransmission(direct_beam, empty_beam)
        else:
            cmd.SetTransmission(1.0, 0.0)
        cmd.ThetaDependentTransmission(False)
        
        # Dark current data file
        #dark_current = self.getProperty("DarkCurrent")        
        #if dark_current != "":
        #    cmd.DarkCurrent(dark_current)
        cmd.ReductionSingleton().set_azimuthal_averager(None)
        
mtd.registerPyAlgorithm(EQSANSLiveSetup())
