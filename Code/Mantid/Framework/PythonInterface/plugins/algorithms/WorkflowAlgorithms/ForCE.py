# Algorithm to start Force
from mantid.api import PythonAlgorithm, AlgorithmFactory
from mantid.kernel import StringListValidator, StringMandatoryValidator, logger

class ForCE(PythonAlgorithm):
 
	def category(self):
		return "Workflow\\MIDAS;PythonAlgorithms"

	def PyInit(self):
		self.declareProperty('Mode','ASCII',StringListValidator(['ASCII','INX']), doc='Ascii format type')
		self.declareProperty('Instrument','IN10',StringListValidator(['IN10','IN16']), doc='Instrument name')
		self.declareProperty('Analyser','silicon',StringListValidator(['silicon']), doc='Analyser crystal')
		self.declareProperty('Reflection','111',StringListValidator(['111']), doc='Analyuser reflection')
		self.declareProperty('RunName', defaultValue='', validator = StringMandatoryValidator(), doc='Run name (after <Instr>_)')
		self.declareProperty(name='RejectZero',defaultValue=False, doc='Reject spectra with zero total count')
		self.declareProperty(name='UseMap',defaultValue=False, doc='Use detector map')
		self.declareProperty(name='Verbose',defaultValue=True, doc='Switch Verbose Off/On')
		self.declareProperty(name='Save',defaultValue=False, doc='Switch Save result to nxs file Off/On')
		self.declareProperty(name='Plot',defaultValue='None',validator=StringListValidator(['None','Spectrum','Contour','Both']), doc='Plot options')
 
	def PyExec(self):

		logger.information('ForCE input')
		mode = self.getPropertyValue('Mode')
		instr = self.getPropertyValue('Instrument')
		ana = self.getPropertyValue('Analyser')
		refl = self.getPropertyValue('Reflection')
		run = self.getPropertyValue('RunName')
		rejectZ = self.getProperty('RejectZero').value
		useM = self.getProperty('UseMap').value
		verbOp = self.getProperty('Verbose').value
		saveOp = self.getProperty('Save').value
		plotOp = self.getPropertyValue('Plot')

                from IndirectForce import IbackStart, InxStart
		if mode == 'ASCII':
			IbackStart(instr,run,ana,refl,rejectZ,useM,verbOp,plotOp,saveOp)
		if mode == 'INX':
			InxStart(instr,run,ana,refl,rejectZ,useM,verbOp,plotOp,saveOp)
 
AlgorithmFactory.subscribe(ForCE)                    # Register algorithm with Mantid
