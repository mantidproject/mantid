# Algorithm to start Bayes programs
from mantid.simpleapi import *
from mantid.api import PythonAlgorithm, AlgorithmFactory
from mantid.kernel import StringListValidator, StringMandatoryValidator
from mantid import config
import os.path

class Moments(PythonAlgorithm):
 
	def category(self):
		return "Workflow\\MIDAS;PythonAlgorithms"

	def PyInit(self):
		self.declareProperty(name='InputType',defaultValue='File',validator=StringListValidator(['File','Workspace']), doc='Origin of data input - File or Workspace')
		self.declareProperty(name='Instrument',defaultValue='iris',validator=StringListValidator(['irs','iris','osi','osiris']), doc='Instrument')
		self.declareProperty(name='Analyser',defaultValue='graphite002',validator=StringListValidator(['graphite002','graphite004']), doc='Analyser & reflection')
		self.declareProperty(name='SamNumber',defaultValue='',validator=StringMandatoryValidator(), doc='Sample run number')
		self.declareProperty(name='EnergyMin', defaultValue=-0.5, doc='Minimum energy for fit. Default=-0.5')
		self.declareProperty(name='EnergyMax', defaultValue=0.5, doc='Maximum energy for fit. Default=0.5')
		self.declareProperty(name='MultiplyBy', defaultValue=1.0, doc='Scale factor to multiply S(Q,w). Default=1.0')
		self.declareProperty('Verbose',defaultValue=True, doc='Switch Verbose Off/On')
		self.declareProperty('Plot',defaultValue=True, doc='Switch Plot Off/On')
		self.declareProperty('Save',defaultValue=False, doc='Switch Save result to nxs file Off/On')
 
	def PyExec(self):

		self.log().information('Moments calculation')
		inType = self.getPropertyValue('InputType')
		prefix = self.getPropertyValue('Instrument')
		ana = self.getPropertyValue('Analyser')
		sn = self.getPropertyValue('SamNumber')
		sam = prefix+sn+'_'+ana+'_sqw'
		emin = self.getPropertyValue('EnergyMin')
		emax = self.getPropertyValue('EnergyMax')
		erange = [float(emin), float(emax)]
		factor = self.getPropertyValue('MultiplyBy')
		factor = float(factor)

		verbOp = self.getProperty('Verbose').value
		plotOp = self.getProperty('Plot').value
		saveOp = self.getProperty('Save').value
		workdir = config['defaultsave.directory']
		if inType == 'File':
			spath = os.path.join(workdir, sam+'.nxs')		# path name for sample nxs file
			self.log().notice('Input from File : '+spath)
			LoadNexus(Filename=spath, OutputWorkspace=sam)
		else:
			self.log().notice('Input from Workspace : '+sam)
		from IndirectEnergyConversion import SqwMoments
		SqwMoments(sam,erange,factor,verbOp,plotOp,saveOp)

AlgorithmFactory.subscribe(Moments)         # Register algorithm with Mantid
