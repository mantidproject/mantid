from mantid.api import *
import math
import numpy as np
from scipy import special as sp

class Bessel(IFunction1D):

  def category(self):
    return "Muon"

  def init(self):
    self.declareParameter("A0",1)
    self.declareParameter("fi",0.1)
    self.declareParameter("nu",0.1)
    
  def function1D(self, x):
    A0 = self.getParameterValue("A0")
    fi = self.getParameterValue("fi")
    nu = self.getParameterValue("nu")
    return A0*sp.j0(2*np.pi*nu*x+fi)
FunctionFactory.subscribe(Bessel)
