"""*WIKI* 

Generate Mask workspace by diagnose white beam data

The output of each function is a workspace containing a single bin where:
    0 - denotes a masked spectra and
    1 - denotes an unmasked spectra.

This workspace can be summed with other masked workspaces to accumulate
masking and also passed to MaskDetectors to match masking there.

*WIKI*"""

from MantidFramework import *
from mantidsimple import *

class GenerateMaskingWorkspace(PythonAlgorithm):

    def category(self): 
        """ Mantid required
        """
        return "Diffraction"

    def name(self):
        """ Mantid required
        """
        return "GenerateMaskingWorkspace"

    def PyInit(self):
        """ Mantid required
        """
        self.declareWorkspaceProperty("InputWorkspace", "", Direction=Direction.Input,
                Description="Input white beam run Workspace")
	self.declareWorkspaceProperty("OutputWorkspace", "", Direction=Direction.Output,
		Description="Output Masking Workspace")

        self.declareProperty("Tiny", 1.0E-10, 
                Description="Minimum threshold for acceptance (default = 1e-10)") 
        self.declareProperty("Large", 1.0E10, 
                Description="Maximum threshold for acceptance (default = 1e10)")

        self.declareProperty("SignificantTest", 3.3, 
                Description="detectors in spectra with a total number of counts within this number of standard deviations from the median will not be labelled bad")
        self.declareProperty("MedianLowThreshold", 0.1,
                Description="Detectors corresponding to spectra with total counts equal to or less than this proportion of the median will be identified as reading badly")
        self.declareProperty("MedianHighThreshold", 1.5,
                Description="Detectors corresponding to spectra with total counts equal to or more than this proportion of the median will be identified as reading badly")

        return


    def PyExec(self):
        """ Mantid required
        """
        # 1. Read inputs and put to class variable
        self.whiteRunWS = self.getProperty("InputWorkspace")
        self.tiny = self.getProperty("Tiny")
        self.large = self.getProperty("Large")
        self.median_lo = self.getProperty("MedianLowThreshold")
        self.median_hi = self.getProperty("MedianHighThreshold")
        self.signif = self.getProperty("SignificantTest")

        self.diagnose()

        return

    def diagnose(self):
        """ Diagnose input workspace and thus create MaskingWorkspace
        """
        # 1. Converte to MatrixWorkspace
        ConvertToMatrixWorkspace(self.whiteRunWS, self.whiteRunWS)

        # 2. Integration
        Integration(InputWorkspace=self.whiteRunWS, 
                OutputWorkspace=str(self.whiteRunWS)+"_counts_white-beam")
	whitecountsWS = mtd[str(self.whiteRunWS)+"_counts_white-beam"]

        # 3. Run first white beam tests
	outputwsname = self.getPropertyValue("OutputWorkspace")
	num_failed = self.doWhiteTest(whitecountsWS, outputwsname, self.tiny, self.large, 
                self.median_lo, self.median_hi, self.signif)
        whitemaskWS = mtd[outputwsname]

	print "Number of Pixels Failing In Tests = ", num_failed
	
        # 4. Set property
        self.setProperty("OutputWorkspace", whitemaskWS)

        # 5. Clean
        DeleteWorkspace(Workspace=whitecountsWS)

        return

    def doWhiteTest(self, whitecountsWS, maskingwsname, tiny, large, median_lo, median_hi, signif):
        """ Run the diagnostic tests on the integrated white beam run
    
        Required inputs:
        
          white_counts  - A workspace containing the integrated counts from a
                          white beam vanadium run
          tiny          - Minimum threshold for acceptance
          large         - Maximum threshold for acceptance
          median_lo     - Fraction of median to consider counting low
          median_hi     - Fraction of median to consider counting high
          signif        - Counts within this number of multiples of the 
                          standard dev will be kept (default = 3.3)
        """
        mtd.sendLogMessage('Running first white beam test')
    
        # 1. Make sure we are a MatrixWorkspace
        ConvertToMatrixWorkspace(whitecountsWS, whitecountsWS)
        
        # 2. The output workspace will have the failed detectors masked
        #    And thus the MaskingWorkspace is created
        # Failure reasons:
        # a. White beam counts out of threshold
        # b. White beam counts fail at median detector test
        rangetestwsname = maskingwsname+"_range"
        range_check = FindDetectorsOutsideLimits(InputWorkspace=whitecountsWS, 
                OutputWorkspace=rangetestwsname, HighThreshold=large, LowThreshold=tiny)

        mediantestwsname = maskingwsname+"_median"
        median_test = MedianDetectorTest(InputWorkspace=whitecountsWS, 
                OutputWorkspace=mediantestwsname, 
		SignificanceTest=signif, 
                LowThreshold=median_lo, HighThreshold=median_hi)

        num_failed = range_check['NumberOfFailures'].value + median_test['NumberOfFailures'].value

        # 3. Mask detector
        origrangetestwsname = rangetestwsname
        origmediantestwsname = mediantestwsname
        # rangetestwsname = "Mask_Range"
        # mediantestwsname = "Mask_Median"
        ConvertToMaskWorkspace(InputWorkspace=origrangetestwsname, OutputWorkspace=rangetestwsname)
        ConvertToMaskWorkspace(InputWorkspace=origmediantestwsname, OutputWorkspace=mediantestwsname)
        
        BinaryOperateMasks(InputWorkspace1=mtd[rangetestwsname], InputWorkspace2=mtd[mediantestwsname],
                OperationType="AND", OutputWorkspace=maskingwsname)

        # 4. Clean workspace
        DeleteWorkspace(Workspace=rangetestwsname)
        DeleteWorkspace(Workspace=mediantestwsname)

        return num_failed

mtd.registerPyAlgorithm(GenerateMaskingWorkspace())
