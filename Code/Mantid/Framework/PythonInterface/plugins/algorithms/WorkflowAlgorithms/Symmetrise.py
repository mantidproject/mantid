"""*WIKI* 

Symmetrise takes an asymmetric <math>S(Q,w)</math> - i.e. one in which the moduli of xmin & xmax are different. Typically xmax is > mod(xmin). 
A negative value of x is chosen (XCut) so that the curve for mod(XCut) to xmax is reflected and inserted for x less than the XCut.

*WIKI*"""

from mantid import logger, mtd
from mantid.api import PythonAlgorithm, AlgorithmFactory, WorkspaceProperty
from mantid.kernel import Direction
from mantid.simpleapi import *

import math
import os.path
import numpy as np


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

    negative_diff = np.absolute(sample_x - self._x_cut)
    ineg = np.where(negative_diff < delta_x)[0][-1]
    self._check_bounds(ineg, npt, label='Negative')

    positive_diff = np.absolute(sample_x + sample_x[ineg])
    ipos = np.where(positive_diff < delta_x)[0][-1]
    self._check_bounds(ipos, npt, label='Positive')

    ncut = npt-ipos+1
    
    if self._verbose:
      logger.notice('No. points = %d' % npt)
      logger.notice('Negative : at i =%d; x = %f' % (ineg, sample_x[ineg]))
      logger.notice('Positive : at i =%d; x = %f' % (ipos, sample_x[ipos]))
      logger.notice('Copy points = %d' % ncut)
    
    CloneWorkspace(InputWorkspace=self._sample, OutputWorkspace=self._output_workspace)
    
    for m in xrange(num_spectra):
      x = mtd[self._sample].readX(m)
      y = mtd[self._output_workspace].readY(m)
      e = mtd[self._output_workspace].readE(m)
      
      x_out = np.zeros(x.size)
      y_out = np.zeros(y.size)
      e_out = np.zeros(e.size)

      x_out[:ncut] = -x[npt:npt-ncut:-1]
      y_out[:ncut] = y[npt:npt-ncut-1:-1]
      e_out[:ncut] = e[npt:npt-ncut-1:-1]

      x_out[ncut:] = x[ncut:]
      y_out[ncut:] = y[ncut:]
      e_out[ncut:] = e[ncut:]

      mtd[self._output_workspace].setX(m, np.asarray(x_out))
      mtd[self._output_workspace].setY(m, np.asarray(y_out))
      mtd[self._output_workspace].setE(m, np.asarray(e_out))
    
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

  def _check_bounds(self, index, num_pts, label=''):
    """
    Check if the index falls within the bounds of the x range.
    Throws a ValueError if the x point falls outside of the range. 

    @param index  - value of the index within the x range.
    @param num_pts - total number of points in the range.
    @param label - label to call the point if an error is thrown. 
    """
    if index <= 0:
      raise ValueError('%s point %d < 0' % (label, index))
    elif index >= num_pts:
      raise ValueError('%s point %d > %d' % (label, index, num_pts))

  def _plotSymmetrise(self):
    """
    Plot the first spectrum of the input and output workspace together 
    """
    from IndirectImport import import_mantidplot
    mp = import_mantidplot()
    mp.plotSpectrum([self._output_workspace, self._sample],0)

# Register algorithm with Mantid
AlgorithmFactory.subscribe(Symmetrise)
