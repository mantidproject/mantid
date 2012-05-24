# Algorithm to start Force
from MantidFramework import *
import IndirectForce as Main

class ForCE(PythonAlgorithm):
 
	def PyInit(self):
		self.declareProperty('Mode','ASCII',ListValidator(['ASCII','INX']),Description = 'Ascii format type')
		self.declareProperty('Instrument','IN10',ListValidator(['IN10','IN16']),Description = 'Instrument name')
		self.declareProperty('RunName', DefaultValue='', Validator = MandatoryValidator(),Description = 'Run name (after <Instr>_)')
		self.declareProperty(Name='RejectZero',DefaultValue=False,Description = 'Reject spectra with zero total count')
		self.declareProperty(Name='UseMap',DefaultValue=False,Description = 'Use detector map')
		self.declareProperty(Name='Verbose',DefaultValue=True,Description = 'Switch Verbose Off/On')
		self.declareProperty(Name='Save',DefaultValue=False,Description = 'Switch Save result to nxs file Off/On')
		self.declareProperty(Name='Plot',DefaultValue='None',Validator=ListValidator(['None','Spectrum','Contour','Both']),Description = 'Plot options')
 
	def PyExec(self):

		self.log().information('ForCE input')
		mode = self.getPropertyValue('Mode')
		instr = self.getPropertyValue('Instrument')
		run = self.getPropertyValue('RunName')
		rejectZ = self.getProperty('RejectZero')
		useM = self.getProperty('UseMap')
		verbOp = self.getProperty('Verbose')
		saveOp = self.getProperty('Save')
		plotOp = self.getPropertyValue('Plot')

		if mode == 'ASCII':
			Main.IbackStart(instr,run,rejectZ,useM,verbOp,plotOp,saveOp)
		if mode == 'INX':
			Main.InxStart(instr,run,rejectZ,useM,verbOp,plotOp,saveOp)
 
mantid.registerPyAlgorithm(ForCE())                    # Register algorithm with Mantid
