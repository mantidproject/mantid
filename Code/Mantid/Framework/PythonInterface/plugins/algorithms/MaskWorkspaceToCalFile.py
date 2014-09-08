import sys
from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *

class QueryFlag:
        def isMasked(self, detector, yValue):
            return detector.isMasked()

class QueryValue:
        def isMasked(self, detector, yValue):
            return yValue == 1

class MaskWorkspaceToCalFile(PythonAlgorithm):

    def category(self):
    	return "DataHandling\\Text;Diffraction;PythonAlgorithms"

    def name(self):
    	return "MaskWorkspaceToCalFile"

    def summary(self):
    	return "Saves the masking information in a workspace to a Cal File."

    def PyInit(self):
    	self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "", Direction.Input), "The workspace containing the Masking to extract.")
    	self.declareProperty(FileProperty(name="OutputFile",defaultValue="",action=FileAction.Save,extensions=['cal']), "The file for the results.")

    	self.declareProperty("Invert", False, "If True, masking is inverted in the input workspace. Default: False")

    def PyExec(self):
    	#extract settings
    	inputWorkspace = mtd[self.getPropertyValue("InputWorkspace")]
    	outputFileName = self.getProperty("OutputFile").value
    	invert = self.getProperty("Invert").value
    	mask_query = QueryFlag()
    	if inputWorkspace.id() == "MaskWorkspace":
    	        mask_query = QueryValue()

        #check for consistency
    	if len(inputWorkspace.readX(0)) < 1:
    		raise RuntimeError('The input workspace is empty.')

    	#define flags for masking and not-masking
    	masking_flag = 0
    	not_masking_flag = 1

    	if invert:
    		masking_flag, not_masking_flag = not_masking_flag, masking_flag

    	calFile = open(outputFileName,"w")
    	#write a header
    	instrumentName = inputWorkspace.getInstrument().getName()
    	calFile.write('# '+instrumentName+' detector file\n')
    	calFile.write('# Format: number      UDET       offset       select    group\n')
    	#save the grouping
    	for i in range(inputWorkspace.getNumberHistograms()):
    		try:
    			det = inputWorkspace.getDetector(i)
    			y_value = inputWorkspace.readY(i)[0]
    			if (mask_query.isMasked(det, y_value)): #check if masked
    				group = masking_flag
    			else:
    				group = not_masking_flag
    			detIDs = []
    			try:
    				detIDs = det.getDetectorIDs()
    			except:
    				detIDs = [det.getID()]
    			for id in detIDs:
    				calFile.write(self.FormatLine(i,id,0.0,group,group))
    		except:
    			#no detector for this spectra
    			pass
    	calFile.close()


    def FormatLine(self,number,UDET,offset,select,group):
    	line = "{0:9d}{1:16d}{2:16.7f}{3:9d}{4:9d}\n".format(number,UDET,offset,select,group)
    	return line

#############################################################################################

AlgorithmFactory.subscribe(MaskWorkspaceToCalFile())
