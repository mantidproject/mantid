'''
@author Jose Borreguero, NScD
@date October 06, 2013

Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

from mantid.api import IFunction1D, FunctionFactory
from mantid.simpleapi import mtd
from mantid import logger
import numpy
from scipy.interpolate import interp1d

class DSFinterp1DFit(IFunction1D):

  def category(self):
    return 'QuasiElastic'

  def init(self):
    '''Declare parameters and attributes that participate in the fitting'''
    # Active fitting parameters
    self.declareParameter('Intensity', 1.0, 'Intensity')
    self.declareParameter('TargetParameter', 1.0, 'Target value of the structure factor parameter')

    self.declareAttribute('InputWorkspaces','')
    self.declareAttribute('LoadErrors', False)
    self.declareAttribute('WorkspaceIndex', 0)
    self.declareAttribute('ParameterValues', '')
    self.declareAttribute('LocalRegression', True)
    self.declareAttribute('RegressionType', 'quadratic')
    self.declareAttribute('RegressionWindow', 6)
    # "private" attributes associated to the declare function attributes
    self._InputWorkspaces = None
    self._LoadErrors = None
    self._WorkspaceIndex = None
    self._ParameterValues = None
    self._fmin = None
    self._fmax = None
    self._LocalRegression = None
    self._RegressionType = None
    self._RegressionTypes = set(['linear','quadratic']) #valid syntaxfor python >= 2.6
    self._RegressionWindow = None
    self._minWindow = { 'linear':3, 'quadratic':4 }
    # channelgroup to interpolate values
    self._channelgroup = None
    self._xvalues = None #energies of the channels

  def setAttributeValue(self, name, value):
    if name == "InputWorkspaces":
      self._InputWorkspaces = value.split()
      if ',' in value:
        self._InputWorkspaces = [x.strip() for x in value.split(',')]
    elif name == 'LoadErrors':
      self._LoadErrors= bool(value)
    elif name == 'WorkspaceIndex':
      self._WorkspaceIndex = int(value)
    elif name == 'ParameterValues':
      self._ParameterValues = [ float(f) for f in value.split() ]
      self._fmin = min(self._ParameterValues)
      self._fmax = max(self._ParameterValues)
    elif name == 'LocalRegression':
      self._LocalRegression = bool(value)
    elif name == 'RegressionType':
      self._RegressionType = value.lower()
    elif name == 'RegressionWindow':
      self._RegressionWindow = value

  def validateParams(self):
    '''Check parameters within expected range'''
    intensity = self.getParameterValue('Intensity')
    if intensity <=0:
      message = 'Parameter Intensity in DSFinterp1DFit must be positive. Got {0} instead'.format(intensity)
      logger.error(message)
      return None
    f = self.getParameterValue('TargetParameter')
    if f < self._fmin or f > self._fmax:
      message = 'TargetParameter {0} is out of bounds [{1}, {2}]. Applying penalty...'.format(f, self._fmin, self._fmax)
      logger.error(message)
      return None
    return {'Intensity':intensity, 'TargetParameter':f}


  def function1D(self, xvals):
    ''' Fit using the interpolated structure factor '''
    p=self.validateParams()
    if not p:
      return numpy.zeros(len(xvals), dtype=float) # return zeros if parameters not valid
    # The first time the function is called requires initialization of the interpolator
    if self._channelgroup == None:
      # Check consistency of the input
      # check InputWorkspaces have at least the workspace index
      for w in self._InputWorkspaces:
        if mtd[w].getNumberHistograms() <= self._WorkspaceIndex:
          message = 'Numer of histograms in Workspace {0} does not allow for workspace index {1}'.format(w,self._WorkspaceIndex)
          logger.error(message)
          raise IndexError(message)
      # check number of input workspaces and parameters is the same
      if len(self._ParameterValues) != len(self._InputWorkspaces):
        message = 'Number of InputWorkspaces and ParameterValues should be the same. Found {0} and {1}, respectively'.format(len(self._InputWorkspaces), len(self._ParameterValues))
        logger.error(message)
        raise ValueError(message)
      # check the regression type is valid
      if self._RegressionType not in self._RegressionTypes:
        message = 'Regression type {0} not implemented. choose one of {1}'.format(self._RegressionType, ', '.join(self._RegressionTypes))
        logger.error(message)
        raise NotImplementedError(message)
      # check the regression window is appropriate for the regression type selected
      if self._RegressionWindow < self._minWindow[self._RegressionType]:
        message = 'RegressionWindow must be equal or bigger than {0} for regression type {1}'.format(self._minWindow[self._RegressionType], self._RegressionType)
        logger.error(message)
        raise ValueError(message)
      # Initialize the energies of the channels with the first of the input workspaces
      self._xvalues = numpy.copy( mtd[ self._InputWorkspaces[0] ].dataX(self._WorkspaceIndex) )
      if len(self._xvalues) == 1+ len( mtd[ self._InputWorkspaces[0] ].dataY(self._WorkspaceIndex) ):
        self._xvalues = (self._xvalues[1:]+self._xvalues[:-1])/2.0  # Deal with histogram data
      # Initialize the channel group
      nf = len(self._ParameterValues)
      # Load the InputWorkspaces into a group of dynamic structure factors
      from dsfinterp.dsf import Dsf
      from dsfinterp.dsfgroup import DsfGroup
      dsfgroup = DsfGroup()
      for idsf in range(nf):
        dsf = Dsf()
        dsf.SetIntensities( mtd[ self._InputWorkspaces[idsf] ].dataY(self._WorkspaceIndex) )
        dsf.errors = None # do not incorporate error data
        if self._LoadErrors:
          dsf.SetErrors(mtd[ self._InputWorkspaces[idsf] ].dataE(self._WorkspaceIndex))
        dsf.SetFvalue( self._ParameterValues[idsf] )
        dsfgroup.InsertDsf(dsf)
      # Create the interpolator
      from dsfinterp.channelgroup import ChannelGroup
      self._channelgroup = ChannelGroup()
      self._channelgroup.InitFromDsfGroup(dsfgroup)
      if self._LocalRegression:
        self._channelgroup.InitializeInterpolator(running_regr_type=self._RegressionType, windowlength=self._RegressionWindow)
      else:
        self._channelgroup.InitializeInterpolator(windowlength=0)
    # channel group has been initialized, so evaluate the interpolator
    dsf = self._channelgroup(p['TargetParameter'])
    # Linear interpolation between the energies of the channels and the xvalues we require
    # NOTE: interpolator evaluates to zero for any of the xvals outside of the domain defined by self._xvalues
    intensities_interpolator = interp1d(self._xvalues, p['Intensity']*dsf.intensities, kind='linear')
    return intensities_interpolator(xvals)  # can we pass by reference?

# Required to have Mantid recognize the new function
try:
  import dsfinterp
  FunctionFactory.subscribe(DSFinterp1DFit)
except:
  logger.debug('Failed to subscribe fit function DSFinterp1DFit; Python package dsfinterp may be missing (https://pypi.python.org/pypi/dsfinterp)')
  pass
