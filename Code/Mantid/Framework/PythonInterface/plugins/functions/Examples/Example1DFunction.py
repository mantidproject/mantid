"""
This example implements a simple Linear function that could be used as a background.

It uses the generic concept of a 1D function, which has no concept that the function
may have a peak-like attributes such as a centre, width or fwhm. If your function can be defined
to have meaningful concepts such as this then see ExamplePeakFunction.

1D functions do not have to have a derivative defined, if they do not then they will use a numerical
derivative
"""
from mantid.api import IFunction1D, FunctionFactory
from mantid import logger
import math
import numpy as np

class Example1DFunction(IFunction1D):

    def category(self):
        """
        Optional method to return the category that this
        function should be listed under. Multiple categories
        should be separated with a semi-colon(;). Sub-categories
        can be specified using a \\ separator, e.g. Category\\Sub-category
        """
        return "Examples"

    def init(self):
        """
        Declare parameters that participate in the fitting (declareParameter)
        and attributes that are constants to be passed (declareAttribute) in
        and do not participate in the fit. Attributes must have type=int,float,string,bool
        See ExamplePeakFunction for more on attributes
        """
        # Active fitting parameters
        self.declareParameter("A0")
        self.declareParameter("A1")

    def function1D(self, xvals):
        """
        Computes the function on the set of values given and returns
        the answer as a numpy array of floats
        """
        return self.getParameterValue("A0") +  self.getParameterValue("A1")*xvals

    def functionDeriv1D(self, xvals, jacobian):
        """
        Computes the partial derivatives of the function on the set of values given
        and the sets these values in the given jacobian. The Jacobian is essentially
        a matrix where jacobian.set(iy,ip,value) takes 3 parameters:
            iy = The index of the data value whose partial derivative this corresponds to
            ip = The index of the parameter value whose partial derivative this corresponds to
            value = The value of the derivative
        """
        i = 0
        for x in xvals:
            jacobian.set(i,0,1);
            jacobian.set(i,1,x);
            i += 1

# Required to have Mantid recognise the new function
FunctionFactory.subscribe(Example1DFunction)
