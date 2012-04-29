# Algorithm to start Bayes programs
from MantidFramework import *
from mantidsimple import *
from mantidplotpy import *
from IndirectCommon import runF2PyCheck, inF2PyCompatibleEnv
if inF2PyCompatibleEnv():
	import IndirectBayes as Main

class QLines(PythonAlgorithm):
 
	def PyInit(self):
		self.declareProperty(Name='Instrument',DefaultValue='IRIS',Validator=ListValidator(['IRIS','OSIRIS']))
		self.declareProperty(Name='Analyser',DefaultValue='graphite002',Validator=ListValidator(['graphite002','graphite004']))
		self.declareProperty(Name='Program',DefaultValue='QL',Validator=ListValidator(['QL','QSe']),Description = 'Name of program to run')
		self.declareProperty(Name='ResType',DefaultValue='Res',Validator=ListValidator(['Res','Data']),Description = 'Format of Resolution file')
		self.declareProperty(Name='SamNumber',DefaultValue='',Validator=MandatoryValidator(),Description = 'Sample run number')
		self.declareProperty(Name='ResNumber',DefaultValue='',Validator=MandatoryValidator(),Description = 'Resolution run number')
		self.declareProperty(Name='ElasticOption',DefaultValue='Yes',Validator=ListValidator(['Yes','No']),Description = 'Include elastic peak in fit')
		self.declareProperty(Name='BackgroundOption',DefaultValue='Sloping',Validator=ListValidator(['Sloping','Flat','Zero']),Description = 'Form of background to fit')
		self.declareProperty(Name='FixWidth',DefaultValue='No',Validator=ListValidator(['No','Yes']),Description = 'Fix one of the widths')
		self.declareProperty(Name='WidthFile', DefaultValue='',Description = 'Name of file containing fixed width values')
		self.declareProperty(Name='ResNorm',DefaultValue='No',Validator=ListValidator(['No','Yes']),Description = 'Use ResNorm output file')
		self.declareProperty(Name='ResNormNumber', DefaultValue='',Description = 'Name of file containing fixed width values')
		self.declareProperty(Name='EnergyMin', DefaultValue=-0.5)
		self.declareProperty(Name='EnergyMax', DefaultValue=0.5)
		self.declareProperty(Name='SamBinning', DefaultValue=1,Description = 'Binning value for sample')
		self.declareProperty(Name='ResBinning', DefaultValue=1,Description = 'Binning value for resolution - QLd only')
		self.declareProperty(Name='Verbose',DefaultValue='Yes',Validator=ListValidator(['No','Yes']))
		self.declareProperty(Name='Plot',DefaultValue='None',Validator=ListValidator(['None','ProbBeta','Intensity','FwHm','Fit','All']))
 
	def PyExec(self):
		runF2PyCheck()
		
		self.log().information('QLines input')
		instr = self.getPropertyValue('Instrument')
		if instr == 'IRIS':
			prefix = 'irs'
		if instr == 'OSIRIS':
			prefix = 'osi'
		ana = self.getPropertyValue('Analyser')
		prog = self.getPropertyValue('Program')
		rtype = self.getPropertyValue('ResType')
		sam = self.getPropertyValue('SamNumber')
		res = self.getPropertyValue('ResNumber')
		elastic = self.getPropertyValue('ElasticOption')
		bgd = self.getPropertyValue('BackgroundOption')
		width = self.getPropertyValue('FixWidth')
		wfile = self.getPropertyValue('WidthFile')
		resnorm = self.getPropertyValue('ResNorm')
		resn = self.getPropertyValue('ResNormNumber')
		emin = self.getPropertyValue('EnergyMin')
		emax = self.getPropertyValue('EnergyMax')
		nbin = self.getPropertyValue('SamBinning')
		nrbin = self.getPropertyValue('ResBinning')
		nbins = [nbin, nrbin]

		sname = prefix+sam
		rname = prefix+res
		rsname = prefix +resn
		erange = [emin, emax]
		if elastic == 'Yes':
			o_el = 1
		if elastic == 'No':
			o_el = 0
		if bgd == 'Sloping':
			o_bgd = 2
		if bgd == 'Flat':
			o_bgd = 1
		if bgd == 'Zero':
			o_bgd = 0
		if width == 'Yes':
			o_w1 = 1
		if width == 'No':
			o_w1 = 0
		if resnorm == 'Yes':
			o_res = 1
		if resnorm == 'No':
			o_res = 0
		fitOp = [o_el, o_bgd, o_w1, o_res]
		verbOp = self.getPropertyValue('Verbose')
		plotOp = self.getPropertyValue('Plot')
		Main.QLStart(prog,ana,sname,rname,rtype,rsname,erange,nbins,fitOp,wfile,verbOp,plotOp)

mantid.registerPyAlgorithm(QLines())         # Register algorithm with Mantid
