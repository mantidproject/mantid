# Algorithm to start Symmetrise
from MantidFramework import *
import IndirectSymm as Main

class Symmetrise(PythonAlgorithm):
 
	def category(self):
		return "Workflow\\MIDAS;PythonAlgorithms"

	def PyInit(self):
		self.declareProperty(Name='InputType',DefaultValue='File',Validator=ListValidator(['File','Workspace']),Description = 'Origin of data input - File (_red.nxs) or Workspace')
		self.declareProperty(Name='Instrument',DefaultValue='IRIS',Validator=ListValidator(['IRIS','OSIRIS']),Description = 'Instrument')
		self.declareProperty(Name='Analyser',DefaultValue='graphite002',Validator=ListValidator(['graphite002','graphite004']),Description = 'Analyser & reflection')
		self.declareProperty(Name='SamNumber',DefaultValue='',Validator=MandatoryValidator(),Description = 'Sample run number')
		self.declareProperty(Name='Xcut',DefaultValue='',Validator=MandatoryValidator(),Description = 'X cutoff value')
		self.declareProperty('Verbose',DefaultValue=True,Description = 'Switch Verbose Off/On')
		self.declareProperty('Plot',DefaultValue=True,Description = 'Switch Plot Off/On')
		self.declareProperty('Save',DefaultValue=False,Description = 'Switch Save result to nxs file Off/On')
 
	def PyExec(self):

		self.log().information('Symmetrise')
		inType = self.getPropertyValue('InputType')
		instr = self.getPropertyValue('Instrument')
		if instr == 'IRIS':
			prefix = 'irs'
		if instr == 'OSIRIS':
			prefix = 'osi'
		ana = self.getPropertyValue('Analyser')
		sn = self.getPropertyValue('SamNumber')
		sam = prefix+sn+'_'+ana+'_red'
		cut = self.getPropertyValue('Xcut')
		cut = float(cut)

		verbOp = self.getProperty('Verbose')
		plotOp = self.getProperty('Plot')
		saveOp = self.getProperty('Save')
		Main.SymmStart(inType,sam,cut,verbOp,plotOp,saveOp)

mantid.registerPyAlgorithm(Symmetrise())         # Register algorithm with Mantid
