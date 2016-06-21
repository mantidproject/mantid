import unittest
import numpy

from mantid.kernel import logger
from mantid.simpleapi import CreateWorkspace, Fit, mtd, SaveNexus
from mantid.api import AnalysisDataService
import sys

class DSFinterp1DTestTest(unittest.TestCase):

  def generateWorkspaces(self, nf, startf, df, e=False, seed=10):
    '''Helper function. Generates input workspaces for testing

    Generates a set of one-histogram workspaces, each containing a Lorentzian. Also
    generate a target lorentzian agains which we will fit

    Arguments:
      nf: number of InputWorkspaces
      startf: first theoretical HWHM
      df: separation between consecutive theoretical HWHM
      [e]: if true, the theoretical HWHM and the actual HWHM used to construct
        the lorentzian are different by a random amount.
      [seed]: seed for the random generators. Default to 10.

    Returns:
      fvalues: list of theoretical HWHM
      InputWorkspaces: names of the generated InputWorkspaces
      xvalues: energy domains
      HWHM: the HWHM of the target lorentzian against which we fit, stored
        in workspace of name "targetW"
    '''
    import random
    random.seed(seed)
    numpy.random.seed(seed)
    n=200
    dE = 0.004 #typical spacing for QENS experiments in BASIS, in meV
    xvalues = dE * numpy.arange(-n, n)
    fvalues = ''
    InputWorkspaces = ''
    for iif in range(nf):
      f = startf + iif*df
      HWHM = f# half-width at half maximum
      # Impose uncertainty in the Lorentzian by making its actual HWHM different than the theoretical one
      if e:
        HWHM += 2*df*(0.5-random.random()) # add uncertainty as big as twice the step between consecutive HWHM!!!
        if HWHM < 0: HWHM = f
      yvalues = 1/numpy.pi * HWHM / (HWHM*HWHM + xvalues*xvalues) * (1.1 - 0.2*numpy.random.rand(2*n)) #10% random noise
      evalues = yvalues*0.1*numpy.random.rand(2*n) # errors
      dataX = numpy.append(xvalues-dE/2.0, n*dE-dE/2.0) # histogram bins
      CreateWorkspace(OutputWorkspace='sim{0}'.format(iif), DataX=dataX, DataY=yvalues, DataE=evalues)
      #SaveNexus(InputWorkspace='sim{0}'.format(iif), Filename='/tmp/sim{0}.nxs'.format(iif))  #only for debugging purposes
      #print iif, f, HWHM
      fvalues += ' {0}'.format(f) # theoretical HWHM, only coincides with real HWHM when no uncertainty
      InputWorkspaces += ' sim{0}'.format(iif)
    # Target workspace against which we will fit
    HWHM = startf + (nf/2)*df
    yvalues = 1/numpy.pi * HWHM / (HWHM*HWHM + xvalues*xvalues)
    evalues = yvalues*0.1*numpy.random.rand(2*n) # errors
    CreateWorkspace(OutputWorkspace='targetW', DataX=dataX, DataY=yvalues, DataE=evalues)
    #SaveNexus(InputWorkspace='targetW', Filename='/tmp/targetW.nxs') #only for debugging purposes
    #print HWHM
    return fvalues, InputWorkspaces, xvalues, HWHM

  def cleanup(self, nf):
    for iif in range(nf):
      AnalysisDataService.remove('sim{0}'.format(iif))
    AnalysisDataService.remove('targetW')
    # Remove the fitting results, if present
    for suffix in 'NormalisedCovarianceMatrix Parameters Workspace':
      if mtd.doesExist('targetW_{0}'.format(suffix)):
        AnalysisDataService.remove('targetW_{0}'.format(suffix))

  def isthere_dsfinterp(self):
    try:
      import dsfinterp
    except:
      logger.debug('Python package dsfinterp is missing (https://pypi.python.org/pypi/dsfinterp)')
      return False
    return True

  def test_input_exceptions(self):
    ''' Test exceptions thrown upon wrong input '''
    if not self.isthere_dsfinterp():
      return
    import dsfinterp
    nf = 9
    fvalues, InputWorkspaces, xvalues, HWHM = self.generateWorkspaces(nf, 0.01, 0.01) # workspaces sim0 to sim8 (nine workpaces)
    # Try passing different number of InputWorkspaces and parameter values
    try:
      fvalueswrong = ' '.join(fvalues.split()[:-1]) # one less value
      func_string = 'name=DSFinterp1DFit,InputWorkspaces="{0}",ParameterValues="{1}",'.format(InputWorkspaces,fvalueswrong) +\
      'LoadErrors=0,LocalRegression=1,RegressionType=quadratic,RegressionWindow=6,' +\
      'WorkspaceIndex=0,Intensity=1.0,TargetParameter=0.01;'
      Fit( Function=func_string, InputWorkspace= 'targetW', WorkspaceIndex=0, StartX=xvalues[0], EndX=xvalues[-1], CreateOutput=1, MaxIterations=0 )
    except Exception as e:
      self.assertTrue('Number of InputWorkspaces and ParameterValues should be the same' in str(e))
    else:
      assert False, 'Did not raise any exception'
    # Try passing the wrong workspace index
    try:
      func_string = 'name=DSFinterp1DFit,InputWorkspaces="{0}",ParameterValues="{1}",'.format(InputWorkspaces,fvalues) +\
      'LoadErrors=0,LocalRegression=1,RegressionType=quadratic,RegressionWindow=6,' +\
      'WorkspaceIndex=1,Intensity=1.0,TargetParameter=0.01;'
      Fit( Function=func_string, InputWorkspace= 'targetW', WorkspaceIndex=0, StartX=xvalues[0], EndX=xvalues[-1], CreateOutput=1, MaxIterations=0 )
    except Exception as e:
      self.assertTrue('Numer of histograms in Workspace sim0 does not allow for workspace index 1' in str(e))
    else:
      assert False, 'Did not raise any exception'
    #Try passing the wrong type of regression
    try:
      func_string = 'name=DSFinterp1DFit,InputWorkspaces="{0}",ParameterValues="{1}",'.format(InputWorkspaces,fvalues) +\
      'LoadErrors=0,LocalRegression=1,RegressionType=baloney,RegressionWindow=6,' +\
      'WorkspaceIndex=0,Intensity=1.0,TargetParameter=0.01;'
      Fit( Function=func_string, InputWorkspace= 'targetW', WorkspaceIndex=0, StartX=xvalues[0], EndX=xvalues[-1], CreateOutput=1, MaxIterations=0 )
    except Exception as e:
      self.assertTrue('Regression type baloney not implemented' in str(e))
    else:
      assert False, 'Did not raise any exception'
    # Try passing an inappropriate regression window for the regression type selected
    try:
      func_string = 'name=DSFinterp1DFit,InputWorkspaces="{0}",ParameterValues="{1}",'.format(InputWorkspaces,fvalues) +\
      'LoadErrors=0,LocalRegression=1,RegressionType=quadratic,RegressionWindow=3,' +\
      'WorkspaceIndex=0,Intensity=1.0,TargetParameter=0.01;'
      Fit( Function=func_string, InputWorkspace= 'targetW', WorkspaceIndex=0, StartX=xvalues[0], EndX=xvalues[-1], CreateOutput=1, MaxIterations=0 )
    except Exception as e:
      self.assertTrue('RegressionWindow must be equal or bigger than 4 for regression type quadratic' in str(e))
    else:
      assert False, 'Did not raise any exception'
    self.cleanup(nf)

  def test_LorentzianFit(self):
    ''' Fit a set or Lorentzians with "noisy" HWHM against a reference lorentzian '''
    if not self.isthere_dsfinterp():
      return
    import dsfinterp
    nf=20
    startf=0.01
    df=0.01
    fvalues, InputWorkspaces, xvalues, HWHM = self.generateWorkspaces(nf, startf, df, e=True)
    # Do the fit starting from different initial guesses
    for iif in range(0,nf,6):
      guess = startf + iif*df
      func_string = 'name=DSFinterp1DFit,InputWorkspaces="{0}",ParameterValues="{1}",'.format(InputWorkspaces,fvalues) +\
      'LoadErrors=0,LocalRegression=1,RegressionType=quadratic,RegressionWindow=6,' +\
      'WorkspaceIndex=0,Intensity=1.0,TargetParameter={0};'.format(guess)
      Fit( Function=func_string, InputWorkspace= 'targetW', WorkspaceIndex=0, StartX=xvalues[0], EndX=xvalues[-1], CreateOutput=1, MaxIterations=20 )
      ws = mtd['targetW_Parameters']
      optimizedf = ws.row(1)[ 'Value' ]
      self.assertTrue( abs(1.0 - optimizedf/HWHM) < 0.05) #five percent error
    self.cleanup(nf)

if __name__=="__main__":
  unittest.main()
