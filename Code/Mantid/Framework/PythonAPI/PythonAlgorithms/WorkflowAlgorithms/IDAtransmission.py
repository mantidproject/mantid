# Algorithm to start Bayes programs
from MantidFramework import *
class IDAtransmission(PythonAlgorithm):
 
	def category(self):
		return "Workflow\\MIDAS;PythonAlgorithms"

	def PyInit(self):
		self.declareProperty(Name='Instrument',DefaultValue='iris',Validator=ListValidator(['irs','iris','osi','osiris']),Description = 'Instrument')
		self.declareProperty(Name='SamNumber',DefaultValue='',Validator=MandatoryValidator(),Description = 'Sample run number')
		self.declareProperty(Name='CanNumber',DefaultValue='',Validator=MandatoryValidator(),Description = 'Resolution run number')
		self.declareProperty('Verbose',DefaultValue=False, Description = 'Switch to show verbose output of algorithm')
		self.declareProperty('Plot',DefaultValue=False, Description = 'Switch to plot output of algorithm')
		self.declareProperty('Save',DefaultValue=False,Description = 'Switch Save result to nxs file Off/On')
 
	def PyExec(self):

		self.log().information('IDA transmission input')
		prefix = self.getPropertyValue('Instrument')
		sn = self.getPropertyValue('SamNumber')
		cn = self.getPropertyValue('CanNumber')

		verbOp = self.getProperty('Verbose')
		plotOp = self.getProperty('Plot')
		saveOp = self.getProperty('Save')
		from IndirectEnergyConversion import IndirectTrans
		IndirectTrans(prefix,sn,cn,Verbose=verbOp,Plot=plotOp,Save=saveOp)

mantid.registerPyAlgorithm(IDAtransmission())         # Register algorithm with Mantid
