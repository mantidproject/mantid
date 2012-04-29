# Algorithm to start Force
from MantidFramework import *
import IndirectForce as Main

class ForCE(PythonAlgorithm):
 
	def PyInit(self):
		self.declareProperty('Mode','ASCII',ListValidator(['ASCII','INX']))
		self.declareProperty('Instrument','IN10',ListValidator(['IN10','IN16']))
		self.declareProperty('RunName', DefaultValue='', Validator = MandatoryValidator())
		self.declareProperty(Name='Verbose',DefaultValue='Yes',Validator=ListValidator(['No','Yes']))
		self.declareProperty(Name='Plot',DefaultValue='None',Validator=ListValidator(['None','Spectrum','Contour','Both']))
 
	def PyExec(self):

		self.log().information('ForCE input')
		mode = self.getPropertyValue('Mode')
		instr = self.getPropertyValue('Instrument')
		run = self.getPropertyValue('RunName')
		verbOp = self.getPropertyValue('Verbose')
		plotOp = self.getPropertyValue('Plot')

		if mode == 'ASCII':
			Main.IbackStart(instr,run,verbOp,plotOp)
		if mode == 'INX':
			Main.InxStart(instr,run,verbOp,plotOp)
 
mantid.registerPyAlgorithm(ForCE())                    # Register algorithm with Mantid
