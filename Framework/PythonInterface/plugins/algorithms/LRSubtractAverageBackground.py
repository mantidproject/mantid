# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name
from mantid.api import AlgorithmFactory, AnalysisDataService, PythonAlgorithm, WorkspaceProperty
from mantid.kernel import Direction, IntArrayLengthValidator, IntArrayProperty, StringListValidator
from mantid.simpleapi import Minus, RefRoi


class LRSubtractAverageBackground(PythonAlgorithm):
    def category(self):
        return "Reflectometry\\SNS"

    def name(self):
        return "LRSubtractAverageBackground"

    def version(self):
        return 1

    def summary(self):
        return "Liquids Reflectometer background subtraction using the average on each side of the peak."

    def PyInit(self):
        self.declareProperty(WorkspaceProperty("InputWorkspace", "", Direction.Input), "The workspace to check.")
        self.declareProperty(
            IntArrayProperty("PeakRange", [150, 160], IntArrayLengthValidator(2), direction=Direction.Input),
            "Pixel range defining the reflectivity peak",
        )
        self.declareProperty(
            IntArrayProperty("BackgroundRange", [147, 163], IntArrayLengthValidator(2), direction=Direction.Input),
            "Pixel range defining the outer range of the background on each side of the peak",
        )
        self.declareProperty(
            IntArrayProperty("LowResolutionRange", [94, 160], IntArrayLengthValidator(2), direction=Direction.Input),
            "Pixel range defining the low-resolution axis to integrate over",
        )
        self.declareProperty("SumPeak", False, doc="If True, the resulting peak will be summed")
        self.declareProperty(
            "ErrorWeighting",
            False,
            "If True, a weighted average is used to to estimate the background. Otherwise, a simple average is used.",
        )
        detector_list = ["2D-Detector", "LinearDetector"]
        self.declareProperty("TypeOfDetector", "2D-Detector", StringListValidator(detector_list), doc="The type of detector used")
        self.declareProperty(WorkspaceProperty("OutputWorkspace", "", Direction.Output), "The workspace to check.")

    def PyExec(self):
        workspace = self.getProperty("InputWorkspace").value

        # Signal region
        peak_range = self.getProperty("PeakRange").value
        peak_min = int(peak_range[0])
        peak_max = int(peak_range[1])

        # Background outer region
        bck_range = self.getProperty("BackgroundRange").value
        bck_min = int(bck_range[0])
        bck_max = int(bck_range[1])

        # Low-resolution range
        x_range = self.getProperty("LowResolutionRange").value
        x_min = int(x_range[0])
        x_max = int(x_range[1])

        sum_peak = self.getProperty("SumPeak").value

        # Number of pixels in each direction
        detector = self.getProperty("TypeOfDetector").value
        if detector == "LinearDetector":
            number_of_pixels_x = 1
            number_of_pixels_y = int(workspace.getNumberHistograms())
        else:
            # TODO: revisit this when we update the IDF
            if workspace.getInstrument().hasParameter("number-of-x-pixels"):
                number_of_pixels_x = int(workspace.getInstrument().getNumberParameter("number-of-x-pixels")[0])
            else:
                raise RuntimeError("Instrument does not have parameter number-of-x-pixels")
            if workspace.getInstrument().hasParameter("number-of-y-pixels"):
                number_of_pixels_y = int(workspace.getInstrument().getNumberParameter("number-of-y-pixels")[0])
            else:
                raise RuntimeError("Instrument does not have parameter number-of-y-pixels")

        left_bck = None
        use_weighted_error = self.getProperty("ErrorWeighting").value
        if peak_min > bck_min:
            left_bck = RefRoi(
                InputWorkspace=workspace,
                IntegrateY=False,
                NXPixel=number_of_pixels_x,
                NYPixel=number_of_pixels_y,
                ConvertToQ=False,
                XPixelMin=x_min,
                XPixelMax=x_max,
                YPixelMin=bck_min,
                YPixelMax=peak_min - 1,
                ErrorWeighting=use_weighted_error,
                SumPixels=True,
                NormalizeSum=True,
            )

        right_bck = None
        if peak_max < bck_max:
            right_bck = RefRoi(
                InputWorkspace=workspace,
                IntegrateY=False,
                NXPixel=number_of_pixels_x,
                NYPixel=number_of_pixels_y,
                ConvertToQ=False,
                XPixelMin=x_min,
                XPixelMax=x_max,
                YPixelMin=peak_max + 1,
                YPixelMax=bck_max,
                ErrorWeighting=use_weighted_error,
                SumPixels=True,
                NormalizeSum=True,
            )

        if right_bck is not None and left_bck is not None:
            average = (left_bck + right_bck) / 2.0
        elif right_bck is not None:
            average = right_bck
        elif left_bck is not None:
            average = left_bck
        else:
            average = RefRoi(
                InputWorkspace=workspace,
                IntegrateY=False,
                NXPixel=number_of_pixels_x,
                NYPixel=number_of_pixels_y,
                ConvertToQ=False,
                XPixelMin=x_min,
                XPixelMax=x_max,
                YPixelMin=bck_min,
                YPixelMax=bck_max,
                ErrorWeighting=use_weighted_error,
                SumPixels=True,
                NormalizeSum=True,
            )

        output_name = self.getPropertyValue("OutputWorkspace")
        # Integrate over the low-res direction
        output = RefRoi(
            InputWorkspace=workspace,
            IntegrateY=False,
            NXPixel=number_of_pixels_x,
            NYPixel=number_of_pixels_y,
            XPixelMin=x_min,
            XPixelMax=x_max,
            ConvertToQ=False,
            SumPixels=sum_peak,
            OutputWorkspace=output_name,
        )
        Minus(LHSWorkspace=output, RHSWorkspace=average, OutputWorkspace=output_name)
        # Avoid leaving trash behind
        average_name = str(average)
        if AnalysisDataService.doesExist(str(left_bck)):
            AnalysisDataService.remove(str(left_bck))
        if AnalysisDataService.doesExist(str(right_bck)):
            AnalysisDataService.remove(str(right_bck))
        if AnalysisDataService.doesExist(average_name):
            AnalysisDataService.remove(average_name)

        self.setProperty("OutputWorkspace", output_name)


AlgorithmFactory.subscribe(LRSubtractAverageBackground)
