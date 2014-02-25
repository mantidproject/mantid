'''*WIKI* 


*WIKI*
    
@author Jose Borreguero, NScD
@date October 06, 2013

Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
'''

from mantid.api import IFunction1D, FunctionFactory #, BoundaryConstraint
from mantid import logger
import numpy as np
import copy

from pdb import set_trace as tr

class DSFinterp1DFit(IFunction1D):

  def __init__(self):
    '''declare some constants'''
    super(DSFinterp1DFit, self).__init__()
    self._regressionTypes = set(['linear','quadratic']) #valid syntaxfor python >= 2.6
    self._minWindow = { 'linear':3, 'quadratic':4 }

  def category(self):
    return 'QENS'

  def init(self):
    '''Declare parameters and attributes that participate in the fitting'''
    # Active fitting parameters
    self.declareParameter('Intensity', 1.0, 'Intensity')
    self.declareParameter('TargetParameter', 1.0, 'Target value of the structure factor parameter')

    self.declareAttribute('Workspaces','')
    self.declareAttribute('LoadErrors', False)
    self.declareAttribute('WorkspaceIndex', 0)
    self.declareAttribute('ParameterValues')
    self.declareAttribute('LocalRegression', True)
    self.declareAttribute('RegressionType', 'quadratic')
    self.declareAttribute('RegressionWindow', 6)

    self._channelgroup = None


  def setAttributeValue(self, name, value):
    if name == "Workspaces":
      self._Workspaces = value.split()
      if ',' in value:
        self._Workspaces = [x.strip() for x in value.split(',')]
    elif name == 'LoadErrors':
      self._LoadErrors= bool(value)
    elif name == 'WorkspaceIndex':
      self._WorkspaceIndex = int(value)
    elif name == 'ParameterValues':
      self._ParameterValues = [ float(f) for f in value.split() ]
      if len(self._ParameterValues) != len(self._Workspaces):
        message = 'Number of Workspaces and ParameterValues should be the same. Found {0} and {1}, respectively'.format(len(self._ParameterValues), len(self._Workspaces))
        logger.error(message)
        raise ValueError
      self._fmin = min(self._ParameterValues)
      self._fmax = max(self._ParameterValues)
    elif name == 'LocalRegression':
      self._localRegression = bool(value)
    elif self._localRegression and name == 'RegressionType':
      self._regressionType = value.lower()
      if self._regressionType not in self._regressionTypes:
        message = 'Regression type {0} not implemented. choose one of {1}'.format(value, ', '.join(self._regressionTypes))
        logger.error(message)
        raise NotImplementedError
    elif self._localRegression and name == 'RegressionWindow':
      if value < self._minWindow[self._regressionType]:
        message = 'RegressionWindow must be equal or bigger than {0} for regression type {1}'.format(self._minWindow[self._regressionType], self._regressionType)
        logger.error(message)
        raise ValueError


  def validateParams(self):
    '''Check parameters are positive'''
    height = self.getParameterValue('height')
    if height <=0:
      message = 'Parameter height in DSFinterp1DFit must be positive. Got {0} instead'.format(height)
      logger.error(message)
      return None
    f = self.getParameterValue('f')
    if f < self_fmin or f > self._fmax:
      message = 'TargetParameter is out of bounds [{0}, {1}]'.format(self._fmin, self._fmax)
      logger.error(message)
      return None
    return {'height':h, 'TargetParameter':f}


  def function1D(self, xvals):
    ''' Does something :)
    '''
    p=self.validateParams()
    if not p:
      return np.zeros(len(xvals), dtype=float) # return zeros if parameters not valid
      '''
        gd=mantid.api.AlgorithmManager.createUnmanaged('GroupDetectors')        
        gd.setChild(True)
        gd.initialize()
        gd.setAlwaysStoreInADS(True)
        gd.setLogging(False)
        gd.setProperty("InputWorkspace",__w.getName())
        gd.setProperty("OutputWorkspace",'__sum')
        gd.setProperty("WorkspaceIndexList",inds[0])
        gd.setProperty("PreserveEvents",'1')
        gd.execute()
      '''
    # The first time the function is called requires initialization of the channel group
    if self._channelgroup == None:
      # Create the interpolator
      nf = len(self._ParameterValues)
      for i in range(nf):
        
    else:
      dsf = self._channelgroup(p['TargetParameter'])
      return dsf.intensities  # can we pass by reference?

# Required to have Mantid recognise the new function
try:
  import dsfinterp
  FunctionFactory.subscribe(DSFinterp1DFit)
except:
  logger.debug('Failed to subscribe fit function DSFinterp1DFit; Python package dsfinterp may be missing (https://pypi.python.org/pypi/dsfinterp)')
  pass
