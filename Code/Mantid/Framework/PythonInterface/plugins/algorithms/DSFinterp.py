"""*WIKI* 

== Summary == 

Given a set of parameter values {<math>T_i</math>} and corresponding structure factors {<math>S(Q,E,T_i)</math>}, this
algorithm interpolates <math>S(Q,E,T)</math> for any value of parameter T within the range spanned by the {<math>T_i</math>} set.

== Usage ==

DSFinterp(Workspaces,OutputWorkspaces,[LoadErrors],[ParameterValues],
 [LocalRegression],[RegressionWindow],[RegressionType],[TargetParameters],
 [Version])
 
<br clear=all>
 
== Properties ==
 
{| border="1" cellpadding="5" cellspacing="0" 
!Order
!Name
!Direction
!Type
!Default
!Description
|-
|colspan=6 align=center|'''Input'''
|-
|1
|Workspaces
|Input
|str list
|Mandatory
|list of input workspaces
|-
|2
|LoadErrors
|Input
|boolean
| True
|Do we load error data contained in the workspaces?
|-
|3
|ParameterValues
|Input
|dbl list
|Mandatory
|list of input parameter values
|-
|colspan=6 align=center|'''Running Local Regression Options'''
|-
|4
|LocalRegression
|Input
|boolean
| True
|Perform running local-regression?
|-
|5
|RegressionWindow
|Input
|number
| 6
|window size for the running local-regression
|-
|6
|RegressionType
|Input
|string
| quadratic
|type of local-regression; linear and quadratic are available
|-
|colspan=6 align=center|'''Output'''
|-
|7
|TargetParameters
|Output
|dbl list
|Mandatory
|Parameters to interpolate the structure factor
|-
|8
|OutputWorkspaces
|Output
|str list
|Mandatory
|list of output workspaces to save the interpolated structure factors
|-
|}

== Required ==

This algorithm requires python package [https://github.com/camm-sns/dsfinterp dsfinterp], available at the [https://pypi.python.org/pypi/dsfinterp python package index]. if the package is not present, this algorithm will not be available. To install:
 sudo pip install dsfinterp

== Details ==

For every "dynamical channel" defined by one particular (Q,E) pair, the sequence of scalars 
{<math>{S_i \equiv S(Q,E,T_i)}</math>} ordered by increasing value of T is interpolated with a cubic spline, which then can be invoked to obtain
<math>S(Q,E,T)</math> at any T value.

Errors in the structure factor are incorporated when constructing the spline, so that the spline need not neccessarily pass trough the  <math>(T_i, S_i)</math> points. This has the desirable effect of producing smooth spline curves when the variation of the structure factors versus <math>T</math> contains significant noise. For more details on the construction of the spline, see [http://docs.scipy.org/doc/scipy/reference/generated/scipy.interpolate.UnivariateSpline.html UnivariateSpline].

[[Image:DSFinterp_local_regression.png|thumb|300px|Local quadratic regression of windowsize w=7 starting at index n=2]]

If the structure factors have no associated errors, an scenario typical of structure factors derived from simulations, then error estimation can be implemented with the running, local regression option. A local regression of windowsize <math>w</math> starting at index <math>n</math> performs a linear squares minimization <math>F</math> on the set of points <math>(T_n,S_n),..,(T_{n+w},S_{n+w})</math>. After the minimization is done, we record the expected value and error at <math>T_{n+w/2}</math>:

value: <math>S_{n+w/2}^' = F(T_{n+w/2})</math>

error: <math>e_{n+w/2} = \sqrt(\frac{1}{w}\sum_{j=n}^{n+w}(S_j-F(T_j))^2)</math>

As we slide the window along the T-axis, we obtain values and errors at every <math>T_i</math>. We use the {<math>F(T_i)</math>} values and {<math>e_i</math>} errors to produce a smooth spline, as well as expected errors at any <math>T</math> value.

== Example ==

Our example system is a simulation of a small crystal of octa-methyl [http://www.en.wikipedia.org/wiki/Silsesquioxane silsesqioxane] molecules. A total of 26 molecular dynamics simulations were performed under different values of the energy barrier to methyl rotations, <math>K</math>. Dynamics structure factors S(Q,E) were derived from each simulation.

[[Image:DSFinterp_fig3.png|thumb|center|600px|Interpolated spline (solid line) with associated errors at one (Q,E) dynamical channel. Red dots are values from the simulation used to construct the spline.]]

There are as many splines as dynamical channels. The algorithm gathers the interpolations for each channel and aggregates them into an interpolated structure factor.

[[Image:DSFinterp_fig4.png|thumb|center|600px|Interpolated structure factor <math>S(K,E|Q)</math>, in logarithm scaling, at fixed <math>Q=0.9A^{-1}</math>.]]

[[Category:Algorithms]]
[[Category:Utility]]
[[Category:PythonAlgorithms]]
[[Category:Transforms]]
[[Category:Smoothing]]
{{AlgorithmLinks|DSFinterp}}

*WIKI*"""

from mantid.api import PythonAlgorithm, MatrixWorkspaceProperty, AlgorithmFactory
from mantid.simpleapi import CloneWorkspace, mtd
from mantid.kernel import StringListValidator, FloatArrayProperty, FloatArrayLengthValidator, FloatArrayMandatoryValidator, StringArrayProperty, StringArrayMandatoryValidator, Direction, FloatBoundedValidator, logger

