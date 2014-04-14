"""*WIKI* 

Symmetrise takes an asymmetric <math>S(Q,w)</math> - i.e. one in which the moduli of xmin & xmax are different. Typically xmax is > mod(xmin). 
A negative value of x is chosen (XCut) so that the curve for mod(XCut) to xmax is reflected and inserted for x less than the XCut.

*WIKI*"""

from mantid import config, logger, mtd
from mantid.api import PythonAlgorithm, AlgorithmFactory, WorkspaceProperty
from mantid.kernel import StringListValidator, StringMandatoryValidator, Direction
from mantid.simpleapi import *
import sys, platform, math, os.path, numpy as np

class Symmetrise(PythonAlgorithm):

  def category(self):
    return "Workflow\\MIDAS;PythonAlgorithms"

  def PyInit(self):
    self.setOptionalMessage("Takes an asymmetric S(Q,w) and makes it symmetric")
    self.setWikiSummary("Takes an asymmetric S(Q,w) and makes it symmetric")

    self.declareProperty(WorkspaceProperty("Sample", "", Direction.Input), doc='Sample to run with')
    self.declareProperty('XCut', 0.0, doc='X cut off value')

    self.declareProperty('Verbose',defaultValue=True, doc='Switch verbose output Off/On')
    self.declareProperty('Plot',defaultValue=True, doc='Switch plotting Off/On')
    self.declareProperty('Save',defaultValue=False, doc='Switch saving result to nxs file Off/On')
 
    self.declareProperty(WorkspaceProperty("OutputWorkspace", "", Direction.Output), doc='Name to call the output workspace.')
  
  def PyExec(self):
    from IndirectCommon import CheckHistZero, StartTime, EndTime, getDefaultWorkingDirectory

    StartTime('Symmetrise')
    self._setup()
    num_spectra, npt = CheckHistZero(self._sample)
    
    sample_x = mtd[self._sample].readX(0)

    if math.fabs(self._x_cut) < 1e-5:
      raise ValueError('XCut point is Zero')
    
    delta_x = sample_x[1]-sample_x[0]
    # diff = np.absolute(sample_x - self._x_cut)
    # ineg = np.where(diff < delta_x)[0]

    for n in range(npt):
      x = sample_x[n]-self._x_cut
      if math.fabs(x) < delta_x:
        ineg = n
    
    if ineg <= 0:
      error = 'Negative point('+str(ineg)+') < 0'     
      logger.notice('ERROR *** ' + error)
      sys.exit(error)
    
    if ineg >= npt:
      error = type + 'Negative point('+str(ineg)+') > '+str(npt)      
      logger.notice('ERROR *** ' + error)
      sys.exit(error)
    
    for n in range(npt):
      x = sample_x[n]+sample_x[ineg]
      if math.fabs(x) < delta_x:
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
    
    if self._verbose:
      logger.notice('No. points = '+str(npt))
      logger.notice('Negative : at i ='+str(ineg)+' ; x = '+str(sample_x[ineg]))
      logger.notice('Positive : at i ='+str(ipos)+' ; x = '+str(sample_x[ipos]))
      logger.notice('Copy points = '+str(xcut))
    
    for m in range(num_spectra):
      sample_x = mtd[self._sample].readX(m)
      Yin = mtd[self._sample].readY(m)
      Ein = mtd[self._sample].readE(m)
      Xout = []
      Yout = []
      Eout = []
      for n in range(0,ncut):
        icut = npt-n-1
        Xout.append(-sample_x[icut])
        Yout.append(Yin[icut])
        Eout.append(Ein[icut])
      for n in range(ncut,npt):
        Xout.append(sample_x[n])
        Yout.append(Yin[n])
        Eout.append(Ein[n])
      
      if m == 0:
        CreateWorkspace(OutputWorkspace=self._output_workspace, DataX=Xout, DataY=Yout, DataE=Eout,
          Nspec=1, UnitX='DeltaE')
      else:
        CreateWorkspace(OutputWorkspace='__tmp', DataX=Xout, DataY=Yout, DataE=Eout,
          Nspec=1, UnitX='DeltaE')
        ConjoinWorkspaces(InputWorkspace1=self._output_workspace, InputWorkspace2='__tmp',CheckOverlapping=False)
    

    if self._save:
      workdir = getDefaultWorkingDirectory()
      file_path = os.path.join(workdir,self._output_workspace+'.nxs')
      SaveNexusProcessed(InputWorkspace=self._output_workspace, Filename=file_path)
      
      if self._verbose:
        logger.notice('Output file : ' + file_path)
    
    if self._plot:
      self._plotSymmetrise()
    
    self.setProperty("OutputWorkspace", self._output_workspace)
    EndTime('Symmetrise')


  def _setup(self):
    """
    Get the algorithm properties. 
    """
    self._sample = self.getPropertyValue('Sample')
    self._x_cut = self.getProperty('XCut').value

    self._verbose = self.getProperty('Verbose').value
    self._plot = self.getProperty('Plot').value
    self._save = self.getProperty('Save').value
    
    self._output_workspace = self.getPropertyValue('OutputWorkspace') 

  def _plotSymmetrise(self):
    """
    Plot the first spectrum of the input and output workspace together 
    """
    from IndirectImport import import_mantidplot
    mp = import_mantidplot()
    tot_plot = mp.plotSpectrum([self._output_workspace, self._sample],0)

AlgorithmFactory.subscribe(Symmetrise)         # Register algorithm with Mantid
