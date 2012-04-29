# Algorithm to start Jump fit programs
from MantidFramework import *
from mantidsimple import *
from mantidplotpy import *
from IndirectCommon import runF2PyCheck, inF2PyCompatibleEnv
if inF2PyCompatibleEnv():
	import IndirectBayes as Main

class Jump(PythonAlgorithm):
 
	def PyInit(self):
		self.declareProperty(Name='Instrument',DefaultValue='IRIS',Validator=ListValidator(['IRIS','OSIRIS']))
		self.declareProperty(Name='QLprogram',DefaultValue='QLr',Validator=ListValidator(['QLr','QLd']))
		self.declareProperty(Name='Fit',DefaultValue='CE',Validator=ListValidator(['CE','SS']))
		self.declareProperty(Name='Width',DefaultValue='FW11',Validator=ListValidator(['FW11','FW21','FW22']))
		self.declareProperty(Name='SamNumber',DefaultValue='',Validator=MandatoryValidator(),Description = 'Sample run number')
		self.declareProperty(Name='Verbose',DefaultValue='Yes',Validator=ListValidator(['Yes','No']))
		self.declareProperty('Plot',DefaultValue='No',Validator=ListValidator(['Yes','No']))
 
	def PyExec(self):
		runF2PyCheck()
		
		self.log().information('Jump input')
		instr = self.getPropertyValue('Instrument')
		if instr == 'IRIS':
			prefix = 'irs'
		if instr == 'OSIRIS':
			prefix = 'osi'
		prog = self.getPropertyValue('QLprogram')
		jump = self.getPropertyValue('Fit')
		fw = self.getPropertyValue('Width')
		sam = self.getPropertyValue('SamNumber')

		sname = prefix+sam
		verbOp = self.getPropertyValue('Verbose')
		plotOp = self.getPropertyValue('Plot')
		Main.JumpStart(sname,jump,prog,fw,verbOp,plotOp)

mantid.registerPyAlgorithm(Jump())         # Register algorithm with Mantid
