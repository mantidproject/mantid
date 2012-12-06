# Algorithm to start Jump fit programs
from MantidFramework import *
from mantid.simpleapi import *
from mantid import config, logger, mtd
from IndirectImport import run_f2py_compatibility_test, is_supported_f2py_platform

if is_supported_f2py_platform():
	import IndirectBayes as Main

class JumpFit(PythonAlgorithm):
 
	def category(self):
		return "Workflow\\MIDAS;PythonAlgorithms"

	def PyInit(self):
		self.declareProperty(Name='InputType',DefaultValue='File',Validator=ListValidator(['File','Workspace']),Description = 'Origin of data input - File (*.nxs) or Workspace')
		self.declareProperty(Name='Instrument',DefaultValue='iris',Validator=ListValidator(['irs','iris','osi','osiris']),Description = 'Instrument')
		self.declareProperty(Name='Analyser',DefaultValue='graphite002',Validator=ListValidator(['graphite002','graphite004']),Description = 'Analyser & reflection')
		self.declareProperty(Name='QLprogram',DefaultValue='QLr',Validator=ListValidator(['QLr','QLd']),Description = 'QL output from Res or Data')
		self.declareProperty(Name='Fit',DefaultValue='CE',Validator=ListValidator(['CE','SS']),Description = 'Chudley-Elliott or Singwi-Sjolander')
		self.declareProperty(Name='Width',DefaultValue='FW11',Validator=ListValidator(['FW11','FW21','FW22']),Description = 'FullWidth type')
		self.declareProperty(Name='SamNumber',DefaultValue='',Validator=MandatoryValidator(),Description = 'Sample run number')
		self.declareProperty(Name='CropQ',DefaultValue=False,Description = 'Switch CropQ Off/On')
		self.declareProperty(Name='Qmin',DefaultValue='',Description = 'Qmin for cropping')
		self.declareProperty(Name='Qmax',DefaultValue='',Description = 'Qmax for cropping')
		self.declareProperty(Name='Plot',DefaultValue=False,Description = 'Switch Plot Off/On')
		self.declareProperty(Name='Verbose',DefaultValue=True,Description = 'Switch Verbose Off/On')
		self.declareProperty(Name='Save',DefaultValue=False,Description = 'Switch Save result to nxs file Off/On')
 
	def PyExec(self):
		run_f2py_compatibility_test()
		
		self.log().information('Jump input')
		inType = self.getPropertyValue('InputType')
		prefix = self.getPropertyValue('Instrument')
		ana = self.getPropertyValue('Analyser')
		prog = self.getPropertyValue('QLprogram')
		jump = self.getPropertyValue('Fit')
		fw = self.getPropertyValue('Width')
		sam = self.getPropertyValue('SamNumber')

		sname = prefix+sam+'_'+ana+'_'+prog
		cropOp = self.getProperty('CropQ')
		if cropOp:
			qmin = self.getPropertyValue('Qmin')
			qmax = self.getPropertyValue('Qmax')
			qrange = [qmin, qmax]
		else:
			qrange = [0.0, 5.0]
		verbOp = self.getProperty('Verbose')
		plotOp = self.getProperty('Plot')
		saveOp = self.getProperty('Save')

		workdir = config['defaultsave.directory']
		if inType == 'File':
			path = os.path.join(workdir, sname+'_Parameters.nxs')					# path name for nxs file
			LoadNexusProcessed(Filename=path, OutputWorkspace=sname+'_Parameters')
			message = 'Input from File : '+path
		else:
			message = 'Input from Workspace : '+sname
		if verbOp:
			logger.notice(message)
		Main.JumpRun(sname,jump,prog,fw,cropOp,qrange,verbOp,plotOp,saveOp)

mantid.registerPyAlgorithm(JumpFit())         # Register algorithm with Mantid
