import numpy as np
from mantid.api import IFunction1D, FunctionFactory
from matplotlib.mlab import bivariate_normal


class BivariateGaussian(IFunction1D): 
    """
    BivariateGaussian implements a bivariate gaussian (BivariateGaussian) in Mantid (M) as a 1D function.  This is done so that it can be
    fit in a straightforward fashion using Mantid's Fit() function.  To achieve this, we use the flattened
    version of the 2D profile and fit it as a 1D function.  It is built on matplotlib.mlab.bivariate_normal, which
    is available on SNS analysis machines.

    To make it compatible with fitting, X, Y, and E must all be the same shape.  This is possible if we input
    twice and reconstruct the BivariateGaussian in the function.

    If h is an n*n 2d profile we're trying to fitt and th, ph are the n*1 arrays containing the x and y coordinates,
    we would fit it as follows:


        TH, PH = np.meshgrid(th, ph,indexing='ij') #Get 2D version
        m = BivariateGaussian.BivariateGaussian()
        m.init()
        m['A'] = 1.0
        # ... #Set initial parameters
        m['nX'] = len(th) #Length needed for reconstruction
        m['nY'] = len(ph) #Length needed for reconstruction

        pos = np.empty(TH.shape + (2,))
        pos[:,:,0] = TH
        pos[:,:,1] = PH
        h2 = m.function2D(pos)

        H = np.empty(h.shape + (2,))
        H[:,:,0] = h
        H[:,:,1] = h

        bvgWS = CreateWorkspace(OutputWorkspace='bvgWS',DataX=pos.ravel(),DataY=H.ravel(),dataE=np.sqrt(H.ravel()))
        fitResults = Fit(Function=m, InputWorkspace='bvgWS', Output='bvgfit')

        BS - May 18 2018
    """
    def init(self):
        self.declareParameter("A") #Amplitude
        self.declareParameter("muX") #Mean along the x direction
        self.declareParameter("muY") #Mean along the y direction
        self.declareParameter("sigX") #sigma along the x direction
        self.declareParameter("sigY") #sigma along the y direction
        self.declareParameter("sigP") #interaction term rho
        self.declareParameter("bg") #constant BG terms
        self.declareAttribute("nX", 50) #used for reconstructing 2d profile
        self.declareAttribute("nY", 50) #used for reconstruction 2d profile
        self.addConstraints("0 < A") #Require amplitude to be positive
        self.addConstraints("0 < sigX") #standard deviations must be positive
        self.addConstraints("0 < sigY") #standard deviations must be positive
        self.addConstraints("0 < bg") #standard deviations must be positive

    def function1D(self, t):
        """
        function1D returns the flattened version of the function.
        Input, t, may be in one of two forms:
            1) a 1D array (e.g. pos.ravel() from the example).
            2) a 3D array (e.g. pos from the example)
        Output
            If input is of type 1, a 1D array matching the size of the
            input is returned.  This allows fitting to take place.
            If input is of type 2, a 1D array matching the size of
            pos.ravel() is returned.
        """
        if t.ndim == 1:
            nX = int(self.getAttributeValue('nX'))
            nY = int(self.getAttributeValue('nY'))
            pos = t.reshape(nX, nY, 2)
        elif t.ndim == 3:
            pos = t
        X = pos[...,0]
        Y = pos[...,1]

        A = self.getParamValue(0)
        muX = self.getParamValue(1)
        muY = self.getParamValue(2)
        sigX = self.getParamValue(3)
        sigY = self.getParamValue(4)
        sigP = self.getParamValue(5)
        bg = self.getParamValue(6)

        sigXY = sigX*sigY*sigP
        Z = A*bivariate_normal(X,Y, sigmax=sigX, sigmay=sigY,
                               mux=muX,muy=muY,sigmaxy=sigXY)
        if t.ndim == 1:
            zRet = np.empty(Z.shape+(2,))
            zRet[:,:,0] = Z
            zRet[:,:,1] = Z
        elif t.ndim == 3:
            zRet = Z
        zRet += bg
        return zRet.ravel()

    def getMu(self):
        muX = self.getParamValue(1)
        muY = self.getParamValue(2)
        return np.array([muX, muY])

    def getSigma(self):
        sigX = self.getParamValue(3)
        sigY = self.getParamValue(4)
        sigP = self.getParamValue(5)
        return np.array([[sigX**2,sigX*sigY*sigP],[sigX*sigY*sigP, sigY**2]])

    def getMuSigma(self):
        return self.getMu(), self.getSigma()

    def setConstraints(self, boundsDict):
        """
        setConstraints sets fitting constraints for the mbvg function.
        Intput:
            boundsDict: a dictionary object where each key is a parameter as a string
                        ('A', 'muX', 'muY', 'sigX', 'sigY', 'sigP', 'bg') and the value is
                        an array with the lower bound and upper bound
        """
        for param in boundsDict.keys():
            try:
                if boundsDict[param][0] < boundsDict[param][1]:
                    constraintString = "{:4.4e} < {:s} < {:4.4e}".format(boundsDict[param][0], param, boundsDict[param][1])
                    self.addConstraints(constraintString)
                else:
                    print 'Setting constraints on mbvg; reversing bounds'
                    self.addConstraints("{:4.4e} < A < {:4.4e}".format(boundsDict[param][1], boundsDict[param][0]))
            except ValueError:
                print 'Cannot set parameter {:s} for mbvg.  Valid choices are', \
                      '(\'A\', \'muX\', \'muY\', \'sigX\', \'sigY\', \'sigP\', \'bg\')'.format(param)

    def function2D(self, t):
        """
        function2D returns the 2D version of the BivariateGaussian.
        Input may be in two forms:
            1) 1D array (e.g. pos.ravel()).  This will be reshaped into an nX*nY*2 array, so
                it must contain nX*nY*2 elements.
            2) 3D array of size A*B*2. A and B are arbitrary integers.
        Output:
            a 2D array either size nX*nY (intput type 1) or A*B (input type 2) with intensities
            of the BivariateGaussian.
        """
        if t.ndim == 1:
            nX = int(self.getAttributeValue('nX'))
            nY = int(self.getAttributeValue('nY'))
            pos = t.reshape(nX, nY, 2)
        elif t.ndim == 3:
            pos = t
        X = pos[...,0]
        Y = pos[...,1]
        A = self.getParamValue(0)
        muX = self.getParamValue(1)
        muY = self.getParamValue(2)
        sigX = self.getParamValue(3)
        sigY = self.getParamValue(4)
        sigP = self.getParamValue(5)
        bg = self.getParamValue(6)

        sigXY = sigX*sigY*sigP
        Z = A*bivariate_normal(X,Y, sigmax=sigX, sigmay=sigY,
                               mux=muX,muy=muY,sigmaxy=sigXY)
        Z += bg
        return Z

    def function3D(self, t):
        if t.ndim == 4:
            pos = t[:,:,:,1:]
        X = pos[...,0]
        Y = pos[...,1]
        A = self.getParamValue(0)
        muX = self.getParamValue(1)
        muY = self.getParamValue(2)
        sigX = self.getParamValue(3)
        sigY = self.getParamValue(4)
        sigP = self.getParamValue(5)
        bg = self.getParamValue(6)

        sigXY = sigX*sigY*sigP
        Z = A*bivariate_normal(X,Y, sigmax=sigX, sigmay=sigY,
                               mux=muX,muy=muY,sigmaxy=sigXY)
        Z += bg
        return Z

    # Evaluate the function for a differnt set of paremeters (trialc)
    def function1DDiffParams(self, xvals, trialc):
        #First, grab the original parameters and set to trialc
        c = np.zeros(self.numParams())
        for i in range(self.numParams()):
            c[i] = self.getParamValue(i)
            self.setParameter(i, trialc[i])

        #Get the trial values
        f_trial = self.function1D(xvals)

        #Now return to the orignial
        for i in range(self.numParams()):
            self.setParameter(i, c[i])
        return f_trial

    # Construction the Jacobian (df) for the function
    def functionDeriv1D(self, xvals, jacobian, eps=1.e-3):
        f_int = self.function1D(xvals)
        #Fetch parameters into array c
        c = np.zeros(self.numParams())
        for i in range(self.numParams()):
            c[i] = self.getParamValue(i)
        nc = np.prod(np.shape(c))
        for k in range(nc):
            dc = np.zeros(nc)
            if k == 1 or k == 2:
                epsUse = 1.e-3
            else:
                epsUse = eps
            dc[k] = max(epsUse,epsUse*c[k])
            f_new = self.function1DDiffParams(xvals,c+dc)
            for i,dF in enumerate(f_new-f_int):
                jacobian.set(i,k,dF/dc[k])


FunctionFactory.subscribe(BivariateGaussian)
