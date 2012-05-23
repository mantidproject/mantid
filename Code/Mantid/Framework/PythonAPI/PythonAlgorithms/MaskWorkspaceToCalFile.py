"""*WIKI* 

Creates a cal file from a mask workspace: the masked out detectors (Y == 0 in mask workspace) will be combined into group 0.

*WIKI*"""

from MantidFramework import *

class MaskWorkspaceToCalFile(PythonAlgorithm):

	def category(self):
		return "DataHandling\\Text;Diffraction;PythonAlgorithms"

	def name(self):
		return "MaskWorkspaceToCalFile"


	def PyInit(self):
		self.declareWorkspaceProperty("InputWorkspace", "", Direction = Direction.Input, Description = 'Input mask workspace')
		self.declareFileProperty("OutputFile","", FileAction.Save, ['cal'],Description="The file to contain the results")

		self.declareProperty("Invert", False, Description="If True, masking is inverted in the input workspace. Default: False")
        
		
	def PyExec(self):
		#extract settings
		inputWorkspace = self.getProperty("InputWorkspace")
		outputFileName = self.getProperty("OutputFile")
		invert = self.getProperty("Invert")
        
        #check for consistency
		if inputWorkspace.getNumberBins() < 1:
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
				if (det.isMasked()): #check if masked
					group = masking_flag
				else:
					group = not_masking_flag
				if type(det) == DetectorGroup:
					detIDs = det.getDetectorIDs()
				else:
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

mtd.registerPyAlgorithm(MaskWorkspaceToCalFile())
