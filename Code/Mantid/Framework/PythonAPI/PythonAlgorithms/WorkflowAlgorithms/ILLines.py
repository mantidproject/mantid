# Algorithm to start Bayes programs
from MantidFramework import *
from IndirectImport import run_f2py_compatibility_test, is_supported_f2py_platform

if is_supported_f2py_platform():
	import IndirectBayes as Main

class ILLines(PythonAlgorithm):
 
	def category(self):
		return "Workflow\\MIDAS;PythonAlgorithms"

	def PyInit(self):
		self.declareProperty(Name='InputType',DefaultValue='File',Validator=ListValidator(['File','Workspace']),Description = 'Origin of data input - File (*.nxs) or Workspace')
		self.declareProperty(Name='Instrument',DefaultValue='IN10',Validator=ListValidator(['IN10','IN16']),Description = 'Instrument')
		self.declareProperty(Name='Program',DefaultValue='QL',Validator=ListValidator(['QL','QSe']),Description = 'Name of program to run')
		self.declareProperty(Name='SamNumber',DefaultValue='',Validator=MandatoryValidator(),Description = 'Sample run number')
		self.declareProperty(Name='ResInputType',DefaultValue='File',Validator=ListValidator(['File','Workspace']),Description = 'Origin of res input - File (*_res.nxs) or Workspace')
		self.declareProperty(Name='ResType',DefaultValue='Res',Validator=ListValidator(['Res','Data']),Description = 'Format of Resolution file')
		self.declareProperty(Name='ResNumber',DefaultValue='',Validator=MandatoryValidator(),Description = 'Resolution run number')
		self.declareProperty(Name='BackgroundOption',DefaultValue='Sloping',Validator=ListValidator(['Sloping','Flat','Zero']),Description = 'Form of background to fit')
		self.declareProperty(Name='ElasticOption',DefaultValue=True,Description = 'Include elastic peak in fit')
		self.declareProperty(Name='FixWidth',DefaultValue=False,Description = 'Fix one of the widths')
		self.declareProperty(Name='WidthFile', DefaultValue='',Description = 'Name of file containing fixed width values')
		self.declareProperty(Name='ResNorm',DefaultValue=False,Description = 'Use ResNorm output file')
		self.declareProperty(Name='ResNormNumber', DefaultValue='',Description = 'Name of file containing fixed width values')
		self.declareProperty(Name='EnergyMin', DefaultValue=-0.5,Description = 'Minimum energy for fit. Default=-0.5')
		self.declareProperty(Name='EnergyMax', DefaultValue=0.5,Description = 'Maximum energy for fit. Default=0.5')
		self.declareProperty(Name='SamBinning', DefaultValue=1,Description = 'Binning value (integer) for sample. Default=1')
		self.declareProperty(Name='ResBinning', DefaultValue=1,Description = 'Binning value (integer) for resolution - QLd only. Default=1')
		self.declareProperty(Name='Plot',DefaultValue='None',Validator=ListValidator(['None','ProbBeta','Intensity','FwHm','Fit','All']),Description = 'Plot options')
		self.declareProperty(Name='Verbose',DefaultValue=True,Description = 'Switch Verbose Off/On')
		self.declareProperty(Name='Save',DefaultValue=False,Description = 'Switch Save result to nxs file Off/On')
 
	def PyExec(self):
		run_f2py_compatibility_test()
		
		self.log().information('ILLines input')
		inType = self.getPropertyValue('InputType')
		instr = self.getPropertyValue('Instrument')
		prog = self.getPropertyValue('Program')
		sam = self.getPropertyValue('SamNumber')
		rinType = self.getPropertyValue('ResInputType')
		rtype = self.getPropertyValue('ResType')
		res = self.getPropertyValue('ResNumber')
		elastic = self.getProperty('ElasticOption')
		bgd = self.getPropertyValue('BackgroundOption')
		width = self.getProperty('FixWidth')
		wfile = self.getPropertyValue('WidthFile')
		resnorm = self.getProperty('ResNorm')
		resn = self.getPropertyValue('ResNormNumber')
		emin = self.getPropertyValue('EnergyMin')
		emax = self.getPropertyValue('EnergyMax')
		nbin = self.getPropertyValue('SamBinning')
		nrbin = self.getPropertyValue('ResBinning')
		nbins = [nbin, nrbin]

		sname = instr+'_'+sam
		rname = instr+'_'+res
		rsname = instr+resn
		erange = [emin, emax]
		if elastic:
			o_el = 1
		else:
			o_el = 0
		if bgd == 'Sloping':
			o_bgd = 2
		if bgd == 'Flat':
			o_bgd = 1
		if bgd == 'Zero':
			o_bgd = 0
		if width:
			o_w1 = 1
		else:
			o_w1 = 0
		if resnorm:
			o_res = 1
		else:
			o_res = 0
		fitOp = [o_el, o_bgd, o_w1, o_res]
		verbOp = self.getProperty('Verbose')
		plotOp = self.getPropertyValue('Plot')
		saveOp = self.getProperty('Save')
		Main.QLStart(prog,inType,sname,rinType,rname,rtype,rsname,erange,nbins,fitOp,wfile,verbOp,plotOp,saveOp)
#def QLStart(program,ana,samWS,resWS,rtype,rsname,erange,nbins,fitOp,wfile,Verbose,Plot,Save):

mantid.registerPyAlgorithm(ILLines())         # Register algorithm with Mantid
