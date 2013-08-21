# Algorithm to start Bayes programs
from mantid.simpleapi import *
from mantid.kernel import StringListValidator, StringMandatoryValidator
from mantid.api import PythonAlgorithm, AlgorithmFactory
from mantid import config

class QLines(PythonAlgorithm):
 
	def category(self):
		return "Workflow\\MIDAS;PythonAlgorithms"

	def PyInit(self):
		self.declareProperty(name='InputType',defaultValue='File',validator=StringListValidator(['File','Workspace']), doc='Origin of data input - File (*.nxs) or Workspace')
		self.declareProperty(name='Instrument',defaultValue='iris',validator=StringListValidator(['irs','iris','osi','osiris']), doc='Instrument')
		self.declareProperty(name='Analyser',defaultValue='graphite002',validator=StringListValidator(['graphite002','graphite004']), doc='Analyser & reflection')
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
		self.declareProperty(name='Sequence',defaultValue=True, doc='Switch Sequence Off/On')
		self.declareProperty(name='Plot',defaultValue='None',validator=StringListValidator(['None','ProbBeta','Intensity','FwHm','Fit','All']), doc='Plot options')
		self.declareProperty(name='Verbose',defaultValue=True, doc='Switch Verbose Off/On')
		self.declareProperty(name='Save',defaultValue=False, doc='Switch Save result to nxs file Off/On')
 
	def PyExec(self):
                from IndirectImport import run_f2py_compatibility_test, is_supported_f2py_platform

                if is_supported_f2py_platform():
                        import IndirectBayes as Main

		run_f2py_compatibility_test()
		
		self.log().information('QLines input')
		inType = self.getPropertyValue('InputType')
		prefix = self.getPropertyValue('Instrument')
		ana = self.getPropertyValue('Analyser')
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

		if rtype == 'Res':
			rext = 'res'
		if rtype == 'Data':
			rext = 'red'
		sname = prefix+sam+'_'+ana + '_red'
		rname = prefix+res+'_'+ana + '_' + rext
		rsname = prefix+resn+'_'+ana + '_ResNorm_Paras'
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
		if width:
			o_w1 = 1
		else:
			o_w1 = 0
		if resnorm:
			o_res = 1
		else:
			o_res = 0
		fitOp = [o_el, o_bgd, o_w1, o_res]
		loopOp = self.getProperty('Sequence')
		verbOp = self.getProperty('Verbose')
		plotOp = self.getPropertyValue('Plot')
		saveOp = self.getProperty('Save')

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
		if verbOp:
			logger.notice(Smessage)
			logger.notice(Rmessage)
		if fitOp[3] == 1:
			path = os.path.join(workdir, rsname+'.nxs')	# path name for resnnrm nxs file
			LoadNexusProcessed(Filename=path, OutputWorkspace='ResNorm')
		Main.QLRun(prog,sname,rname,rsname,erange,nbins,fitOp,wfile,loopOp,verbOp,plotOp,saveOp)

AlgorithmFactory.subscribe(QLines)         # Register algorithm with Mantid
