# Algorithm to start Bayes programs
from MantidFramework import *
from mantid.simpleapi import *
from mantid import config, logger, mtd

class iFudge(PythonAlgorithm):
 
	def category(self):
		return "Workflow\\MIDAS;PythonAlgorithms"

	def PyInit(self):
		self.declareProperty(Name='InputType',DefaultValue='File',Validator=ListValidator(['File','Workspace']),Description = 'Origin of data input - File (_red.nxs) or Workspace')
		self.declareProperty(Name='Instrument',DefaultValue='IRIS',Validator=ListValidator(['IRIS','OSIRIS']),Description = 'Instrument')
		self.declareProperty(Name='Analyser',DefaultValue='graphite002',Validator=ListValidator(['graphite002','graphite004']),Description = 'Analyser & reflection')
		self.declareProperty(Name='InNumber',DefaultValue='',Validator=MandatoryValidator(),Description = 'Sample run number')
		self.declareProperty(Name='MultiplyBy',DefaultValue='',Validator=MandatoryValidator(),Description = 'Multiplicative scale factor')
		self.declareProperty(Name='OutNumber',DefaultValue='',Validator=MandatoryValidator(),Description = 'Sample run number')
 
	def PyExec(self):

		self.log().information('Scale input')
		workdir = config['defaultsave.directory']
		inType = self.getPropertyValue('InputType')
		instr = self.getPropertyValue('Instrument')
		if instr == 'IRIS':
			prefix = 'irs'
		if instr == 'OSIRIS':
			prefix = 'osi'
		ana = self.getPropertyValue('Analyser')
		InNumb = self.getPropertyValue('InNumber')
		inWS = prefix+InNumb+'_'+ana+'_red'
		factor = self.getPropertyValue('MultiplyBy')
		if inType == 'File':
			spath = os.path.join(workdir, inWS+'.nxs')		# path name for sample nxs file
			logger.notice('Input from File : '+spath)
			LoadNexusProcessed(Filename=spath, OutputWorkspace=inWS)
		else:
			logger.notice('Input from Workspace : '+inWS)
		OutNumb = self.getPropertyValue('OutNumber')
		outWS = prefix+OutNumb+'_'+ana+'_red'
		Scale(InputWorkspace=inWS, OutputWorkspace=outWS, Factor=float(factor), Operation='Multiply')

mantid.registerPyAlgorithm(iFudge())         # Register algorithm with Mantid
