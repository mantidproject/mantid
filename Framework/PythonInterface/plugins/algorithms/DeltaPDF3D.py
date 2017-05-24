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
                             "Input Workspace with HKL dimensions centered on zero.")
        self.declareProperty(WorkspaceProperty("IntermediateWorkspace", "",
                                               optional=PropertyMode.Optional,
                                               direction=Direction.Output),
                             "The resulting workspace after reflection removal and filters applied. What is the input of the FFT.")
        self.declareProperty(WorkspaceProperty("OutputWorkspace", "",
                                               optional=PropertyMode.Mandatory,
                                               direction=Direction.Output),
                             "Output Workspace")

        self.declareProperty("RemoveReflections", True, "Remove HKL reflections")
        condition = EnabledWhenProperty("RemoveReflections", PropertyCriterion.IsDefault)
        self.declareProperty("Shape", "sphere", doc="Shape to cut out reflections",
                             validator=StringListValidator(['sphere', 'cube']))
        self.setPropertySettings("Shape", condition)
        val_min_zero = FloatArrayBoundedValidator()
        val_min_zero.setLower(0.)
        self.declareProperty(FloatArrayProperty("Size", [0.2], validator=val_min_zero),
                             "Width of cube/diameter of sphere used to remove reflections, in (HKL) (one or three values)")
        self.setPropertySettings("Size", condition)
        self.declareProperty("SpaceGroup", "",
                             doc="Space group for reflection removal, either full name or number. If empty all HKL's will be removed.")
        self.setPropertySettings("SpaceGroup", condition)

        self.declareProperty("CropSphere", False, "Limit min/max q values. Can help with edge effects.")
        condition = EnabledWhenProperty("CropSphere", PropertyCriterion.IsNotDefault)
        self.declareProperty(FloatArrayProperty("SphereMin", [Property.EMPTY_DBL], validator=val_min_zero),
                             "HKL values below which will be removed (one or three values)")
        self.setPropertySettings("SphereMin", condition)
        self.declareProperty(FloatArrayProperty("SphereMax", [Property.EMPTY_DBL], validator=val_min_zero),
                             "HKL values above which will be removed (one or three values)")
        self.setPropertySettings("SphereMax", condition)
        self.declareProperty("FillValue", Property.EMPTY_DBL, "Value to replace with outside sphere")
        self.setPropertySettings("FillValue", condition)

        self.declareProperty("Convolution", True, "Apply convolution to fill in removed reflections")
        condition = EnabledWhenProperty("Convolution", PropertyCriterion.IsDefault)
        self.declareProperty("ConvolutionWidth", 2.0, validator=FloatBoundedValidator(0.),
                             doc="Width of gaussian convolution in pixels")
        self.setPropertySettings("ConvolutionWidth", condition)
        self.declareProperty("Deconvolution", False, "Apply deconvolution after fourier transform")
        self.setPropertySettings("Deconvolution", condition)

        # Reflections
        self.setPropertyGroup("RemoveReflections","Reflection Removal")
        self.setPropertyGroup("Shape","Reflection Removal")
        self.setPropertyGroup("Size","Reflection Removal")
        self.setPropertyGroup("SpaceGroup","Reflection Removal")

        # Sphere
        self.setPropertyGroup("CropSphere","Cropping to a sphere")
        self.setPropertyGroup("SphereMin","Cropping to a sphere")
        self.setPropertyGroup("SphereMax","Cropping to a sphere")
        self.setPropertyGroup("FillValue","Cropping to a sphere")

        # Convolution
        self.setPropertyGroup("Convolution","Convolution")
        self.setPropertyGroup("ConvolutionWidth","Convolution")
        self.setPropertyGroup("Deconvolution","Convolution")

    def validateInputs(self):
        issues = dict()

        inWS = self.getProperty("InputWorkspace").value
        dimX=inWS.getXDimension()
        dimY=inWS.getYDimension()
        dimZ=inWS.getZDimension()

        if dimX.name != '[H,0,0]' or dimY.name != '[0,K,0]' or dimZ.name != '[0,0,L]':
            issues['InputWorkspace'] = 'dimensions must be [H,0,0], [0,K,0] and [0,0,L]'

        for d in range(inWS.getNumDims()):
            dim = inWS.getDimension(d)
            if not np.isclose(dim.getMaximum(), -dim.getMinimum()):
                issues['InputWorkspace'] = 'dimensions must be centered on zero'

        if self.getProperty("Convolution").value:
            try:
                import astropy # noqa
            except ImportError:
                issues["Convolution"] = 'python-astropy required to do convolution'

        size = self.getProperty("Size").value
        if len(size) != 1 and len(size) != 3:
            issues["Size"] = 'Must provide 1 or 3 sizes'

        if self.getProperty("SpaceGroup").value:
            space_group=self.getProperty("SpaceGroup").value
            try:
                if not SpaceGroupFactory.isSubscribedNumber(int(space_group)):
                    issues["SpaceGroup"] = 'Space group number is not valid'
            except ValueError:
                if not SpaceGroupFactory.isSubscribedSymbol(space_group):
                    issues["SpaceGroup"] = 'Space group name is not valid'

        sphereMin = self.getProperty("SphereMin").value
        if len(sphereMin) != 1 and len(sphereMin) != 3:
            issues["SphereMin"] = 'Must provide 1 or 3 diameters'

        sphereMax = self.getProperty("SphereMax").value
        if len(sphereMax) != 1 and len(sphereMax) != 3:
            issues["SphereMax"] = 'Must provide 1 or 3 diameters'

        return issues

    def PyExec(self): # noqa
        progress = Progress(self, 0.0, 1.0, 5)
        inWS = self.getProperty("InputWorkspace").value
        signal = inWS.getSignalArray().copy()

        dimX=inWS.getXDimension()
        dimY=inWS.getYDimension()
        dimZ=inWS.getZDimension()

        Xmin=dimX.getMinimum()
        Ymin=dimY.getMinimum()
        Zmin=dimZ.getMinimum()
        Xmax=dimX.getMaximum()
        Ymax=dimY.getMaximum()
        Zmax=dimZ.getMaximum()
        Xbins=dimX.getNBins()
        Ybins=dimY.getNBins()
        Zbins=dimZ.getNBins()
        Xwidth=dimX.getBinWidth()
        Ywidth=dimY.getBinWidth()
        Zwidth=dimZ.getBinWidth()

        X=np.linspace(Xmin,Xmax,Xbins+1)
        Y=np.linspace(Ymin,Ymax,Ybins+1)
        Z=np.linspace(Zmin,Zmax,Zbins+1)

        X, Y, Z = np.ogrid[(dimX.getX(0)+dimX.getX(1))/2:(dimX.getX(Xbins)+dimX.getX(Xbins-1))/2:Xbins*1j,
                           (dimY.getX(0)+dimY.getX(1))/2:(dimY.getX(Ybins)+dimY.getX(Ybins-1))/2:Ybins*1j,
                           (dimZ.getX(0)+dimZ.getX(1))/2:(dimZ.getX(Zbins)+dimZ.getX(Zbins-1))/2:Zbins*1j]

        if self.getProperty("RemoveReflections").value:
            progress.report("Removing Reflections")
            size = self.getProperty("Size").value
            if len(size)==1:
                size = np.repeat(size, 3)
            size/=2.0 # We want radii or half box width
            cut_shape = self.getProperty("Shape").value
            space_group = self.getProperty("SpaceGroup").value
            if space_group:
                check_space_group = True
                try:
                    space_group=SpaceGroupFactory.subscribedSpaceGroupSymbols(int(space_group))[0]
                except ValueError:
                    pass
                logger.information('Using space group: '+space_group)
                sg=SpaceGroupFactory.createSpaceGroup(space_group)
            else:
                check_space_group = False

            if cut_shape == 'cube':
                for h in range(int(np.ceil(Xmin)), int(Xmax)+1):
                    for k in range(int(np.ceil(Ymin)), int(Ymax)+1):
                        for l in range(int(np.ceil(Zmin)), int(Zmax)+1):
                            if not check_space_group or sg.isAllowedReflection([h,k,l]):
                                signal[int((h-size[0]-Xmin)/Xwidth+1):int((h+size[0]-Xmin)/Xwidth),
                                       int((k-size[1]-Ymin)/Ywidth+1):int((k+size[1]-Ymin)/Ywidth),
                                       int((l-size[2]-Zmin)/Zwidth+1):int((l+size[2]-Zmin)/Zwidth)]=np.nan
            else:  # sphere
                mask=((X-np.round(X))**2/size[0]**2 + (Y-np.round(Y))**2/size[1]**2 + (Z-np.round(Z))**2/size[2]**2 < 1)

                # Unmask invalid reflections
                if check_space_group:
                    for h in range(int(np.ceil(Xmin)), int(Xmax)+1):
                        for k in range(int(np.ceil(Ymin)), int(Ymax)+1):
                            for l in range(int(np.ceil(Zmin)), int(Zmax)+1):
                                if not sg.isAllowedReflection([h,k,l]):
                                    mask[int((h-0.5-Xmin)/Xwidth+1):int((h+0.5-Xmin)/Xwidth),
                                         int((k-0.5-Ymin)/Ywidth+1):int((k+0.5-Ymin)/Ywidth),
                                         int((l-0.5-Zmin)/Zwidth+1):int((l+0.5-Zmin)/Zwidth)]=False

                signal[mask]=np.nan

        if self.getProperty("CropSphere").value:
            progress.report("Cropping to sphere")
            sphereMin = self.getProperty("SphereMin").value

            if sphereMin[0] < Property.EMPTY_DBL:
                if len(sphereMin)==1:
                    sphereMin = np.repeat(sphereMin, 3)
                signal[X**2/sphereMin[0]**2 + Y**2/sphereMin[1]**2 + Z**2/sphereMin[2]**2 < 1]=np.nan

            sphereMax = self.getProperty("SphereMax").value

            if sphereMax[0] < Property.EMPTY_DBL:
                if len(sphereMax)==1:
                    sphereMax = np.repeat(sphereMax, 3)
                if self.getProperty("FillValue").value == Property.EMPTY_DBL:
                    fill_value = np.nan
                else:
                    fill_value = self.getProperty("FillValue").value
                signal[X**2/sphereMax[0]**2 + Y**2/sphereMax[1]**2 + Z**2/sphereMax[2]**2 > 1]=fill_value

        if self.getProperty("Convolution").value:
            progress.report("Convoluting signal")
            signal = self._convolution(signal)

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

        signal=np.fft.fftshift(np.fft.fftn(np.fft.ifftshift(signal)))
        number_of_bins = signal.shape

        # Do deconvolution
        if self.getProperty("Convolution").value and self.getProperty("Deconvolution").value:
            signal /= self._deconvolution(np.array(signal.shape))

        # CreateMDHistoWorkspace expects Fortan `column-major` ordering
        signal = signal.real.flatten('F')

        createWS_alg = self.createChildAlgorithm("CreateMDHistoWorkspace", enableLogging=False)
        createWS_alg.setProperty("SignalInput", signal)
        createWS_alg.setProperty("ErrorInput", signal**2)
        createWS_alg.setProperty("Dimensionality", 3)
        createWS_alg.setProperty("Extents", self._calc_new_extents(inWS))
        createWS_alg.setProperty("NumberOfBins", number_of_bins)
        createWS_alg.setProperty("Names", 'x,y,z')
        createWS_alg.setProperty("Units", 'a,b,c')
        createWS_alg.execute()
        outWS = createWS_alg.getProperty("OutputWorkspace").value

        # Copy first experiment info
        if inWS.getNumExperimentInfo() > 0:
            outWS.copyExperimentInfos(inWS)

        progress.report()

        self.setProperty("OutputWorkspace", outWS)

    def _convolution(self, signal):
        from astropy.convolution import convolve, convolve_fft, Gaussian1DKernel
        G1D = Gaussian1DKernel(self.getProperty("ConvolutionWidth").value).array
        G3D = G1D * G1D.reshape((-1,1)) * G1D.reshape((-1,1,1))
        try:
            logger.debug('Trying astropy.convolution.convolve_fft for convolution')
            return convolve_fft(signal, G3D)  # Faster but will fail with large signal and kernel arrays
        except ValueError:
            logger.debug('Using astropy.convolution.convolve for convolution')
            return convolve(signal, G3D)

    def _deconvolution(self, shape):
        from astropy.convolution import Gaussian1DKernel
        G1D = Gaussian1DKernel(self.getProperty("ConvolutionWidth").value).array
        G3D = G1D * G1D.reshape((-1,1)) * G1D.reshape((-1,1,1))
        G3D_shape = np.array(G3D.shape)
        G3D = np.pad(G3D,pad_width=np.array([np.maximum(np.floor((shape-G3D_shape)/2),np.zeros(len(shape))),
                                             np.maximum(np.ceil((shape-G3D_shape)/2),np.zeros(len(shape)))],
                                            dtype=np.int).transpose(),mode='constant')
        deconv = np.fft.fftshift(np.fft.fftn(np.fft.ifftshift(G3D)))
        iarr = (deconv.shape-shape)//2
        return deconv[iarr[0]:shape[0]+iarr[0],iarr[1]:shape[1]+iarr[1],iarr[2]:shape[2]+iarr[2]]

    def _calc_new_extents(self, inWS):
        # Calculate new extents for fft space
        extents=''
        for d in range(inWS.getNumDims()):
            dim = inWS.getDimension(d)
            if dim.getNBins() == 1:
                fft_dim = 1./(dim.getMaximum()-dim.getMinimum())
                extents+=str(-fft_dim/2.)+','+str(fft_dim/2.)+','
            else:
                fft_dim=np.fft.fftshift(np.fft.fftfreq(dim.getNBins(), (dim.getMaximum()-dim.getMinimum())/dim.getNBins()))
                extents+=str(fft_dim[0])+','+str(fft_dim[-1])+','
        return extents[:-1]


AlgorithmFactory.subscribe(DeltaPDF3D)
