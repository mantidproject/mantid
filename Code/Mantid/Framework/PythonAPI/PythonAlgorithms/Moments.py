# Algorithm to start Bayes programs
from MantidFramework import *
from mantidsimple import *
from mantidplotpy import *
import IndirectMoment as Main

class Moments(PythonAlgorithm):
 
	def PyInit(self):
		self.declareProperty(Name='Instrument',DefaultValue='IRIS',Validator=ListValidator(['IRIS','OSIRIS']),Description = 'Instrument')
		self.declareProperty(Name='Analyser',DefaultValue='graphite002',Validator=ListValidator(['graphite002','graphite004']),Description = 'Analyser & reflection')
		self.declareProperty(Name='SamNumber',DefaultValue='',Validator=MandatoryValidator(),Description = 'Sample run number')
		self.declareProperty('Verbose',DefaultValue=True,Description = 'Switch Verbose Off/On')
		self.declareProperty('Plot',DefaultValue=True,Description = 'Switch Plot Off/On')
		self.declareProperty('Save',DefaultValue=False,Description = 'Switch Save result to nxs file Off/On')
 
	def PyExec(self):

		self.log().information('Moments calculation')
		instr = self.getPropertyValue('Instrument')
		if instr == 'IRIS':
			prefix = 'irs'
		if instr == 'OSIRIS':
			prefix = 'osi'
		ana = self.getPropertyValue('Analyser')
		sn = self.getPropertyValue('SamNumber')
		sam = prefix+sn+'_'+ana+'_sqw'

		verbOp = self.getProperty('Verbose')
		plotOp = self.getProperty('Plot')
		saveOp = self.getProperty('Save')
		Main.MomentStart(sam,verbOp,plotOp,saveOp)

mantid.registerPyAlgorithm(Moments())         # Register algorithm with Mantid
