"""*WIKI* 

Symmetrise takes an asymmetric <math>S(Q,w)</math> - i.e. one in which the moduli of xmin & xmax are different. Typically xmax is > mod(xmin). 
A negative value of x is chosen (Xcut) so that the curve for mod(Xcut) to xmax is reflected and inserted for x less than the Xcut.

*WIKI*"""

from mantid import config, logger, mtd
from mantid.api import PythonAlgorithm, AlgorithmFactory
from mantid.kernel import StringListValidator, StringMandatoryValidator
from mantid.simpleapi import *
import sys, platform, math, os.path, numpy as np

class Symmetrise(PythonAlgorithm):

  def category(self):
    return "Workflow\\MIDAS;PythonAlgorithms"

  def PyInit(self):
    self.setOptionalMessage("Takes and asymmetric S(Q,w) and makes it symmetric")
    self.setWikiSummary("Takes and asymmetric S(Q,w) and makes it symmetric")

    self.declareProperty(name='InputType',defaultValue='File',validator=StringListValidator(['File','Workspace']), doc='Origin of data input - File (_red.nxs) or Workspace')
    self.declareProperty(name='Instrument',defaultValue='iris',validator=StringListValidator(['irs','iris','osi','osiris']), doc='Instrument')
    self.declareProperty(name='Analyser',defaultValue='graphite002',validator=StringListValidator(['graphite002','graphite004']), doc='Analyser & reflection')
    self.declareProperty(name='SamNumber',defaultValue='',validator=StringMandatoryValidator(), doc='Sample run number')
    self.declareProperty(name='Xcut',defaultValue='',validator=StringMandatoryValidator(), doc='X cutoff value')
    self.declareProperty('Verbose',defaultValue=True, doc='Switch Verbose Off/On')
    self.declareProperty('Plot',defaultValue=True, doc='Switch Plot Off/On')
    self.declareProperty('Save',defaultValue=False, doc='Switch Save result to nxs file Off/On')
 
  def PyExec(self):

    self.log().information('Symmetrise')
    inType = self.getPropertyValue('InputType')
    prefix = self.getPropertyValue('Instrument')
    ana = self.getPropertyValue('Analyser')
    sn = self.getPropertyValue('SamNumber')
    sam = prefix+sn+'_'+ana+'_red'
    cut = self.getPropertyValue('Xcut')
    cut = float(cut)

    verbOp = self.getProperty('Verbose').value
    plotOp = self.getProperty('Plot').value
    saveOp = self.getProperty('Save').value
    
    self.SymmStart(inType,sam,cut,verbOp,plotOp,saveOp)


  def SymmRun(self, sample,cut,Verbose,Plot,Save):
    workdir = config['defaultsave.directory']
    symWS = sample[:-3] + 'sym'
    hist,npt = CheckHistZero(sample)
    Xin = mtd[sample].readX(0)
    delx = Xin[1]-Xin[0]
    if math.fabs(cut) < 1e-5:
      error = 'Cut point is Zero'     
      logger.notice('ERROR *** ' + error)
      sys.exit(error)
    for n in range(0,npt):
      x = Xin[n]-cut
      if math.fabs(x) < delx:
        ineg = n
    if ineg <= 0:
      error = 'Negative point('+str(ineg)+') < 0'     
      logger.notice('ERROR *** ' + error)
      sys.exit(error)
    if ineg >= npt:
      error = type + 'Negative point('+str(ineg)+') > '+str(npt)      
      logger.notice('ERROR *** ' + error)
      sys.exit(error)
    for n in range(0,npt):
      x = Xin[n]+Xin[ineg]
      if math.fabs(x) < delx:
        ipos = n
    if ipos <= 0:
      error = 'Positive point('+str(ipos)+') < 0'     
      logger.notice('ERROR *** ' + error)
      sys.exit(error)
    if ipos >= npt:
      error = type + 'Positive point('+str(ipos)+') > '+str(npt)      
      logger.notice('ERROR *** ' + error)
      sys.exit(error)
    ncut = npt-ipos+1
    if Verbose:
      logger.notice('No. points = '+str(npt))
      logger.notice('Negative : at i ='+str(ineg)+' ; x = '+str(Xin[ineg]))
      logger.notice('Positive : at i ='+str(ipos)+' ; x = '+str(Xin[ipos]))
      logger.notice('Copy points = '+str(ncut))
    for m in range(0,hist):
      Xin = mtd[sample].readX(m)
      Yin = mtd[sample].readY(m)
      Ein = mtd[sample].readE(m)
      Xout = []
      Yout = []
      Eout = []
      for n in range(0,ncut):
        icut = npt-n-1
        Xout.append(-Xin[icut])
        Yout.append(Yin[icut])
        Eout.append(Ein[icut])
      for n in range(ncut,npt):
        Xout.append(Xin[n])
        Yout.append(Yin[n])
        Eout.append(Ein[n])
      if m == 0:
        CreateWorkspace(OutputWorkspace=symWS, DataX=Xout, DataY=Yout, DataE=Eout,
          Nspec=1, UnitX='DeltaE')
      else:
        CreateWorkspace(OutputWorkspace='__tmp', DataX=Xout, DataY=Yout, DataE=Eout,
          Nspec=1, UnitX='DeltaE')
        ConjoinWorkspaces(InputWorkspace1=symWS, InputWorkspace2='__tmp',CheckOverlapping=False)
    if Save:
      path = os.path.join(workdir,symWS+'.nxs')
      SaveNexusProcessed(InputWorkspace=symWS, Filename=path)
      if Verbose:
        logger.notice('Output file : ' + path)
    if Plot:
      self.plotSymm(symWS,sample)

  def SymmStart(self, inType,sname,cut,Verbose,Plot,Save):
    from IndirectCommon import CheckHistZero, StartTime, EndTime, getDefaultWorkingDirectory
    from IndirectImport import import_mantidplot
    
    StartTime('Symmetrise')
    workdir = config['defaultsave.directory']
    if inType == 'File':
      spath = os.path.join(workdir, sname+'.nxs')   # path name for sample nxs file
      LoadNexusProcessed(FileName=spath, OutputWorkspace=sname)
      message = 'Input from File : '+spath
    else:
      message = 'Input from Workspace : '+sname
    if Verbose:
      logger.notice(message)
    self.SymmRun(sname,cut,Verbose,Plot,Save)
    EndTime('Symmetrise')

  def plotSymm(self, sym,sample):
    mp = import_mantidplot()
    tot_plot=mp.plotSpectrum([sym,sample],0)

AlgorithmFactory.subscribe(Symmetrise)         # Register algorithm with Mantid
