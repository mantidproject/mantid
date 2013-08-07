# Algorithm to start Bayes programs
from mantid.api import PythonAlgorithm, AlgorithmFactory
from mantid.kernel import StringListValidator, StringMandatoryValidator, logger
from IndirectImport import run_f2py_compatibility_test, is_supported_f2py_platform

if is_supported_f2py_platform():
	import IndirectBayes as Main

class ILLines(PythonAlgorithm):
 
	def category(self):
		return "Workflow\\MIDAS;PythonAlgorithms"

	def PyInit(self):
		self.declareProperty(name='InputType',defaultValue='File',validator=StringListValidator(['File','Workspace']), doc='Origin of data input - File (*.nxs) or Workspace')
		self.declareProperty(name='Instrument',defaultValue='IN10',validator=StringListValidator(['IN10','IN16']), doc='Instrument')
		self.declareProperty(name='Program',defaultValue='QL',validator=StringListValidator(['QL','QSe']), doc='Name of program to run')
		self.declareProperty(name='SamNumber',defaultValue='',validator=StringMandatoryValidator(), doc='Sample run number')
		self.declareProperty(name='ResInputType',defaultValue='File',validator=StringListValidator(['File','Workspace']), doc='Origin of res input - File (*_res.nxs) or Workspace')
		self.declareProperty(name='ResType',defaultValue='Res',validator=StringListValidator(['Res','Data']), doc='Format of Resolution file')
		self.declareProperty(name='ResNumber',defaultValue='',validator=StringMandatoryValidator(), doc='Resolution run number')
		self.declareProperty(name='BackgroundOption',defaultValue='Sloping',validator=StringListValidator(['Sloping','Flat','Zero']), doc='Form of background to fit')
		self.declareProperty(name='ElasticOption',defaultValue=True, doc='Include elastic peak in fit')
		self.declareProperty(name='FixWidth',defaultValue=False, doc='Fix one of the widths')
		self.declareProperty(name='WidthFile', defaultValue='', doc='Name of file containing fixed width values')
		self.declareProperty(name='ResNorm',defaultValue=False, doc='Use ResNorm output file')
		self.declareProperty(name='ResNormNumber', defaultValue='', doc='Name of file containing fixed width values')
		self.declareProperty(name='EnergyMin', defaultValue=-0.5, doc='Minimum energy for fit. Default=-0.5')
		self.declareProperty(name='EnergyMax', defaultValue=0.5, doc='Maximum energy for fit. Default=0.5')
		self.declareProperty(name='SamBinning', defaultValue=1, doc='Binning value (integer) for sample. Default=1')
		self.declareProperty(name='ResBinning', defaultValue=1, doc='Binning value (integer) for resolution - QLd only. Default=1')
		self.declareProperty(name='Plot',defaultValue='None',validator=StringListValidator(['None','ProbBeta','Intensity','FwHm','Fit','All']), doc='Plot options')
		self.declareProperty(name='Verbose',defaultValue=True, doc='Switch Verbose Off/On')
		self.declareProperty(name='Save',defaultValue=False, doc='Switch Save result to nxs file Off/On')
 
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

AlgorithmFactory.subscribe(ILLines)         # Register algorithm with Mantid
