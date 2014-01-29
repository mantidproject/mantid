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

from mantid.api import PythonAlgorithm, MatrixWorkspaceProperty
from mantid.simpleapi import CloneWorkspace, mtd
from mantid.kernel import StringMandatoryValidator, Direction


class InterpolateSQE(PythonAlgorithm):

  def category(self):
    return "Arithmetic"

  def name(self):
    return "InterpolateSQE"

  def PyInit(self):
    mustHaveWorkspaceNames = StringMandatoryValidator()
    self.declareProperty("Workspaces", "", validator=mustHaveWorkspaceNames, direction=Direction.Input, doc="Input workspaces. Comma separated workspace names")
    self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", Direction.Output), "Output interpolated workspace")

  def areWorkspacesCompatible(self, a, b):
    sizeA = a.blocksize() * a.getNumberHistograms() 
    sizeB = b.blocksize() * b.getNumberHistograms()
    return sizeA == sizeB

  def PyExec(self):
    import dsfinterp
    workspaces = self.getProperty("Workspaces").value.split(',')
    out_ws = CloneWorkspace(InputWorkspace=mtd[workspaces[0]], OutputWorkspace=self.getPropertyValue("OutputWorkspace"))
    for index in range(1, len(workspaces)):
      name = workspaces[index].strip()
      ws = mtd[name]
      if not self.areWorkspacesCompatible(out_ws, ws):
        raise RuntimeError("Input Workspaces are not the same shape.")
      out_ws += ws
    out_ws /= len(workspaces)
    self.setProperty("OutputWorkspace", out_ws)
        


#############################################################################################

try:
  import dsfinterp
  AlgorithmFactory.subscribe(Mean())
except:
  pass
