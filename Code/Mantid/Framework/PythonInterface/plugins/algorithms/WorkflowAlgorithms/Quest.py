from mantid.api import PythonAlgorithm, AlgorithmFactory
from mantid.kernel import StringListValidator, StringMandatoryValidator
from mantid.simpleapi import *
from mantid import config, logger, mtd
import os

class Quest(PythonAlgorithm):

    def category(self):
    	return "Workflow\\MIDAS;PythonAlgorithms"

    def summary(self):
    	return "This is a variation of the stretched exponential option of Quasi."

    def PyInit(self):
    	self.declareProperty(name='InputType',defaultValue='File',validator=StringListValidator(['File','Workspace']), doc='Origin of data input - File (.nxs) or Workspace')
    	self.declareProperty(name='Instrument',defaultValue='iris',validator=StringListValidator(['irs','iris','osi','osiris']), doc='Instrument')
    	self.declareProperty(name='Analyser',defaultValue='graphite002',validator=StringListValidator(['graphite002','graphite004']), doc='Analyser & reflection')
    	self.declareProperty(name='SamNumber',defaultValue='',validator=StringMandatoryValidator(), doc='Sample run number')
    	self.declareProperty(name='ResInputType',defaultValue='File',validator=StringListValidator(['File','Workspace']), doc='Origin of res input - File (_res.nxs) or Workspace')
    	self.declareProperty(name='ResNumber',defaultValue='',validator=StringMandatoryValidator(), doc='Resolution run number')
    	self.declareProperty(name='ResNormInputType',defaultValue='File',validator=StringListValidator(['File','Workspace']), doc='Origin of ResNorm input - File (_red.nxs) or Workspace')
    	self.declareProperty(name='ResNormNumber',defaultValue='',validator=StringMandatoryValidator(), doc='ResNorm run number')
    	self.declareProperty(name='ElasticOption',defaultValue=True, doc='Include elastic peak in fit')
    	self.declareProperty(name='BackgroundOption',defaultValue='Sloping',validator=StringListValidator(['Sloping','Flat','Zero']), doc='Form of background to fit')
    	self.declareProperty(name='EnergyMin', defaultValue=-0.5, doc='Minimum energy for fit. Default=-0.5')
    	self.declareProperty(name='EnergyMax', defaultValue=0.5, doc='Maximum energy for fit. Default=0.5')
    	self.declareProperty(name='SamBinning', defaultValue=1, doc='Binning value(integer) for sample. Default=1')
    	self.declareProperty(name='NumberSigma', defaultValue=50, doc='Number of sigma values. Default=50')
    	self.declareProperty(name='NumberBeta', defaultValue=30, doc='Number of beta values. Default=30')
    	self.declareProperty(name='Sequence',defaultValue=True, doc='Switch Sequence Off/On')
    	self.declareProperty(name='Plot',defaultValue='None',validator=StringListValidator(['None','Sigma','Beta','All']), doc='Plot options')
    	self.declareProperty(name='Verbose',defaultValue=True, doc='Switch Verbose Off/On')
    	self.declareProperty(name='Save',defaultValue=False, doc='Switch Save result to nxs file Off/On')

    def PyExec(self):
    	from IndirectImport import run_f2py_compatibility_test, is_supported_f2py_platform

    	if is_supported_f2py_platform():
    		import IndirectBayes as Main

    	run_f2py_compatibility_test()

    	self.log().information('Quest input')
    	inType = self.getPropertyValue('InputType')
    	prefix = self.getPropertyValue('Instrument')
    	ana = self.getPropertyValue('Analyser')
    	sam = self.getPropertyValue('SamNumber')
    	rinType = self.getPropertyValue('ResInputType')
    	res = self.getPropertyValue('ResNumber')
    	rsnormType = self.getPropertyValue('ResNormInputType')
    	rsnormNum = self.getPropertyValue('ResNormNumber')
    	elastic = self.getProperty('ElasticOption').value
    	bgd = self.getPropertyValue('BackgroundOption')
    	emin = self.getPropertyValue('EnergyMin')
    	emax = self.getPropertyValue('EnergyMax')
    	nbin = self.getPropertyValue('SamBinning')
    	nbins = [nbin, 1]
    	nbet = self.getPropertyValue('NumberBeta')
    	nsig = self.getPropertyValue('NumberSigma')
    	nbs = [nbet, nsig]

    	sname = prefix+sam+'_'+ana + '_red'
    	rname = prefix+res+'_'+ana + '_res'
    	rsname = prefix+rsnormNum+'_'+ana+ '_ResNorm_Paras'
    	erange = [float(emin), float(emax)]
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
    	fitOp = [o_el, o_bgd, 0, 0]
    	loopOp = self.getProperty('Sequence').value
    	verbOp = self.getProperty('Verbose').value
    	plotOp = self.getPropertyValue('Plot')
    	saveOp = self.getProperty('Save').value

    	workdir = config['defaultsave.directory']
    	if inType == 'File':
    		spath = os.path.join(workdir, sname+'.nxs')		# path name for sample nxs file
    		LoadNexusProcessed(Filename=spath, OutputWorkspace=sname)
    		Smessage = 'Sample from File : '+spath
    	else:
    		Smessage = 'Sample from Workspace : '+sname

    	if rinType == 'File':
    		rpath = os.path.join(workdir, rname+'.nxs')		# path name for res nxs file
    		LoadNexusProcessed(Filename=rpath, OutputWorkspace=rname)
    		Rmessage = 'Resolution from File : '+rpath
    	else:
    		Rmessage = 'Resolution from Workspace : '+rname

    	if rsnormType == 'File':
    		rpath = os.path.join(workdir, rsname+'.nxs')		# path name for res nxs file
    		LoadNexusProcessed(Filename=rpath, OutputWorkspace=rsname)
    		Rmessage = 'ResNorm from File : '+rpath
    	else:
    		Rmessage = 'ResNorm from Workspace : '+rsname

    	if verbOp:
    		logger.notice(Smessage)
    		logger.notice(Rmessage)
    	Main.QuestRun(sname,rname,rsname,nbs,erange,nbins,fitOp,loopOp,verbOp,plotOp,saveOp)

AlgorithmFactory.subscribe(Quest)         # Register algorithm with Mantid
