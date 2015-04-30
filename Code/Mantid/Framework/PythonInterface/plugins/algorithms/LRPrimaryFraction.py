#pylint: disable=no-init,invalid-name
import math
from mantid.api import *
from mantid.simpleapi import *
from mantid.kernel import *

class LRPrimaryFraction(PythonAlgorithm):

    def category(self):
        return "Reflectometry\\SNS"

    def name(self):
        return "LRPrimaryFraction"

    def version(self):
        return 1

    def summary(self):
        return "Liquids Reflectometer primary fraction ('clocking') calculation"

    def PyInit(self):
        self.declareProperty(WorkspaceProperty("InputWorkspace", "",Direction.Input), "The workspace to check.")
        self.declareProperty(IntArrayProperty("SignalRange", [117, 197],
                                              IntArrayLengthValidator(2), direction=Direction.Input),
                             "Pixel range defining the reflected signal")
        self.declareProperty("BackgroundWidth", 3,
                             doc="Number of pixels defining width of the background on each side of the signal")
        self.declareProperty(FloatArrayProperty("ScalingFactor", [1.0, 0.0], direction=Direction.Output),
                             "Calculated scaling factor and error")

    def PyExec(self):
        workspace = self.getProperty("InputWorkspace").value

        # Background offset in number of pixels
        bck_width = self.getProperty("BackgroundWidth").value

        # Signal region
        [peak_from_pixel, peak_to_pixel] = self.getProperty("SignalRange").value

        # Background outer region
        bck_from_pixel = peak_from_pixel - bck_width
        bck_to_pixel = peak_to_pixel + bck_width

        # Number of pixels in each direction
        #TODO: revisit this when we update the IDF
        number_of_pixels_x = int(workspace.getInstrument().getNumberParameter("number-of-x-pixels")[0])
        number_of_pixels_y = int(workspace.getInstrument().getNumberParameter("number-of-y-pixels")[0])

        # Sum up the low-resolution axis and sum up all the wavelengths
        workspace = Integration(InputWorkspace=workspace)
        workspace = RefRoi(InputWorkspace=workspace,
                           NXPixel=number_of_pixels_x, NYPixel=number_of_pixels_y,
                           IntegrateY=False, ConvertToQ=False)
        workspace = Transpose(InputWorkspace=workspace)

        data_y = workspace.dataY(0)
        data_e = workspace.dataE(0)

        # Compute average background on each side
        avg_bck = 0
        avg_bck_err = 0
        for i in range(bck_from_pixel, peak_from_pixel):
            if data_e[i] == 0:
                data_e[i] = 1.0
            avg_bck += data_y[i] / data_e[i] / data_e[i]
            avg_bck_err += 1.0 / data_e[i] / data_e[i]

        for i in range(peak_to_pixel+1, bck_to_pixel+1):
            if data_e[i] == 0:
                data_e[i] = 1
            avg_bck += data_y[i] / data_e[i] / data_e[i]
            avg_bck_err += 1.0 / data_e[i] / data_e[i]

        if avg_bck_err > 0:
            avg_bck /= avg_bck_err
            avg_bck_err = math.sqrt(1.0/avg_bck_err)

        # Subtract average background from specular peak pixels and sum
        specular_counts = 0
        specular_counts_err = 0
        for i in range(peak_from_pixel, peak_to_pixel+1):
            specular_counts += data_y[i] - avg_bck
            if data_e[i] == 0:
                data_e[i] = 1.0
            specular_counts_err += data_e[i] * data_e[i] + avg_bck_err * avg_bck_err
        specular_counts_err = math.sqrt(specular_counts_err)

        total_counts = sum(data_y)

        # Specular ratio
        r = specular_counts / total_counts
        r_err = r * math.sqrt(specular_counts_err * specular_counts_err / specular_counts / specular_counts) + 1.0/total_counts

        self.setProperty("ScalingFactor", [r, r_err])

        logger.information("Total counts:       %s" % total_counts)
        logger.information("Average background: %s +- %s" % (avg_bck, avg_bck_err))
        logger.information("Primary counts:     %s +- %s" % (specular_counts, specular_counts_err))
        logger.information("Primary fraction:   %s +- %s" % (r, r_err))


AlgorithmFactory.subscribe(LRPrimaryFraction)
