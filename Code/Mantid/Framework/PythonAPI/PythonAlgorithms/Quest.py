# Algorithm to start Bayes programs
from MantidFramework import *
from mantidsimple import *
from mantidplotpy import *
from IndirectCommon import runF2PyCheck, inF2PyCompatibleEnv
if inF2PyCompatibleEnv():
	import IndirectBayes as Main

class Quest(PythonAlgorithm):
 
	def PyInit(self):
		self.declareProperty(Name='Instrument',DefaultValue='IRIS',Validator=ListValidator(['IRIS','OSIRIS']))
		self.declareProperty(Name='Analyser',DefaultValue='graphite002',Validator=ListValidator(['graphite002','graphite004']))
		self.declareProperty(Name='SamNumber',DefaultValue='',Validator=MandatoryValidator(),Description = 'Sample run number')
		self.declareProperty(Name='ResNumber',DefaultValue='',Validator=MandatoryValidator(),Description = 'Resolution run number')
		self.declareProperty(Name='ElasticOption',DefaultValue='Yes',Validator=ListValidator(['Yes','No']),Description = 'Include elastic peak in fit')
		self.declareProperty(Name='BackgroundOption',DefaultValue='Sloping',Validator=ListValidator(['Sloping','Flat','Zero']),Description = 'Form of background to fit')
		self.declareProperty(Name='EnergyMin', DefaultValue=-0.5)
		self.declareProperty(Name='EnergyMax', DefaultValue=0.5)
		self.declareProperty(Name='SamBinning', DefaultValue=1,Description = 'Binning value for sample')
		self.declareProperty(Name='NumberSigma', DefaultValue=50,Description = 'Number of sigma values - Quest only')
		self.declareProperty(Name='NumberBeta', DefaultValue=30,Description = 'Number of beta values - Quest only')
		self.declareProperty(Name='Verbose',DefaultValue='Yes',Validator=ListValidator(['Yes','No']))
		self.declareProperty(Name='Plot',DefaultValue='None',Validator=ListValidator(['None','Sigma','Beta','All']))
 
	def PyExec(self):
		runF2PyCheck()
		
		self.log().information('Quest input')
		instr = self.getPropertyValue('Instrument')
		if instr == 'IRIS':
			prefix = 'irs'
		if instr == 'OSIRIS':
			prefix = 'osi'
		ana = self.getPropertyValue('Analyser')
		sam = self.getPropertyValue('SamNumber')
		res = self.getPropertyValue('ResNumber')
		elastic = self.getPropertyValue('ElasticOption')
		bgd = self.getPropertyValue('BackgroundOption')
		emin = self.getPropertyValue('EnergyMin')
		emax = self.getPropertyValue('EnergyMax')
		nbin = self.getPropertyValue('SamBinning')
		nbins = [nbin, 1]
		nbet = self.getPropertyValue('NumberBeta')
		nsig = self.getPropertyValue('NumberSigma')
		nbs = [nbet, nsig]
		nbs = [ 30,50]

		sname = prefix+sam
		rname = prefix+res
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
		fitOp = [o_el, o_bgd, 0, 0]
		verbOp = self.getPropertyValue('Verbose')
		plotOp = self.getPropertyValue('Plot')
		Main.QuestStart(ana,sname,rname,nbs,erange,nbins,fitOp,verbOp,plotOp)

mantid.registerPyAlgorithm(Quest())         # Register algorithm with Mantid
