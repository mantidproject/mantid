# Algorithm to start Bayes programs
from MantidFramework import *

class IDAtransmission(PythonAlgorithm):
 
	def category(self):
		return "Workflow\\MIDAS;PythonAlgorithms"

	def PyInit(self):
		self.declareProperty(Name='Instrument',DefaultValue='iris',Validator=ListValidator(['irs','iris','osi','osiris']),Description = 'Instrument')
		self.declareProperty(Name='SamNumber',DefaultValue='',Validator=MandatoryValidator(),Description = 'Sample run number')
		self.declareProperty(Name='CanNumber',DefaultValue='',Validator=MandatoryValidator(),Description = 'Resolution run number')
		self.declareProperty('Verbose',DefaultValue='Yes',Validator=ListValidator(['Yes','No']))
		self.declareProperty('Plot',DefaultValue='Yes',Validator=ListValidator(['Yes','No']))
		self.declareProperty('Save',DefaultValue=False,Description = 'Switch Save result to nxs file Off/On')
 
	def PyExec(self):

		self.log().information('IDA transmission input')
		prefix = self.getPropertyValue('Instrument')
		sn = self.getPropertyValue('SamNumber')
		cn = self.getPropertyValue('CanNumber')

		sam = prefix+sn
		can = prefix+cn

		verbOp = self.getPropertyValue('Verbose')
		plotOp = self.getPropertyValue('Plot')
		saveOp = self.getProperty('Save')
		from IndirectEnergyConversion import IndirectTrans
		IndirectTrans(sam,can,verbOp,plotOp,saveOp)

mantid.registerPyAlgorithm(IDAtransmission())         # Register algorithm with Mantid
