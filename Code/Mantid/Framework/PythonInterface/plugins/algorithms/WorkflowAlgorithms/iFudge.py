# Algorithm to start Bayes programs
from mantid.simpleapi import *
from mantid.api import PythonAlgorithm, AlgorithmFactory
from mantid.kernel import StringListValidator, StringMandatoryValidator
from mantid import config, logger, mtd

class iFudge(PythonAlgorithm):
 
	def category(self):
		return "Workflow\\MIDAS;PythonAlgorithms"

	def PyInit(self):
		self.declareProperty(name='InputType',defaultValue='File',validator=StringListValidator(['File','Workspace']), doc='Origin of data input - File (_red.nxs) or Workspace')
		self.declareProperty(name='Instrument',defaultValue='iris',validator=StringListValidator(['irs','iris','osi','osiris']), doc='Instrument')
		self.declareProperty(name='Analyser',defaultValue='graphite002',validator=StringListValidator(['graphite002','graphite004']), doc='Analyser & reflection')
		self.declareProperty(name='InNumber',defaultValue='',validator=StringMandatoryValidator(), doc='Sample run number')
		self.declareProperty(name='MultiplyBy',defaultValue='',validator=StringMandatoryValidator(), doc='Multiplicative scale factor')
		self.declareProperty(name='OutNumber',defaultValue='',validator=StringMandatoryValidator(), doc='Sample run number')
 
	def PyExec(self):

		self.log().information('Scale input')
		workdir = config['defaultsave.directory']
		inType = self.getPropertyValue('InputType')
		prefix = self.getPropertyValue('Instrument')
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

AlgorithmFactory.subscribe(iFudge)         # Register algorithm with Mantid
