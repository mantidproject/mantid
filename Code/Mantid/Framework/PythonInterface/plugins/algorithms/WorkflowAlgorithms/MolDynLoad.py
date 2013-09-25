# Algorithm to start Force
from MantidFramework import *
from MolDynTransfer import MolDynImport

class MolDynLoad(PythonAlgorithm):
 
	def category(self):
		return "Workflow\\MIDAS;PythonAlgorithms"

	def PyInit(self):
		self.declareProperty('FileName', DefaultValue='', Validator = MandatoryValidator(),Description = 'File name (cdl ext)')
		self.declareProperty('FunctionNames', DefaultValue='', Validator = MandatoryValidator(),Description = 'Function names - comma separated(eg Fqt Sqw)')
		self.declareProperty(Name='Verbose',DefaultValue=True,Description = 'Switch Verbose Off/On')
		self.declareProperty(Name='Save',DefaultValue=False,Description = 'Switch Save result to nxs file Off/On')
		self.declareProperty(Name='Plot',DefaultValue='None',Validator=ListValidator(['None','Spectrum','Contour','Both']),Description = 'Plot options')
 
	def PyExec(self):

		self.log().information('MolDynLoad input')
		file = self.getPropertyValue('FileName')
		functions = self.getPropertyValue('FunctionNames')
		func = functions.split(',')
		verbOp = self.getProperty('Verbose')
		saveOp = self.getProperty('Save')
		plotOp = self.getPropertyValue('Plot')

		MolDynImport(file,func,verbOp,plotOp,saveOp)
 
mantid.registerPyAlgorithm(MolDynLoad())                    # Register algorithm with Mantid
