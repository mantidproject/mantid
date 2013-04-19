"""*WIKI* 
This algorithms writes a cal file with the selection column set to the masking status of the workspaces provided.  The offsets and grouping details of the cal file are not completed, so you would normally use MargeCalFiles afterwards to import these values from another file.
  
*WIKI*"""
"""*WIKI_USAGE*
Example:
 #Load up two workspaces and mask some data
 ws1=Load("GEM38370")
 MaskDetectors(ws1,"100-200")
 
 ws2=Load("GEM38370")
 MaskDetectors(ws2,"300-400")
 
 # Extract the masks to Mask Workspaces 
 #this drops the data and just reatains the mask informantion
 #You can still visualize these using the instrument view
 #Note: ExtractMasking outputs two items you need to catch them seperately
 mw1,detList1=ExtractMasking(ws1)
 mw2,detList2=ExtractMasking(ws2)
 
 #combine the masks
 #Two ways to do this
 #either
 combinedMask = mw1+mw2
 #or
 MaskDetectors(Workspace=mw1,MaskedWorkspace=mw2)
 
 #Extract the Mask to a cal file
 dataDir = "C:/MantidInstall/data/"
 MaskWorkspaceToCalFile(combinedMask,dataDir+"mask.cal")
  
 #Merge this with another cal file to pick up offsets and groups
 MergeCalFiles(UpdateFile = dataDir+"mask.cal", MasterFile=dataDir+"offsets_2006_cycle064.cal", \
 OutputFile=dataDir+"resultCal.cal", MergeOffsets=False, MergeSelections=True,MergeGroups=False)

*WIKI_USAGE*"""
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


	def PyInit(self):
                self.setWikiSummary("Saves the masking information in a workspace to a [[CalFile| Cal File]].")
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
