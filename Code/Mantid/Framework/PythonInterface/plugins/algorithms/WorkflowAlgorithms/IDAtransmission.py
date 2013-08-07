# Algorithm to start Bayes programs
from mantid.api import PythonAlgorithm, AlgorithmFactory
from mantid.kernel import StringListValidator, StringMandatoryValidator

class IDAtransmission(PythonAlgorithm):
 
	def category(self):
		return "Workflow\\MIDAS;PythonAlgorithms"

	def PyInit(self):
		self.declareProperty(name='Instrument',defaultValue='iris',validator=StringListValidator(['irs','iris','osi','osiris']), doc='Instrument')
		self.declareProperty(name='SamNumber',defaultValue='',validator=StringMandatoryValidator(), doc='Sample run number')
		self.declareProperty(name='CanNumber',defaultValue='',validator=StringMandatoryValidator(), doc='Resolution run number')
		self.declareProperty('Verbose',defaultValue='Yes',validator=StringListValidator(['Yes','No']))
		self.declareProperty('Plot',defaultValue='Yes',validator=StringListValidator(['Yes','No']))
		self.declareProperty('Save',defaultValue=False, doc='Switch Save result to nxs file Off/On')
 
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

AlgorithmFactory.subscribe(IDAtransmission)         # Register algorithm with Mantid
