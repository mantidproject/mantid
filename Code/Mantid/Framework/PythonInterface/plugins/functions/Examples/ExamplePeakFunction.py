#pylint: disable=no-init,invalid-name
"""
This example reimplements a Gaussian fitting function. It is not meant to
be used in production for fitting, it is simply provided as a relatively complete
guide to creating a Fit function.

It uses the so-called IPeakFunction that should be used when there is a sensible way to
calculate the centre, height & fwhm of the function. If it does not make sense, for example a in linear background,
where does not give a peak shape, then see the more general Example1DFunction that does not require these concepts.
"""
from mantid.api import IPeakFunction, FunctionFactory
import math
import numpy as np

class ExamplePeakFunction(IPeakFunction):

    _nterms = None

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
        """
        # Active fitting parameters
        self.declareParameter("Height")
        self.declareParameter("PeakCentre")
        self.declareParameter("Sigma")

        # Simple attributes required for the function but
        # not as part of the fit itself e.g. number of terms to evaluate in some expression
        # They must have a default value.
        # It is advisable to look at the setAttributeValue function below and take local copies
        # of attributes so that they do not have to be retrieved repeatedly througout the fitting.
        self.declareAttribute("NTerms", 1)

    def functionLocal(self, xvals):
        """
        Computes the function on the set of values given and returns
        the answer as a numpy array of floats
        """
        # As Fit progresses the declared parameter values will change
        height = self.getParameterValue("Height") # Can also be retrieve by index self.getParameterValue(0)
        peak_centre = self.getParameterValue("PeakCentre")
        sigma = self.getParameterValue("Sigma")
        weight = math.pow(1./sigma,2)

        # Here you can use the NTerms attr if required by
        #   using self._nterms: see setAttributeValue below or
        #   accessing the attribute each time directly nterms = self.getAttributeValue("NTerms") but this is much slower

        offset_sq=np.square(xvals-peak_centre)
        out=height*np.exp(-0.5*offset_sq*weight)
        return out

    def functionDerivLocal(self, xvals, jacobian):
        """
        Computes the partial derivatives of the function on the set of values given
        and the sets these values in the given jacobian. The Jacobian is essentially
        a matrix where jacobian.set(iy,ip,value) takes 3 parameters:
            iy = The index of the data value whose partial derivative this corresponds to
            ip = The index of the parameter value whose partial derivative this corresponds to
            value = The value of the derivative
        """
        height = self.getParameterValue("Height")
        peak_centre = self.getParameterValue("PeakCentre")
        sigma = self.getParameterValue("Sigma")
        weight = math.pow(1./sigma,2)

        # X index
        i = 0
        for x in xvals:
            diff = x-peak_centre
            exp_term = math.exp(-0.5*diff*diff*weight)
            jacobian.set(i,0, exp_term)
            jacobian.set(i,1, diff*height*exp_term*weight)
            # derivative with respect to weight not sigma
            jacobian.set(i,2, -0.5*diff*diff*height*exp_term)
            i += 1

    def setAttributeValue(self, name, value):
        """
        This is called by the framework when an attribute is passed to Fit and its value set.
        It's main use is to store the attribute value on the object once to avoid
        repeated calls during the fitting process
        """
        if name == "NTerms":
            self._nterms = value

    def activeParameter(self, index):
        """
        Returns the value of the parameter that
        is taking part in the fitting for the given index.
        Only required if the fitting is to be done over
        a different parameter than declared for some reason, i.e
        stability
        """
        param_value = self.getParameterValue(index)
        if index == 2: #Sigma. Actually fit to 1/(sigma^2) for stability
            return 1./math.pow(param_value,2)
        else:
            return param_value

    def setActiveParameter(self, index, value):
        """
        Called by the fitting framework when a parameter value is updated.
        Only required if the fitting is done over a different parameter
        set than that declared
        """
        param_value = value
        if index == 2:
            param_value = math.sqrt(math.fabs(1.0/value))
        else:
            param_value = value
        # Final explicit arugment is required to be false here by framework
        self.setParameter(index, param_value, False)

        param_value = self.getParameterValue(index)
        if index == 2: #Sigma. Actually fit to 1/(sigma^2) for stability
            return math.pow(1./param_value,2)
        else:
            return param_value

    def centre(self):
        """
        Return what should be considered the centre of this function. In this
        simple case it is just the centre value but it can be any combination
        of parameters
        """
        return self.getParameterValue("PeakCentre")

    def height(self):
        """
        Return what should be considered the 'height' of this function. In this
        simple case it is just the centre value but it can be any combination
        of parameters
        """
        return self. getParameterValue("Height")

    def fwhm(self):
        """
        Return what should be considered the 'fwhm' of this function.
        """
        return 2.0*math.sqrt(2.0*math.log(2.0))*self.getParameterValue("Sigma")

    def setCentre(self, new_centre):
        """
        Called by an external entity, probably a GUI, in response to a mouse click
        that gives a guess at the centre.
        """
        self.setParameter("PeakCentre",new_centre)

    def setHeight(self, new_height):
        """
        Called by an external entity, probably a GUI, in response to a user guessing
        the height.
        """
        self.setParameter("Height", new_height)

    def setFwhm(self, new_fwhm):
        """
        Called by an external entity, probably a GUI, in response to a user guessing
        the height.
        """
        sigma = new_fwhm/(2.0*math.sqrt(2.0*math.log(2.0)))
        self.setParameter("Sigma",sigma)

# Required to have Mantid recognise the new function
FunctionFactory.subscribe(ExamplePeakFunction)
