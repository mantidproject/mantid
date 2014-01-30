"""*WIKI* 
Given a set of parameter values {T_i} and corresponding structure factors {S(Q,E,T_i)}, this
algorithm calculates S(Q,E,T) for any parameter value T within the range spanned by the {T_i} set.

Required:

  This algorithm requires python package dsfinterp from the python package index.
  To install: sudo pip install dsfinterp

Details:

  For every "dynamical channel" defined by one particular (Q,E) pair, the sequence of scalars 
  S_i=S(Q,E,T_i) is interpolated with a cubic spline which then can be invoked to obtain
  S(Q,E,T).
  
  Previous to the construction of the cubic spline, a local regression is performed
  in the window {S_{i-D/2},..,S_{i+D/2}}, with i running from D/2 to N-D/2.
  The local regression provides estimation of the structure factor S(Q,E,T_i) which replaces
  the actual S_i value. The local regression is also used to obtain an estimation of the
  error of S(Q,E,T_i). This error is employed if the original sequence of scalars {S_i}
  had no associated errors. This lack of errors arises when the structure factors are derived
  from simulations.
 
*WIKI*"""

from mantid.api import PythonAlgorithm, MatrixWorkspaceProperty, CloneWorkspace
from mantid.simpleapi import CloneWorkspace, mtd
from mantid.kernel import FloatArrayProperty, FloatArrayLengthValidator, StringArrayProperty, arrvalidator, Direction, FloatBoundedValidator, logger


class InterpolateSQE(PythonAlgorithm):

  def category(self):
    return "Arithmetic"

  def name(self):
    return 'InterpolateSQE'

  def PyInit(self):
    arrvalidator = StringArrayMandatoryValidator()
    self.declareProperty(StringArrayProperty('Workspaces', values=[], validator=arrvalidator, direction=Direction.Input), doc='list of input workspaces')
    # check the number of input workspaces is same as number of input parameters
    arrvalidator2 = FloatArrayLengthValidator(len(self.getProperty('Workspaces')))
    self.declareProperty(FloatArrayProperty('ParameterValues', values=[], validator=arrvalidator2, direction=Direction.Input), doc='list of input parameter values')
    # check requested parameter falls within the list of parameters
    parmvalidator=FloatBoundedValidator()
    parmvalidator.setBounds( min(self.getProperty('ParameterValues')), max(self.getProperty('ParameterValues')) )
    self.declareProperty('Parameter', 0.0, validator=parmvalidator, direction=Direction.Input, doc="Parameter to interpolate the structure factor")
    self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '', direction=Direction.InputOutput), doc='Workspace to be overwritten with interpolated structure factor')

  def areWorkspacesCompatible(self, a, b):
    sizeA = a.blocksize() * a.getNumberHistograms() 
    sizeB = b.blocksize() * b.getNumberHistograms()
    return sizeA == sizeB

  def PyExec(self):
    # Check congruence of workspaces
    workspaces = self.getProperty('Workspaces')
    fvalues = self.getProperty('ParameterValues')
    for workspace in workspaces[1:]+[]:
      if not self.areWorkspacesCompatible(mtd[workspaces[0]],mtd[workspace]):
        logger.error('Workspace {0} incompatible with {1}'.format(workspace, workspaces[0]))
        return
    # Load the workspaces into a group of dynamic structure factors
    from dsfinterp import Dsf, DsfGroup, ChannelGroup
    dsfgroup = DsfGroup()
    for idsf in range(len(workspaces)):
      dsf = Dsf()
      dsf.Load( mtd[workspaces[idsf]] )
      dsf.setFvalue( fvalues[idsf] )
      dsfgroup.InsertDsf(dsf)
    # Create the intepolator
    channelgroup = ChannelGroup()
    channelgroup.InitFromDsfGroup(dsfgroup)
    channelgroup.InitializeInterpolator()
    # Invoke the interpolator and save to outputworkspace
    dsf = channelgroup( self.getProperty('Parameter') )
    outws = CloneWorkspace( mtd[workspaces[0]] )
    dsf.Save(outws) # overwrite dataY and dataE
    self.setProperty("OutputWorkspace", outws)
#############################################################################################

try:
  import dsfinterp
  AlgorithmFactory.subscribe(Mean())
except:
  pass
