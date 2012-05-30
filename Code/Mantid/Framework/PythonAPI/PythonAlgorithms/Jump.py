# Algorithm to start Jump fit programs
from MantidFramework import *
from IndirectCommon import runF2PyCheck, inF2PyCompatibleEnv

if inF2PyCompatibleEnv():
	import IndirectBayes as Main

class Jump(PythonAlgorithm):
 
	def PyInit(self):
		self.declareProperty(Name='Instrument',DefaultValue='IRIS',Validator=ListValidator(['IRIS','OSIRIS']),Description = 'Instrument name')
		self.declareProperty(Name='Analyser',DefaultValue='graphite002',Validator=ListValidator(['graphite002','graphite004']),Description = 'Analyser & reflection')
		self.declareProperty(Name='QLprogram',DefaultValue='QLr',Validator=ListValidator(['QLr','QLd']),Description = 'QL output from Res or Data')
		self.declareProperty(Name='Fit',DefaultValue='CE',Validator=ListValidator(['CE','SS']),Description = 'Chudley-Elliott or Singwi-Sjolander')
		self.declareProperty(Name='Width',DefaultValue='FW11',Validator=ListValidator(['FW11','FW21','FW22']),Description = 'FullWidth type')
		self.declareProperty(Name='SamNumber',DefaultValue='',Validator=MandatoryValidator(),Description = 'Sample run number')
		self.declareProperty(Name='Plot',DefaultValue=False,Description = 'Switch Plot Off/On')
		self.declareProperty(Name='Verbose',DefaultValue=True,Description = 'Switch Verbose Off/On')
		self.declareProperty(Name='Save',DefaultValue=False,Description = 'Switch Save result to nxs file Off/On')
 
	def PyExec(self):
		runF2PyCheck()
		
		self.log().information('Jump input')
		instr = self.getPropertyValue('Instrument')
		if instr == 'IRIS':
			prefix = 'irs'
		if instr == 'OSIRIS':
			prefix = 'osi'
		ana = self.getPropertyValue('Analyser')
		prog = self.getPropertyValue('QLprogram')
		jump = self.getPropertyValue('Fit')
		fw = self.getPropertyValue('Width')
		sam = self.getPropertyValue('SamNumber')

		sname = prefix+sam+'_'+ana+'_'+prog
		verbOp = self.getProperty('Verbose')
		plotOp = self.getProperty('Plot')
		saveOp = self.getProperty('Save')
		Main.JumpStart(sname,jump,prog,fw,verbOp,plotOp,saveOp)

mantid.registerPyAlgorithm(Jump())         # Register algorithm with Mantid
