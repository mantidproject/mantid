from __future__ import (absolute_import, division, print_function)
from mantid.api import PythonAlgorithm, AlgorithmFactory, IMDHistoWorkspaceProperty, PropertyMode, WorkspaceProperty, Progress
from mantid.kernel import Direction, EnabledWhenProperty, PropertyCriterion
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
        self.declareProperty(WorkspaceProperty("OutputWorkspace", "",
                                               optional=PropertyMode.Mandatory,
                                               direction=Direction.Output),
                             "Output Workspace")
        self.declareProperty("RemoveReflections", True, "Remove reflections")
        condition = EnabledWhenProperty("RemoveReflections", PropertyCriterion.IsDefault)
        self.declareProperty("BoxWidth", 0.1, "Width of box to remove reflections, in (HKL)")
        self.setPropertySettings("BoxWidth", condition)
        self.declareProperty("SpaceGroup", "", doc="Space groups, for reflection removal")
        self.setPropertySettings("SpaceGroup", condition)
        self.declareProperty("Convolution", True, "Apply convolution to fill in removed reflections")
        condition = EnabledWhenProperty("Convolution", PropertyCriterion.IsDefault)
        self.declareProperty("ConvolutionWidth", 2.0, "Width of gaussian convolution in pixels")
        self.setPropertySettings("ConvolutionWidth", condition)
        self.declareProperty("FFT", True, "Calculate 3D-deltaPDF")

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
        return issues

    def PyExec(self):
        progress = Progress(self, 0.0, 1.0, 4)
        inWS = self.getProperty("InputWorkspace").value
        signal = inWS.getSignalArray().copy()

        if self.getProperty("RemoveReflections").value:
            progress.report("Removing Reflections")
            box_width = self.getProperty("BoxWidth").value
            dimX=inWS.getXDimension()
            dimY=inWS.getYDimension()
            dimZ=inWS.getZDimension()
            X=np.linspace(dimX.getMinimum(),dimX.getMaximum(),dimX.getNBins()+1)
            Y=np.linspace(dimY.getMinimum(),dimY.getMaximum(),dimY.getNBins()+1)
            Z=np.linspace(dimZ.getMinimum(),dimZ.getMaximum(),dimZ.getNBins()+1)
            space_group = self.getProperty("SpaceGroup").value
            if space_group is '':
                check_space_group = False
            else:
                check_space_group = True
                sg=SpaceGroupFactory.createSpaceGroup(space_group)
            for h in range(int(np.ceil(dimX.getMinimum())), int(np.floor(dimX.getMaximum()))+1):
                for k in range(int(np.ceil(dimY.getMinimum())), int(np.floor(dimY.getMaximum()))+1):
                    for l in range(int(np.ceil(dimZ.getMinimum())), int(np.floor(dimZ.getMaximum()))+1):
                        if not check_space_group or sg.isAllowedReflection([h,k,l]):
                            x_min=np.searchsorted(X,h-box_width)
                            x_max=np.searchsorted(X,h+box_width)
                            y_min=np.searchsorted(Y,k-box_width)
                            y_max=np.searchsorted(Y,k+box_width)
                            z_min=np.searchsorted(Z,l-box_width)
                            z_max=np.searchsorted(Z,l+box_width)
                            signal[x_min:x_max,y_min:y_max,z_min:z_max]=np.nan

        if self.getProperty("Convolution").value:
            progress.report("Convoluting signal")
            G1D = Gaussian1DKernel(2).array
            G3D = G1D * G1D.reshape((-1,1)) * G1D.reshape((-1,1,1))
            try:
                logger.debug('Trying astropy.convolution.convolve_fft for convolution')
                signal = convolve_fft(signal, G3D)  # Faster but will fail with large signal and kernel arrays
            except ValueError:
                logger.debug('Using astropy.convolution.convolve for convolution')
                signal = convolve(signal, G3D)

        if self.getProperty("FFT").value:
            progress.report("Running FFT")
            # Replace any remaining nan's or inf's with 0
            # Otherwise you end up with a lot of nan's
            signal[np.isnan(signal)]=0
            signal[np.isinf(signal)]=0

            fft=np.fft.fftshift(np.fft.fftn(np.fft.fftshift(signal)))
            signal=(fft*np.conj(fft)).real

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
        else:
            cloneWS_alg = self.createChildAlgorithm("CloneMDWorkspace", enableLogging=False)
            cloneWS_alg.setProperty("InputWorkspace",inWS)
            cloneWS_alg.execute()
            outWS = cloneWS_alg.getProperty("OutputWorkspace").value
            outWS.setSignalArray(signal)

        progress.report()

        self.setProperty("OutputWorkspace", outWS)


try:
    from astropy.convolution import convolve, convolve_fft, Gaussian1DKernel
    AlgorithmFactory.subscribe(DeltaPDF3D)
except ImportError:
    logger.debug('Failed to subscribe algorithm DeltaPDF3D; cannot import astropy')
