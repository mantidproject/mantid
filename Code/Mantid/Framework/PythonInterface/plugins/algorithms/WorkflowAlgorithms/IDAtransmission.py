# Algorithm to start Bayes programs
from mantid.api import PythonAlgorithm, AlgorithmFactory
from mantid.kernel import StringListValidator, StringMandatoryValidator

class IDAtransmission(PythonAlgorithm):
 
	def category(self):
		return "Workflow\\MIDAS;PythonAlgorithms"

	def PyInit(self):
		self.declareProperty(name='Instrument',defaultValue='', doc='Instrument')
		self.declareProperty(name='SamNumber',defaultValue='',validator=StringMandatoryValidator(), doc='Sample run number')
		self.declareProperty(name='CanNumber',defaultValue='',validator=StringMandatoryValidator(), doc='Resolution run number')
		self.declareProperty('Verbose',defaultValue=False, doc="Switch to show verbose output of algorithm")
		self.declareProperty('Plot',defaultValue=False, doc="Switch to plot output of algorithm")
		self.declareProperty('Save',defaultValue=False, doc='Switch Save result to nxs file')
 
	def PyExec(self):

		self.log().information('IDA transmission input')
		inst = self.getPropertyValue('Instrument')
		sam = self.getPropertyValue('SamNumber')
		can = self.getPropertyValue('CanNumber')

		verbOp = self.getProperty('Verbose').value
		plotOp = self.getProperty('Plot').value
		saveOp = self.getProperty('Save').value
		
		from IndirectEnergyConversion import IndirectTrans
		IndirectTrans(inst,sam,can,Verbose=verbOp,Plot=plotOp,Save=saveOp)

AlgorithmFactory.subscribe(IDAtransmission)         # Register algorithm with Mantid
