# Algorithm to start Bayes programs
from MantidFramework import *
from mantid.simpleapi import *
from mantid import config, logger, mtd
from IndirectImport import run_f2py_compatibility_test, is_supported_f2py_platform

if is_supported_f2py_platform():
	import IndirectBayes as Main

class ResNorm(PythonAlgorithm):
 
	def category(self):
		return "Workflow\\MIDAS;PythonAlgorithms"

	def PyInit(self):
		self.declareProperty(Name='InputType',DefaultValue='File',Validator=ListValidator(['File','Workspace']),Description = 'Origin of data input - File (*.nxs) or Workspace')
		self.declareProperty(Name='Instrument',DefaultValue='iris',Validator=ListValidator(['irs','iris','osi','osiris']),Description = 'Instrument')
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
		prefix = self.getPropertyValue('Instrument')
		ana = self.getPropertyValue('Analyser')
		van = self.getPropertyValue('VanNumber')
		rinType = self.getPropertyValue('ResInputType')
		res = self.getPropertyValue('ResNumber')
		emin = self.getPropertyValue('EnergyMin')
		emax = self.getPropertyValue('EnergyMax')
		nbin = self.getPropertyValue('VanBinning')

		vname = prefix+van+'_'+ana+ '_red'
		rname = prefix+res+'_'+ana+ '_res'
		erange = [float(emin), float(emax)]
		verbOp = self.getProperty('Verbose')
		plotOp = self.getPropertyValue('Plot')
		saveOp = self.getProperty('Save')

		workdir = config['defaultsave.directory']
		if inType == 'File':
			vpath = os.path.join(workdir, vname+'.nxs')		           # path name for van nxs file
			LoadNexusProcessed(Filename=vpath, OutputWorkspace=vname)
			Vmessage = 'Vanadium from File : '+vpath
		else:
			Vmessage = 'Vanadium from Workspace : '+vname
		if rinType == 'File':
			rpath = os.path.join(workdir, rname+'.nxs')                # path name for res nxs file
			LoadNexusProcessed(Filename=rpath, OutputWorkspace=rname)
			Rmessage = 'Resolution from File : '+rpath
		else:
			Rmessage = 'Resolution from Workspace : '+rname
		if verbOp:
			logger.notice(Vmessage)
			logger.notice(Rmessage)
		Main.ResNormRun(vname,rname,erange,nbin,verbOp,plotOp,saveOp)

mantid.registerPyAlgorithm(ResNorm())         # Register algorithm with Mantid