from pdb import set_trace as tr

class DSFinterp(PythonAlgorithm):

  def category(self):
    return "Transforms\\Smoothing;Utility;PythonAlgorithms"

  def name(self):
    return 'DSFinterp'

  def PyInit(self):
    arrvalidator = StringArrayMandatoryValidator()
    lrg='Input'
    self.declareProperty(StringArrayProperty('Workspaces', values=[], validator=arrvalidator, direction=Direction.Input), doc='list of input workspaces')
    self.declareProperty('LoadErrors', True, direction=Direction.Input, doc='Do we load error data contained in the workspaces?')
    self.declareProperty(FloatArrayProperty('ParameterValues', values=[], validator=FloatArrayMandatoryValidator(),direction=Direction.Input), doc='list of input parameter values')
    self.setPropertyGroup('Workspaces', lrg)
    self.setPropertyGroup('LoadErrors', lrg)
    self.setPropertyGroup('ParameterValues', lrg)
    self.declareProperty('LocalRegression', True, direction=Direction.Input, doc='Perform running local-regression?')
    self.declareProperty('RegressionWindow', 6, direction=Direction.Input, doc='window size for the running local-regression')
    regtypes = [ 'linear', 'quadratic']
    self.declareProperty('RegressionType', 'quadratic', StringListValidator(regtypes), direction=Direction.Input, doc='type of local-regression; linear and quadratic are available')
    lrg = 'Running Local Regression Options'
    self.setPropertyGroup('LocalRegression', lrg)
    self.setPropertyGroup('RegressionWindow', lrg)
    self.setPropertyGroup('RegressionType', lrg)
    lrg='Output'
    self.declareProperty(FloatArrayProperty('TargetParameters', values=[], ), doc="Parameters to interpolate the structure factor")
    self.declareProperty(StringArrayProperty('OutputWorkspaces', values=[], validator=arrvalidator), doc='list of output workspaces to save the interpolated structure factors')
    self.setPropertyGroup('TargetParameters', lrg)
    self.setPropertyGroup('OutputWorkspaces', lrg)
    self.channelgroup = None

  def areWorkspacesCompatible(self, a, b):
    sizeA = a.blocksize() * a.getNumberHistograms() 
    sizeB = b.blocksize() * b.getNumberHistograms()
    return sizeA == sizeB

  def PyExec(self):
    # Check congruence of workspaces
    workspaces = self.getProperty('Workspaces').value
    fvalues = self.getProperty('ParameterValues').value
    if len(workspaces) != len(fvalues):
      mesg = 'Number of Workspaces and ParameterValues should be the same'
      #logger.error(mesg)
      raise IndexError(mesg)
    for workspace in workspaces[1:]:
      if not self.areWorkspacesCompatible(mtd[workspaces[0]],mtd[workspace]):
        mesg = 'Workspace {0} incompatible with {1}'.format(workspace, workspaces[0])
        logger.error(mesg)
        raise ValueError(mesg)
    # Load the workspaces into a group of dynamic structure factors
    from dsfinterp.dsf import Dsf
    from dsfinterp.dsfgroup import DsfGroup
    from dsfinterp.channelgroup import ChannelGroup
    dsfgroup = DsfGroup()
    for idsf in range(len(workspaces)):
      dsf = Dsf()
      dsf.Load( mtd[workspaces[idsf]] )
      if not self.getProperty('LoadErrors').value:
        dsf.errors = None # do not incorporate error data
      dsf.SetFvalue( fvalues[idsf] )
      dsfgroup.InsertDsf(dsf)
    # Create the intepolator if not instantiated before
    if not self.channelgroup:
      self.channelgroup = ChannelGroup()
      self.channelgroup.InitFromDsfGroup(dsfgroup)
      localregression = self.getProperty('LocalRegression').value
      if localregression:
        regressiontype = self.getProperty('RegressionType').value
        windowlength = self.getProperty('RegressionWindow').value
        self.channelgroup.InitializeInterpolator(running_regr_type=regressiontype, windowlength=windowlength)
      else:
        self.channelgroup.InitializeInterpolator(windowlength=0)
    # Invoke the interpolator and generate the output workspaces
    targetfvalues = self.getProperty('TargetParameters').value
    for targetfvalue in targetfvalues:
      if targetfvalue < min(fvalues) or targetfvalue > max(fvalues):
        mesg = 'Target parameters should lie in [{0}, {1}]'.format(min(fvalues),max(fvalues))
        logger.error(mesg)
        raise ValueError(mesg)
    outworkspaces = self.getProperty('OutputWorkspaces').value
    if len(targetfvalues) != len(outworkspaces):
      mesg = 'Number of OutputWorkspaces and TargetParameters should be the same'
      logger.error(mesg)
      raise IndexError(mesg)
    for i in range(len(targetfvalues)):
      outworkspace = outworkspaces[i]
      dsf = self.channelgroup( targetfvalues[i] )
      outws = CloneWorkspace( mtd[workspaces[0]], OutputWorkspace=outworkspaces[i])
      dsf.Save(outws) # overwrite dataY and dataE

#############################################################################################

try:
  import dsfinterp
  AlgorithmFactory.subscribe(DSFinterp)
except:
  logger.debug('Failed to subscribe algorithm DSFinterp; Python package dsfinterp may be missing (https://pypi.python.org/pypi/dsfinterp)')
  pass
