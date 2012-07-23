# Algorithm to start Bayes programs
from MantidFramework import *
from IndirectImport import run_f2py_compatibility_test, is_supported_f2py_platform

if is_supported_f2py_platform():
	import IndirectBayes as Main

class ResNorm(PythonAlgorithm):
 
	def category(self):
		return "Workflow\\MIDAS;PythonAlgorithms"

	def PyInit(self):
		self.declareProperty(Name='InputType',DefaultValue='File',Validator=ListValidator(['File','Workspace']),Description = 'Origin of data input - File (*.nxs) or Workspace')
		self.declareProperty(Name='Instrument',DefaultValue='IRIS',Validator=ListValidator(['IRIS','OSIRIS']),Description = 'Instrument')
		self.declareProperty(Name='Analyser',DefaultValue='graphite002',Validator=ListValidator(['graphite002','graphite004']),Description = 'Analyser & reflection')
		self.declareProperty(Name='VanNumber',DefaultValue='',Validator=MandatoryValidator(),Description = 'Sample run number')
		self.declareProperty(Name='ResInputType',DefaultValue='File',Validator=ListValidator(['File','Workspace']),Description = 'Origin of res input - File (*_res.nxs) or Workspace')
		self.declareProperty(Name='ResNumber',DefaultValue='',Validator=MandatoryValidator(),Description = 'Resolution run number')
		self.declareProperty(Name='EnergyMin', DefaultValue=-0.2,Description = 'Minimum energy for fit. Default=-0.2')
		self.declareProperty(Name='EnergyMax', DefaultValue=0.2,Description = 'Maximum energy for fit. Default=0.2')
		self.declareProperty(Name='VanBinning', DefaultValue=1,Description = 'Binning value (integer) for sample. Default=1')
		self.declareProperty(Name='Plot',DefaultValue='None',Validator=ListValidator(['None','Intensity','Stretch','Fit','All']),Description = 'Plot options')
		self.declareProperty(Name='Verbose',DefaultValue=True,Description = 'Switch Verbose Off/On')
		self.declareProperty(Name='Save',DefaultValue=False,Description = 'Switch Save result to nxs file Off/On')
 
	def PyExec(self):
		run_f2py_compatibility_test()
		
		self.log().information('ResNorm input')
		inType = self.getPropertyValue('InputType')
		instr = self.getPropertyValue('Instrument')
		if instr == 'IRIS':
			prefix = 'irs'
		if instr == 'OSIRIS':
			prefix = 'osi'
		ana = self.getPropertyValue('Analyser')
		van = self.getPropertyValue('VanNumber')
		rinType = self.getPropertyValue('ResInputType')
		res = self.getPropertyValue('ResNumber')
		emin = self.getPropertyValue('EnergyMin')
		emax = self.getPropertyValue('EnergyMax')
		nbin = self.getPropertyValue('VanBinning')

		vname = prefix+van+'_'+ana
		rname = prefix+res+'_'+ana
		erange = [float(emin), float(emax)]
		verbOp = self.getProperty('Verbose')
		plotOp = self.getPropertyValue('Plot')
		saveOp = self.getProperty('Save')
		Main.ResNormStart(inType,vname,rinType,rname,erange,nbin,verbOp,plotOp,saveOp)

mantid.registerPyAlgorithm(ResNorm())         # Register algorithm with Mantid
