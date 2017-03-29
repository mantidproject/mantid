from __future__ import (absolute_import, division, print_function)
from mantid.api import PythonAlgorithm, AlgorithmFactory, IMDHistoWorkspaceProperty, PropertyMode, WorkspaceProperty, Progress
from mantid.kernel import (Direction, EnabledWhenProperty, PropertyCriterion, Property, StringListValidator, FloatArrayBoundedValidator,
                           FloatArrayProperty, FloatBoundedValidator)
from mantid.geometry import SpaceGroupFactory
from mantid import logger
import numpy as np


class DeltaPDF3D(PythonAlgorithm):

    def category(self):
        return 'Diffraction\\Utility'

    def name(self):
        return 'DeltaPDF3D'

    def summary(self):
        return 'Calculates the 3D-deltaPDF from a HKL workspace'

    def PyInit(self):
        self.declareProperty(IMDHistoWorkspaceProperty("InputWorkspace", "",
                                                       optional=PropertyMode.Mandatory,
                                                       direction=Direction.Input),
                             "Input Workspace")
        self.declareProperty(WorkspaceProperty("IntermediateWorkspace", "",
                                               optional=PropertyMode.Optional,
                                               direction=Direction.Output),
                             "The resulting workspace after reflection removal and filters applied. What is the input of the FFT.")
        self.declareProperty(WorkspaceProperty("OutputWorkspace", "",
                                               optional=PropertyMode.Mandatory,
                                               direction=Direction.Output),
                             "Output Workspace")
        self.declareProperty("RemoveReflections", True, "Remove reflections")
        condition = EnabledWhenProperty("RemoveReflections", PropertyCriterion.IsDefault)
        self.declareProperty("Shape", "sphere", doc="Shape to cut out reflections",
                             validator=StringListValidator(['sphere', 'cube']))
        self.setPropertySettings("Shape", condition)
        val_min_zero = FloatArrayBoundedValidator()
        val_min_zero.setLower(0.)
        self.declareProperty(FloatArrayProperty("Width", [0.1], validator=val_min_zero),
                             "Width of sphere/cube to remove reflections, in (HKL)")
        self.setPropertySettings("Width", condition)
        self.declareProperty("SpaceGroup", "", doc="Space groups, for reflection removal")
        self.setPropertySettings("SpaceGroup", condition)
        self.declareProperty("CutSphere", False, "Limit min/max q values. Can help with edge effects.")
        condition = EnabledWhenProperty("CutSphere", PropertyCriterion.IsNotDefault)
        self.declareProperty(FloatArrayProperty("SphereMin", [Property.EMPTY_DBL], validator=val_min_zero), "Min Sphere")
        self.setPropertySettings("SphereMin", condition)
        self.declareProperty(FloatArrayProperty("SphereMax", [Property.EMPTY_DBL], validator=val_min_zero), "Max Sphere")
        self.setPropertySettings("SphereMax", condition)
        self.declareProperty("Convolution", True, "Apply convolution to fill in removed reflections")
        condition = EnabledWhenProperty("Convolution", PropertyCriterion.IsDefault)
        self.declareProperty("ConvolutionWidth", 2.0, validator=FloatBoundedValidator(0.),
                             doc="Width of gaussian convolution in pixels")
        self.setPropertySettings("ConvolutionWidth", condition)
        self.declareProperty("Inpaint", False, "Use inpainting by biharmonic functions to fill remove reflactions")

    def validateInputs(self):
        issues = dict()

        inWS = self.getProperty("InputWorkspace").value
        dimX=inWS.getXDimension()
        dimY=inWS.getYDimension()
        dimZ=inWS.getZDimension()

        if dimX.name != '[H,0,0]' or dimY.name != '[0,K,0]' or dimZ.name != '[0,0,L]':
            issues['InputWorkspace'] = 'dimensions must be [H,0,0], [0,K,0] and [0,0,L]'

        if (dimX.getMaximum() != -dimX.getMinimum() or
                dimY.getMaximum() != -dimY.getMinimum() or
                dimZ.getMaximum() != -dimZ.getMinimum()):
            issues['InputWorkspace'] = 'dimensions must be centered on zero'

        if self.getProperty("Convolution").value:
            try:
                import astropy # noqa
            except ImportError:
                issues["Convolution"] = 'python-astropy required to do convolution'

        if self.getProperty("Inpaint").value:
            try:
                import skimage # noqa
            except ImportError:
                issues["Inpaint"] = 'scikit-image required to do inprinting'

        width = self.getProperty("Width").value
        if len(width) != 1 and len(width) != 3:
            issues["Width"] = 'Must provide 1 or 3 widths'

        sphereMin = self.getProperty("SphereMin").value
        if len(sphereMin) != 1 and len(sphereMin) != 3:
            issues["SphereMin"] = 'Must provide 1 or 3 widths'

        sphereMax = self.getProperty("SphereMax").value
        if len(sphereMax) != 1 and len(sphereMax) != 3:
            issues["SphereMax"] = 'Must provide 1 or 3 widths'

        return issues

    def PyExec(self):
        progress = Progress(self, 0.0, 1.0, 5)
        inWS = self.getProperty("InputWorkspace").value
        signal = inWS.getSignalArray().copy()

        dimX=inWS.getXDimension()
        dimY=inWS.getYDimension()
        dimZ=inWS.getZDimension()
        X=np.linspace(dimX.getMinimum(),dimX.getMaximum(),dimX.getNBins()+1)
        Y=np.linspace(dimY.getMinimum(),dimY.getMaximum(),dimY.getNBins()+1)
        Z=np.linspace(dimZ.getMinimum(),dimZ.getMaximum(),dimZ.getNBins()+1)
        Xs, Ys, Zs = np.mgrid[(X[0]+X[1])/2:(X[-1]+X[-2])/2:dimX.getNBins()*1j,
                              (Y[0]+Y[1])/2:(Y[-1]+Z[-2])/2:dimY.getNBins()*1j,
                              (Z[0]+Z[1])/2:(Y[-1]+Z[-2])/2:dimZ.getNBins()*1j]

        if self.getProperty("RemoveReflections").value:
            progress.report("Removing Reflections")
            width = self.getProperty("Width").value
            if len(width)==1:
                width = np.repeat(width, 3)
            cut_shape = self.getProperty("Shape").value
            space_group = self.getProperty("SpaceGroup").value
            if space_group:
                check_space_group = True
                sg=SpaceGroupFactory.createSpaceGroup(space_group)
            else:
                check_space_group = False
            for h in range(int(np.ceil(dimX.getMinimum())), int(np.floor(dimX.getMaximum()))+1):
                for k in range(int(np.ceil(dimY.getMinimum())), int(np.floor(dimY.getMaximum()))+1):
                    for l in range(int(np.ceil(dimZ.getMinimum())), int(np.floor(dimZ.getMaximum()))+1):
                        if not check_space_group or sg.isAllowedReflection([h,k,l]):
                            if cut_shape == 'cube':
                                x_min=np.searchsorted(X,h-width[0])
                                x_max=np.searchsorted(X,h+width[0])
                                y_min=np.searchsorted(Y,k-width[1])
                                y_max=np.searchsorted(Y,k+width[1])
                                z_min=np.searchsorted(Z,l-width[2])
                                z_max=np.searchsorted(Z,l+width[2])
                                signal[x_min:x_max,y_min:y_max,z_min:z_max]=np.nan
                            else:
                                signal[(Xs-h)**2/width[0]**2 + (Ys-k)**2/width[1]**2 + (Zs-l)**2/width[2]**2 < 1]=np.nan

        if self.getProperty("CutSphere").value:
            progress.report("Cutting to sphere")
            sphereMin = self.getProperty("SphereMin").value
            if sphereMin[0] < Property.EMPTY_DBL:
                if len(sphereMin)==1:
                    sphereMin = np.repeat(sphereMin, 3)
                signal[Xs**2/sphereMin[0]**2 + Ys**2/sphereMin[1]**2 + Zs**2/sphereMin[2]**2 < 1]=np.nan
            sphereMax = self.getProperty("SphereMax").value
            if sphereMax[0] < Property.EMPTY_DBL:
                if len(sphereMax)==1:
                    sphereMax = np.repeat(sphereMax, 3)
                signal[Xs**2/sphereMax[0]**2 + Ys**2/sphereMax[1]**2 + Zs**2/sphereMax[2]**2 > 1]=np.nan

        if self.getProperty("Convolution").value:
            progress.report("Convoluting signal")
            signal = self._convolution(signal)

        if self.getProperty("Inpaint").value:
            progress.report("Inpainting signal")
            signal = self._inpaint(signal)

        if self.getPropertyValue("IntermediateWorkspace"):
            cloneWS_alg = self.createChildAlgorithm("CloneMDWorkspace", enableLogging=False)
            cloneWS_alg.setProperty("InputWorkspace",inWS)
            cloneWS_alg.execute()
            signalOutWS = cloneWS_alg.getProperty("OutputWorkspace").value
            signalOutWS.setSignalArray(signal)
            self.setProperty("IntermediateWorkspace", signalOutWS)

        # Do FFT
        progress.report("Running FFT")
        # Replace any remaining nan's or inf's with 0
        # Otherwise you end up with a lot of nan's
        signal[np.isnan(signal)]=0
        signal[np.isinf(signal)]=0

        signal=np.fft.fftshift(np.fft.fftn(np.fft.fftshift(signal))).real

        # Calculate new extents for fft space
        extents=''
        for d in range(inWS.getNumDims()):
            dim = inWS.getDimension(d)
            fft_dim=np.fft.fftshift(np.fft.fftfreq(dim.getNBins(), (dim.getMaximum()-dim.getMinimum())/dim.getNBins()))
            extents+=str(fft_dim[0])+','+str(fft_dim[-1])+','
        extents=extents[:-1]

        createWS_alg = self.createChildAlgorithm("CreateMDHistoWorkspace", enableLogging=False)
        createWS_alg.setProperty("SignalInput",signal)
        createWS_alg.setProperty("ErrorInput",signal**2)
        createWS_alg.setProperty("Dimensionality",3)
        createWS_alg.setProperty("Extents",extents)
        createWS_alg.setProperty("NumberOfBins",signal.shape)
        createWS_alg.setProperty("Names",'x,y,z')
        createWS_alg.setProperty("Units",'A,A,A')
        createWS_alg.execute()
        outWS = createWS_alg.getProperty("OutputWorkspace").value

        progress.report()

        self.setProperty("OutputWorkspace", outWS)

    def _convolution(self, signal):
        from astropy.convolution import convolve, convolve_fft, Gaussian1DKernel
        G1D = Gaussian1DKernel(2).array
        G3D = G1D * G1D.reshape((-1,1)) * G1D.reshape((-1,1,1))
        try:
            logger.debug('Trying astropy.convolution.convolve_fft for convolution')
            return convolve_fft(signal, G3D)  # Faster but will fail with large signal and kernel arrays
        except ValueError:
            logger.debug('Using astropy.convolution.convolve for convolution')
            return convolve(signal, G3D)

    def _inpaint(self, signal):
        from skimage.restoration import inpaint
        return inpaint.inpaint_biharmonic(signal, np.isnan(signal))


AlgorithmFactory.subscribe(DeltaPDF3D)
