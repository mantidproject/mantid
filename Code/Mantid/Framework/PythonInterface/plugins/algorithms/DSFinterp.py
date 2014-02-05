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

from mantid.api import PythonAlgorithm, MatrixWorkspaceProperty, AlgorithmFactory
from mantid.simpleapi import CloneWorkspace, mtd
from mantid.kernel import FloatArrayProperty, FloatArrayLengthValidator, StringArrayProperty, StringArrayMandatoryValidator, Direction, FloatBoundedValidator, logger

from pdb import set_trace as tr

class DSFinterp(PythonAlgorithm):

  def category(self):
    return "Arithmetic"

  def name(self):
    return 'DSFinterp'

  def PyInit(self):
    arrvalidator = StringArrayMandatoryValidator()
    self.declareProperty(StringArrayProperty('Workspaces', values=[], validator=arrvalidator, direction=Direction.Input), doc='list of input workspaces')
    self.declareProperty(FloatArrayProperty('ParameterValues', values=[], direction=Direction.Input), doc='list of input parameter values')
    self.declareProperty('RegressionWindow',0, direction=Direction.Input, doc='window for the running local-regression. No regression if value set to zero')
    self.declareProperty('RegressionType','linear',direction=Direction.Input, doc='type of local-regression; linear and quadratic are available')
    self.declareProperty('TargetParameter', 0.0, direction=Direction.Input, doc="Parameter to interpolate the structure factor")
    self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '', direction=Direction.Output), doc='Workspace to save the interpolated structure factor')

  def areWorkspacesCompatible(self, a, b):
    sizeA = a.blocksize() * a.getNumberHistograms() 
    sizeB = b.blocksize() * b.getNumberHistograms()
    return sizeA == sizeB

  def PyExec(self):
    # Check congruence of workspaces
    workspaces = self.getProperty('Workspaces').value
    fvalues = self.getProperty('ParameterValues').value
    if len(workspaces) != len(fvalues):
      logger.error('Number of workspaces and fvalues should be the same')
      return
    targetfvalue = self.getProperty('TargetParameter').value
    if targetfvalue < min(fvalues) or targetfvalue > max(fvalues):
      logger.error('the target fvalue should lie in [{0}, {1}]'.format(min(fvalues),max(fvalues)))
    for workspace in workspaces[1:]:
      if not self.areWorkspacesCompatible(mtd[workspaces[0]],mtd[workspace]):
        logger.error('Workspace {0} incompatible with {1}'.format(workspace, workspaces[0]))
        return
    # Load the workspaces into a group of dynamic structure factors
    from dsfinterp.dsf import Dsf
    from dsfinterp.dsfgroup import DsfGroup
    from dsfinterp.channelgroup import ChannelGroup
    dsfgroup = DsfGroup()
    for idsf in range(len(workspaces)):
      dsf = Dsf()
      dsf.Load( mtd[workspaces[idsf]] )
      dsf.SetFvalue( fvalues[idsf] )
      dsfgroup.InsertDsf(dsf)
    # Create the intepolator
    channelgroup = ChannelGroup()
    channelgroup.InitFromDsfGroup(dsfgroup)
    regressiontype = self.getProperty('RegressionType').value
    windowlength = self.getProperty('RegressionWindow').value
    channelgroup.InitializeInterpolator(running_regr_type=regressiontype, windowlength=windowlength)
    # Invoke the interpolator and save to outputworkspace
    dsf = channelgroup(targetfvalue)
    #tr()
    outws = CloneWorkspace( mtd[workspaces[0]])
    dsf.Save(outws) # overwrite dataY and dataE
    self.setProperty("OutputWorkspace", outws)
#############################################################################################

try:
  import dsfinterp
  AlgorithmFactory.subscribe(DSFinterp)
except:
  logger.error('Failed to subscribe algorithm DSFinterp')
  pass
