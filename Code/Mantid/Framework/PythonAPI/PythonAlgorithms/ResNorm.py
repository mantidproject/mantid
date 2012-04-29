# Algorithm to start Bayes programs
from MantidFramework import *
from mantidsimple import *
from mantidplotpy import *
from IndirectCommon import runF2PyCheck, inF2PyCompatibleEnv
if inF2PyCompatibleEnv():
	import IndirectBayes as Main

class ResNorm(PythonAlgorithm):
 
	def PyInit(self):
		self.declareProperty(Name='Instrument',DefaultValue='IRIS',Validator=ListValidator(['IRIS','OSIRIS']))
		self.declareProperty(Name='Analyser',DefaultValue='graphite002',Validator=ListValidator(['graphite002','graphite004']))
		self.declareProperty(Name='VanNumber',DefaultValue='',Validator=MandatoryValidator(),Description = 'Sample run number')
		self.declareProperty(Name='ResNumber',DefaultValue='',Validator=MandatoryValidator(),Description = 'Resolution run number')
		self.declareProperty(Name='EnergyMin', DefaultValue=-0.2)
		self.declareProperty(Name='EnergyMax', DefaultValue=0.2)
		self.declareProperty(Name='VanBinning', DefaultValue=1,Description = 'Binning value for sample')
		self.declareProperty(Name='Verbose',DefaultValue='Yes',Validator=ListValidator(['Yes','No']))
		self.declareProperty('Plot',DefaultValue='None',Validator=ListValidator(['None','Intensity','Stretch','Fit','All']))
 
	def PyExec(self):
		runF2PyCheck()
		
		self.log().information('ResNorm input')
		instr = self.getPropertyValue('Instrument')
		if instr == 'IRIS':
			prefix = 'irs'
		if instr == 'OSIRIS':
			prefix = 'osi'
		ana = self.getPropertyValue('Analyser')
		van = self.getPropertyValue('VanNumber')
		res = self.getPropertyValue('ResNumber')
		emin = self.getPropertyValue('EnergyMin')
		emax = self.getPropertyValue('EnergyMax')
		nbin = self.getPropertyValue('VanBinning')

		vname = prefix+van
		rname = prefix+res
		erange = [emin, emax]
		verbOp = self.getPropertyValue('Verbose')
		plotOp = self.getPropertyValue('Plot')
		Main.ResNormStart(ana,vname,rname,erange,nbin,verbOp,plotOp)

mantid.registerPyAlgorithm(ResNorm())         # Register algorithm with Mantid
