# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# ruff: noqa: E741  # Ambiguous variable name
from mantid.api import PythonAlgorithm, AlgorithmFactory, IMDHistoWorkspaceProperty, PropertyMode, WorkspaceProperty, Progress
from mantid.kernel import (
    Direction,
    EnabledWhenProperty,
    PropertyCriterion,
    Property,
    StringListValidator,
    FloatArrayBoundedValidator,
    FloatArrayProperty,
    FloatBoundedValidator,
)
from mantid.geometry import SpaceGroupFactory
from mantid import logger
import numpy as np
from scipy import ndimage


class DeltaPDF3D(PythonAlgorithm):
    def category(self):
        return "Diffraction\\Utility"

    def name(self):
        return "DeltaPDF3D"

    def summary(self):
        return "Calculates the 3D-deltaPDF from a HKL workspace"

    def PyInit(self):
        self.declareProperty(
            IMDHistoWorkspaceProperty("InputWorkspace", "", optional=PropertyMode.Mandatory, direction=Direction.Input),
            "Input Workspace with HKL dimensions centered on zero.",
        )
        self.declareProperty(
            WorkspaceProperty("IntermediateWorkspace", "", optional=PropertyMode.Optional, direction=Direction.Output),
            "The resulting workspace after reflection removal and filters applied. What is the input of the FFT.",
        )
        self.declareProperty(
            WorkspaceProperty("OutputWorkspace", "", optional=PropertyMode.Mandatory, direction=Direction.Output), "Output Workspace"
        )

        self.declareProperty("Method", "KAREN", StringListValidator(["None", "Punch and fill", "KAREN"]), "Bragg peak removal method")
        self.declareProperty(
            "WindowFunction",
            "Blackman",
            StringListValidator(["None", "Gaussian", "Blackman", "Tukey", "Kaiser"]),
            "Apply a window function to the data",
        )
        self.declareProperty(
            "WindowParameter",
            defaultValue=0.5,
            validator=FloatBoundedValidator(0.0),
            doc="Parameter for window function, depends on window type, see algorithm docs",
        )

        # Punch and fill
        condition = EnabledWhenProperty("Method", PropertyCriterion.IsEqualTo, "Punch and fill")
        self.declareProperty("Shape", "sphere", doc="Shape to punch out reflections", validator=StringListValidator(["sphere", "cube"]))
        self.setPropertySettings("Shape", condition)
        val_min_zero = FloatArrayBoundedValidator(lower=0.0)
        self.declareProperty(
            FloatArrayProperty("Size", [0.2], validator=val_min_zero),
            "Width of cube/diameter of sphere used to remove reflections, in (HKL) (one or three values)",
        )
        self.setPropertySettings("Size", condition)
        self.declareProperty(
            "SpaceGroup", "", doc="Space group for reflection removal, either full name or number. If empty all HKL's will be removed."
        )
        self.setPropertySettings("SpaceGroup", condition)
        self.declareProperty("Convolution", True, "Apply convolution to fill in removed reflections")
        self.setPropertySettings("Convolution", condition)
        self.declareProperty("ConvolutionWidth", 2.0, validator=FloatBoundedValidator(0.0), doc="Width of gaussian convolution in pixels")
        self.setPropertySettings("ConvolutionWidth", condition)

        self.declareProperty("CropSphere", False, "Limit min/max q values. Can help with edge effects.")
        condition = EnabledWhenProperty("CropSphere", PropertyCriterion.IsNotDefault)
        self.declareProperty(
            FloatArrayProperty("SphereMin", [Property.EMPTY_DBL], validator=val_min_zero),
            "HKL values below which will be removed (one or three values)",
        )
        self.setPropertySettings("SphereMin", condition)
        self.declareProperty(
            FloatArrayProperty("SphereMax", [Property.EMPTY_DBL], validator=val_min_zero),
            "HKL values above which will be removed (one or three values)",
        )
        self.setPropertySettings("SphereMax", condition)
        self.declareProperty("FillValue", Property.EMPTY_DBL, "Value to replace with outside sphere")
        self.setPropertySettings("FillValue", condition)

        # KAREN
        self.declareProperty("KARENWidth", 7, "Size of filter window")

        # Reflections
        self.setPropertyGroup("Shape", "Punch and fill")
        self.setPropertyGroup("Size", "Punch and fill")
        self.setPropertyGroup("SpaceGroup", "Punch and fill")

        # Sphere
        self.setPropertyGroup("CropSphere", "Cropping to a sphere")
        self.setPropertyGroup("SphereMin", "Cropping to a sphere")
        self.setPropertyGroup("SphereMax", "Cropping to a sphere")
        self.setPropertyGroup("FillValue", "Cropping to a sphere")

        # Convolution
        self.setPropertyGroup("Convolution", "Convolution")
        self.setPropertyGroup("ConvolutionWidth", "Convolution")

    def validateInputs(self):
        issues = dict()

        inWS = self.getProperty("InputWorkspace").value
        dimX = inWS.getXDimension()
        dimY = inWS.getYDimension()
        dimZ = inWS.getZDimension()

        if dimX.name != "[H,0,0]" or dimY.name != "[0,K,0]" or dimZ.name != "[0,0,L]":
            issues["InputWorkspace"] = "dimensions must be [H,0,0], [0,K,0] and [0,0,L]"

        for d in range(inWS.getNumDims()):
            dim = inWS.getDimension(d)
            if not np.isclose(dim.getMaximum(), -dim.getMinimum(), atol=1e-5):
                issues["InputWorkspace"] = "dimensions must be centered on zero"

        if self.getProperty("Convolution").value and self.getProperty("Method").value == "Punch and fill":
            try:
                import astropy  # noqa
            except ImportError:
                issues["Convolution"] = "python-astropy required to do convolution"

        size = self.getProperty("Size").value
        if len(size) != 1 and len(size) != 3:
            issues["Size"] = "Must provide 1 or 3 sizes"

        if self.getProperty("SpaceGroup").value:
            space_group = self.getProperty("SpaceGroup").value
            try:
                if not SpaceGroupFactory.isSubscribedNumber(int(space_group)):
                    issues["SpaceGroup"] = "Space group number is not valid"
            except ValueError:
                if not SpaceGroupFactory.isSubscribedSymbol(space_group):
                    issues["SpaceGroup"] = "Space group name is not valid"

        sphereMin = self.getProperty("SphereMin").value
        if len(sphereMin) != 1 and len(sphereMin) != 3:
            issues["SphereMin"] = "Must provide 1 or 3 diameters"

        sphereMax = self.getProperty("SphereMax").value
        if len(sphereMax) != 1 and len(sphereMax) != 3:
            issues["SphereMax"] = "Must provide 1 or 3 diameters"

        return issues

    def PyExec(self):
        progress = Progress(self, 0.0, 1.0, 5)
        inWS = self.getProperty("InputWorkspace").value
        signal = inWS.getSignalArray().copy()

        if self.getProperty("CropSphere").value:
            signal = self._crop_sphere(signal, inWS.getXDimension(), inWS.getYDimension(), inWS.getZDimension())

        window_function = self.getProperty("WindowFunction").value
        if window_function != "None":
            paramater = self.getProperty("WindowParameter").value
            _, _, Xbins, _ = self._get_dim_params(inWS.getXDimension())
            _, _, Ybins, _ = self._get_dim_params(inWS.getYDimension())
            _, _, Zbins, _ = self._get_dim_params(inWS.getZDimension())
            if window_function == "Gaussian":
                progress.report("Applying Gaussian window")
                window = self._gaussian_window((Xbins, Ybins, Zbins), paramater)
            elif window_function == "Blackman":
                progress.report("Applying Blackman window")
                window = self._blackman_window((Xbins, Ybins, Zbins))
            elif window_function == "Tukey":
                progress.report("Applying Tukey window")
                window = self._tukey_window((Xbins, Ybins, Zbins), paramater)
            elif window_function == "Kaiser":
                progress.report("Applying Kaiser window")
                window = self._kaiser_window((Xbins, Ybins, Zbins), paramater)
            signal = np.multiply(signal, window)

        if self.getProperty("Method").value == "Punch and fill":
            progress.report("Removing Reflections")
            signal = self._punch_and_fill(signal, inWS.getXDimension(), inWS.getYDimension(), inWS.getZDimension())

            if self.getProperty("Convolution").value:
                progress.report("Convoluting signal")
                signal = self._convolution(signal)

        elif self.getProperty("Method").value == "KAREN":
            progress.report("Running KAREN")
            signal = self._karen(signal, self.getProperty("KARENWidth").value)

        if self.getPropertyValue("IntermediateWorkspace"):
            cloneWS_alg = self.createChildAlgorithm("CloneMDWorkspace", enableLogging=False)
            cloneWS_alg.setProperty("InputWorkspace", inWS)
            cloneWS_alg.execute()
            signalOutWS = cloneWS_alg.getProperty("OutputWorkspace").value
            signalOutWS.setSignalArray(signal)
            self.setProperty("IntermediateWorkspace", signalOutWS)

        # Do FFT
        progress.report("Running FFT")
        # Replace any remaining nan's or inf's with 0
        # Otherwise you end up with a lot of nan's
        signal[np.isnan(signal)] = 0
        signal[np.isinf(signal)] = 0

        signal = np.fft.fftshift(np.fft.fftn(np.fft.ifftshift(signal)))
        number_of_bins = signal.shape

        # CreateMDHistoWorkspace expects Fortan `column-major` ordering
        signal = signal.real.flatten("F")

        createWS_alg = self.createChildAlgorithm("CreateMDHistoWorkspace", enableLogging=False)
        createWS_alg.setProperty("SignalInput", signal)
        createWS_alg.setProperty("ErrorInput", signal**2)
        createWS_alg.setProperty("Dimensionality", 3)
        createWS_alg.setProperty("Extents", self._calc_new_extents(inWS))
        createWS_alg.setProperty("NumberOfBins", number_of_bins)
        createWS_alg.setProperty("Names", "x,y,z")
        createWS_alg.setProperty("Units", "a,b,c")
        createWS_alg.execute()
        outWS = createWS_alg.getProperty("OutputWorkspace").value

        # Copy first experiment info
        if inWS.getNumExperimentInfo() > 0:
            outWS.copyExperimentInfos(inWS)

        progress.report()

        self.setProperty("OutputWorkspace", outWS)

    def _punch_and_fill(self, signal, dimX, dimY, dimZ):
        Xmin, Xmax, _, Xwidth = self._get_dim_params(dimX)
        Ymin, Ymax, _, Ywidth = self._get_dim_params(dimY)
        Zmin, Zmax, _, Zwidth = self._get_dim_params(dimZ)
        X, Y, Z = self._get_XYZ_ogrid(dimX, dimY, dimZ)

        size = self.getProperty("Size").value
        if len(size) == 1:
            size = np.repeat(size, 3)
        size /= 2.0  # We want radii or half box width
        cut_shape = self.getProperty("Shape").value
        space_group = self.getProperty("SpaceGroup").value
        if space_group:
            check_space_group = True
            try:
                space_group = SpaceGroupFactory.subscribedSpaceGroupSymbols(int(space_group))[0]
            except ValueError:
                pass
            logger.information("Using space group: " + space_group)
            sg = SpaceGroupFactory.createSpaceGroup(space_group)
        else:
            check_space_group = False

        if cut_shape == "cube":
            for h in range(int(np.ceil(Xmin)), int(Xmax) + 1):
                for k in range(int(np.ceil(Ymin)), int(Ymax) + 1):
                    for l in range(int(np.ceil(Zmin)), int(Zmax) + 1):
                        if not check_space_group or sg.isAllowedReflection([h, k, l]):
                            signal[
                                int((h - size[0] - Xmin) / Xwidth + 1) : int((h + size[0] - Xmin) / Xwidth),
                                int((k - size[1] - Ymin) / Ywidth + 1) : int((k + size[1] - Ymin) / Ywidth),
                                int((l - size[2] - Zmin) / Zwidth + 1) : int((l + size[2] - Zmin) / Zwidth),
                            ] = np.nan
        else:  # sphere
            mask = (X - np.round(X)) ** 2 / size[0] ** 2 + (Y - np.round(Y)) ** 2 / size[1] ** 2 + (Z - np.round(Z)) ** 2 / size[2] ** 2 < 1

            # Unmask invalid reflections
            if check_space_group:
                for h in range(int(np.ceil(Xmin)), int(Xmax) + 1):
                    for k in range(int(np.ceil(Ymin)), int(Ymax) + 1):
                        for l in range(int(np.ceil(Zmin)), int(Zmax) + 1):
                            if not sg.isAllowedReflection([h, k, l]):
                                mask[
                                    int((h - 0.5 - Xmin) / Xwidth + 1) : int((h + 0.5 - Xmin) / Xwidth),
                                    int((k - 0.5 - Ymin) / Ywidth + 1) : int((k + 0.5 - Ymin) / Ywidth),
                                    int((l - 0.5 - Zmin) / Zwidth + 1) : int((l + 0.5 - Zmin) / Zwidth),
                                ] = False

            signal[mask] = np.nan

        return signal

    def _crop_sphere(self, signal, dimX, dimY, dimZ):
        X, Y, Z = self._get_XYZ_ogrid(dimX, dimY, dimZ)

        sphereMin = self.getProperty("SphereMin").value

        if sphereMin[0] < Property.EMPTY_DBL:
            if len(sphereMin) == 1:
                sphereMin = np.repeat(sphereMin, 3)
            signal[X**2 / sphereMin[0] ** 2 + Y**2 / sphereMin[1] ** 2 + Z**2 / sphereMin[2] ** 2 < 1] = np.nan

        sphereMax = self.getProperty("SphereMax").value

        if sphereMax[0] < Property.EMPTY_DBL:
            if len(sphereMax) == 1:
                sphereMax = np.repeat(sphereMax, 3)
            if self.getProperty("FillValue").value == Property.EMPTY_DBL:
                fill_value = np.nan
            else:
                fill_value = self.getProperty("FillValue").value
            signal[X**2 / sphereMax[0] ** 2 + Y**2 / sphereMax[1] ** 2 + Z**2 / sphereMax[2] ** 2 > 1] = fill_value

        return signal

    def _get_XYZ_ogrid(self, dimX, dimY, dimZ):
        """
        Returns X, Y and Z as ogrid
        """
        Xmin, Xmax, Xbins, _ = self._get_dim_params(dimX)
        Ymin, Ymax, Ybins, _ = self._get_dim_params(dimY)
        Zmin, Zmax, Zbins, _ = self._get_dim_params(dimZ)

        return np.ogrid[
            (dimX.getX(0) + dimX.getX(1)) / 2 : (dimX.getX(Xbins) + dimX.getX(Xbins - 1)) / 2 : Xbins * 1j,
            (dimY.getX(0) + dimY.getX(1)) / 2 : (dimY.getX(Ybins) + dimY.getX(Ybins - 1)) / 2 : Ybins * 1j,
            (dimZ.getX(0) + dimZ.getX(1)) / 2 : (dimZ.getX(Zbins) + dimZ.getX(Zbins - 1)) / 2 : Zbins * 1j,
        ]

    def _get_dim_params(self, dim):
        """
        Return the min, max, number_of_bins and bin_width of dim
        """
        return dim.getMinimum(), dim.getMaximum(), dim.getNBins(), dim.getBinWidth()

    def _convolution(self, signal):
        from astropy.convolution import convolve, convolve_fft, Gaussian1DKernel

        G1D = Gaussian1DKernel(self.getProperty("ConvolutionWidth").value).array
        G3D = G1D * G1D.reshape((-1, 1)) * G1D.reshape((-1, 1, 1))
        try:
            logger.debug("Trying astropy.convolution.convolve_fft for convolution")
            return convolve_fft(signal, G3D)  # Faster but will fail with large signal and kernel arrays
        except ValueError:
            logger.debug("Using astropy.convolution.convolve for convolution")
            return convolve(signal, G3D)

    def _calc_new_extents(self, inWS):
        # Calculate new extents for fft space
        extents = ""
        for d in range(inWS.getNumDims()):
            dim = inWS.getDimension(d)
            if dim.getNBins() == 1:
                fft_dim = 1.0 / (dim.getMaximum() - dim.getMinimum())
                extents += str(-fft_dim / 2.0) + "," + str(fft_dim / 2.0) + ","
            else:
                fft_dim = np.fft.fftshift(np.fft.fftfreq(dim.getNBins(), (dim.getMaximum() - dim.getMinimum()) / dim.getNBins()))
                extents += str(fft_dim[0]) + "," + str(fft_dim[-1]) + ","
        return extents[:-1]

    def _karen(self, signal, width):
        """
        Bragg peaks are located as outliers in some moving window
        Outliers are defined as values more than 3sigma away from the median
        Sigma is estimated using 1.4826*MAD
        Returns median+2.2*MAD of window for values detected to be outliers
        Input dataset (dset) and window width (x)
        Input an odd window or the window will be asymmetric and stuff breaks
        """
        med = ndimage.filters.median_filter(signal, size=width, mode="nearest")  # Get median of input data set
        mad = ndimage.filters.median_filter(np.abs(signal - med), size=width, mode="nearest")  # Get median absolute deviation (MAD)
        asigma = np.abs(mad * 3 * 1.4826)  # Absolute value of approximate sigma
        mask = np.logical_or(signal < (med - asigma), signal > (med + asigma))  # Check if value is outlier based on MAD
        signal[mask] = (med + 2.2 * mad)[mask]  # Return median+2.2*MAD if value is outlier
        return signal

    def _gaussian_window(self, width, sigma):
        """
        Generates a gaussian window

        sigma is based on the dat being in a range 0 to 1
        """
        from scipy.signal.windows import gaussian

        return (
            gaussian(width[0], sigma * width[0]).reshape((-1, 1, 1))
            * gaussian(width[1], sigma * width[1]).reshape((-1, 1))
            * gaussian(width[2], sigma * width[2])
        )

    def _blackman_window(self, width):
        """
        Generates a blackman window
        """
        return np.blackman(width[0]).reshape((-1, 1, 1)) * np.blackman(width[1]).reshape((-1, 1)) * np.blackman(width[2])

    def _tukey_window(self, width, alpha):
        """
        Generates a tukey window

        0 <= alpha <=1

        alpha = 0 becomes rectangular
        alpha = 1 becomes a Hann window
        """
        from scipy.signal.windows import tukey

        return tukey(width[0], alpha).reshape((-1, 1, 1)) * tukey(width[1], alpha).reshape((-1, 1)) * tukey(width[2], alpha)

    def _kaiser_window(self, width, beta):
        """
        Generates a kaiser window

        beta Window shape
        0    Rectangular
        5    Similar to a Hamming
        6    Similar to a Hann
        8.6  Similar to a Blackman
        """
        return np.kaiser(width[0], beta).reshape((-1, 1, 1)) * np.kaiser(width[1], beta).reshape((-1, 1)) * np.kaiser(width[2], beta)


AlgorithmFactory.subscribe(DeltaPDF3D)
