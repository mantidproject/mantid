# Algorithm to start Jump fit programs
from mantid.api import PythonAlgorithm, AlgorithmFactory
from mantid.kernel import StringListValidator, StringMandatoryValidator
from mantid.simpleapi import *
from mantid import config, logger, mtd
import os

class JumpFit(PythonAlgorithm):
 
	def category(self):
		return "Workflow\\MIDAS;PythonAlgorithms"

	def PyInit(self):
		self.declareProperty(name='InputType',defaultValue='File',validator=StringListValidator(['File','Workspace']), doc='Origin of data input - File (*.nxs) or Workspace')
		self.declareProperty(name='Instrument',defaultValue='iris',validator=StringListValidator(['irs','iris','osi','osiris']), doc='Instrument')
		self.declareProperty(name='Analyser',defaultValue='graphite002',validator=StringListValidator(['graphite002','graphite004']), doc='Analyser & reflection')
		self.declareProperty(name='QLprogram',defaultValue='QLr',validator=StringListValidator(['QLr','QLd']), doc='QL output from Res or Data')
		self.declareProperty(name='Fit',defaultValue='CE',validator=StringListValidator(['CE','SS']), doc='Chudley-Elliott or Singwi-Sjolander')
		self.declareProperty(name='Width',defaultValue='FW11',validator=StringListValidator(['FW11','FW21','FW22']), doc='FullWidth type')
		self.declareProperty(name='SamNumber',defaultValue='',validator=StringMandatoryValidator(), doc='Sample run number')
		self.declareProperty(name='CropQ',defaultValue=False, doc='Switch CropQ Off/On')
		self.declareProperty(name='Qmin',defaultValue='', doc='Qmin for cropping')
		self.declareProperty(name='Qmax',defaultValue='', doc='Qmax for cropping')
		self.declareProperty(name='Plot',defaultValue=False, doc='Switch Plot Off/On')
		self.declareProperty(name='Verbose',defaultValue=True, doc='Switch Verbose Off/On')
		self.declareProperty(name='Save',defaultValue=False, doc='Switch Save result to nxs file Off/On')
 
	def PyExec(self):
                from IndirectImport import run_f2py_compatibility_test, is_supported_f2py_platform

                if is_supported_f2py_platform():
                        import IndirectBayes as Main

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
		cropOp = self.getProperty('CropQ').value
		if cropOp:
			qmin = self.getPropertyValue('Qmin')
			qmax = self.getPropertyValue('Qmax')
			qrange = [qmin, qmax]
		else:
			qrange = [0.0, 5.0]
		verbOp = self.getProperty('Verbose').value
		plotOp = self.getProperty('Plot').value
		saveOp = self.getProperty('Save').value

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

AlgorithmFactory.subscribe(JumpFit)         # Register algorithm with Mantid
