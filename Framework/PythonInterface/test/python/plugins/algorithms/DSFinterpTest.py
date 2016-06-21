import unittest,os
import mantid
import numpy
import pdb
from mantid.kernel import logger

class DSFinterpTest(unittest.TestCase):

  def generateWorkspaces(self, nf):
    n=200
    dE = 0.004 #typical spacing for QENS experiments in BASIS, in meV
    xvalues = dE * numpy.arange(-n, n)
    fvalues = []
    workspaces = []
    for f in range(1,1+nf):
      HWHM = 0.01*f # half-width at half maximum
      yvalues = 1/numpy.pi * HWHM / (HWHM*HWHM + xvalues*xvalues)
      evalues = yvalues*0.1*numpy.random.rand(2*n) # errors
      mantid.simpleapi.CreateWorkspace(OutputWorkspace='sim{0}'.format(f), DataX=evalues, DataY=yvalues, DataE=evalues)
      fvalues.append(f)
      workspaces.append('sim{0}'.format(f))
    return fvalues, workspaces

  def cleanup(self, nf):
    for f in range(1,nf):
      mantid.api.AnalysisDataService.remove('sim{0}'.format(f))

  def test_input_exceptions(self):
    # Run the test only if dsfinterp package is present
    try:
      import dsfinterp
    except:
      logger.debug('Python package dsfinterp is missing (https://pypi.python.org/pypi/dsfinterp)')
      return
    nf = 9
    fvalues, workspaces = self.generateWorkspaces(nf) # workspaces sim1 to sim9 (nine workpaces)
    # Try passing different number of workspaces and parameter values
    try:
      fvalueswrong = range(nf-1) # eight values
      mantid.simpleapi.DSFinterp(Workspaces=workspaces, ParameterValues=fvalueswrong, LocalRegression=False, TargetParameters=5.5, OutputWorkspaces='outws')
    except Exception as e:
      self.assertTrue('Number of Workspaces and ParameterValues should be the same' in str(e))
    else:
      assert False, "Didn't raise any exception"
    # Try passing an incompatible workspace
    try:
      mantid.simpleapi.CreateWorkspace(OutputWorkspace='sim10', DataX='1,2,3', DataY='1,1,1', DataE='0,0,0')
      fvalues2 = fvalues+[10,]
      workspaces2 = workspaces + ['sim10',]
      mantid.simpleapi.DSFinterp(Workspaces=workspaces2, ParameterValues=fvalues2, LocalRegression=False, TargetParameters=5.5, OutputWorkspaces='outws')
    except Exception as e:
      self.assertTrue('Workspace sim10 incompatible with sim1' in str(e))
    else:
      assert False, "Didn't raise any exception"
    mantid.api.AnalysisDataService.remove('sim10')
    #Try passing a target parameter outside range
    try:
      mantid.simpleapi.DSFinterp(Workspaces=workspaces, ParameterValues=fvalues, LocalRegression=False, TargetParameters=nf+1, OutputWorkspaces='outws')
    except Exception as e:
      self.assertTrue('Target parameters should lie in' in str(e))
    else:
      assert False, "Didn't raise any exception"
    # Try passing a different number of target parameters and output workspaces
    try:
      mantid.simpleapi.DSFinterp(Workspaces=workspaces, ParameterValues=fvalues, LocalRegression=False, TargetParameters=[1,2], OutputWorkspaces='outws')
    except Exception as e:
      self.assertTrue('Number of OutputWorkspaces and TargetParameters should be the same' in str(e))
    else:
      assert False, "Didn't raise any exception"
    self.cleanup(nf)

if __name__=="__main__":
    unittest.main()
