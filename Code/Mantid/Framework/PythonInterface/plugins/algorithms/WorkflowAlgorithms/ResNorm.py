from mantid.api import PythonAlgorithm, AlgorithmFactory
from mantid.kernel import StringListValidator, StringMandatoryValidator
from mantid.simpleapi import *
from mantid import config, logger, mtd
import os

class ResNorm(PythonAlgorithm):

    def category(self):
        return "Workflow\\MIDAS;PythonAlgorithms"

    def summary(self):
        return "This algorithm creates a group 'normalisation' file by taking a resolution file and fitting it to all the groups in the resolution (vanadium) data file which has the same grouping as the sample data of interest."

    def PyInit(self):
        self.declareProperty(name='InputType',defaultValue='File',validator=StringListValidator(['File','Workspace']), doc='Origin of data input - File (.nxs) or Workspace')
        self.declareProperty(name='Instrument',defaultValue='iris',validator=StringListValidator(['irs','iris','osi','osiris']), doc='Instrument')
        self.declareProperty(name='Analyser',defaultValue='graphite002',validator=StringListValidator(['graphite002','graphite004']), doc='Analyser & reflection')
        self.declareProperty(name='VanNumber',defaultValue='',validator=StringMandatoryValidator(), doc='Sample run number')
        self.declareProperty(name='ResInputType',defaultValue='File',validator=StringListValidator(['File','Workspace']), doc='Origin of res input - File (_res.nxs) or Workspace')
        self.declareProperty(name='ResNumber',defaultValue='',validator=StringMandatoryValidator(), doc='Resolution run number')
        self.declareProperty(name='EnergyMin', defaultValue=-0.2, doc='Minimum energy for fit. Default=-0.2')
        self.declareProperty(name='EnergyMax', defaultValue=0.2, doc='Maximum energy for fit. Default=0.2')
        self.declareProperty(name='VanBinning', defaultValue=1, doc='Binning value (integer) for sample. Default=1')
        self.declareProperty(name='Plot',defaultValue='None',validator=StringListValidator(['None','Intensity','Stretch','Fit','All']), doc='Plot options')
        self.declareProperty(name='Verbose',defaultValue=True, doc='Switch Verbose Off/On')
        self.declareProperty(name='Save',defaultValue=False, doc='Switch Save result to nxs file Off/On')

    def PyExec(self):
        from IndirectImport import run_f2py_compatibility_test, is_supported_f2py_platform

        if is_supported_f2py_platform():
            import IndirectBayes as Main

        run_f2py_compatibility_test()

        self.log().information('ResNorm input')
        inType = self.getPropertyValue('InputType')
        prefix = self.getPropertyValue('Instrument')
        ana = self.getPropertyValue('Analyser')
        van = self.getPropertyValue('VanNumber')
        rinType = self.getPropertyValue('ResInputType')
        res = self.getPropertyValue('ResNumber')
        emin = self.getPropertyValue('EnergyMin')
        emax = self.getPropertyValue('EnergyMax')
        nbin = self.getPropertyValue('VanBinning')

        vname = prefix+van+'_'+ana+ '_red'
        rname = prefix+res+'_'+ana+ '_res'
        erange = [float(emin), float(emax)]
        verbOp = self.getProperty('Verbose').value
        plotOp = self.getPropertyValue('Plot')
        saveOp = self.getProperty('Save').value

        workdir = config['defaultsave.directory']
        if inType == 'File':
            vpath = os.path.join(workdir, vname+'.nxs')                          # path name for van nxs file
            LoadNexusProcessed(Filename=vpath, OutputWorkspace=vname)
            Vmessage = 'Vanadium from File : '+vpath
        else:
            Vmessage = 'Vanadium from Workspace : '+vname
        if rinType == 'File':
            rpath = os.path.join(workdir, rname+'.nxs')                            # path name for res nxs file
            LoadNexusProcessed(Filename=rpath, OutputWorkspace=rname)
            Rmessage = 'Resolution from File : '+rpath
        else:
            Rmessage = 'Resolution from Workspace : '+rname
        if verbOp:
            logger.notice(Vmessage)
            logger.notice(Rmessage)
        Main.ResNormRun(vname,rname,erange,nbin,verbOp,plotOp,saveOp)

AlgorithmFactory.subscribe(ResNorm)         # Register algorithm with Mantid
