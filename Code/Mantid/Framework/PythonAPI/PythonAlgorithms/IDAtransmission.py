# Algorithm to start Bayes programs
from MantidFramework import *
from mantidsimple import *
from mantidplotpy import *
import IndirectTrans as Main

class IDAtransmission(PythonAlgorithm):
 
	def PyInit(self):
		self.declareProperty(Name='Instrument',DefaultValue='IRIS',Validator=ListValidator(['IRIS','OSIRIS']))
		self.declareProperty(Name='SamNumber',DefaultValue='',Validator=MandatoryValidator(),Description = 'Sample run number')
		self.declareProperty(Name='CanNumber',DefaultValue='',Validator=MandatoryValidator(),Description = 'Resolution run number')
		self.declareProperty('Verbose',DefaultValue='Yes',Validator=ListValidator(['Yes','No']))
		self.declareProperty('Plot',DefaultValue='Yes',Validator=ListValidator(['Yes','No']))
 
	def PyExec(self):

		self.log().information('IDA transmission input')
		instr = self.getPropertyValue('Instrument')
		if instr == 'IRIS':
			prefix = 'irs'
		if instr == 'OSIRIS':
			prefix = 'osi'
		sn = self.getPropertyValue('SamNumber')
		cn = self.getPropertyValue('CanNumber')

		sam = prefix+sn
		can = prefix+cn

		verbOp = self.getPropertyValue('Verbose')
		plotOp = self.getPropertyValue('Plot')
		Main.IDATransStart(sam,can,verbOp,plotOp)

mantid.registerPyAlgorithm(IDAtransmission())         # Register algorithm with Mantid
