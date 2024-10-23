# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name,too-many-locals,too-many-branches
from mantid.api import AlgorithmFactory, AlgorithmManager, MatrixWorkspaceProperty, PythonAlgorithm
from mantid.kernel import (
    logger,
    Direction,
    FloatArrayProperty,
    IntBoundedValidator,
    Logger,
    PropertyManagerDataService,
    StringMandatoryValidator,
)
import math


class SANSAzimuthalAverage1D(PythonAlgorithm):
    def category(self):
        return "Workflow\\SANS\\UsesPropertyManager"

    def name(self):
        return "SANSAzimuthalAverage1D"

    def summary(self):
        return "Compute I(q) for reduced SANS data"

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "", direction=Direction.Input))

        self.declareProperty(
            FloatArrayProperty("Binning", values=[0.0, 0.0, 0.0], direction=Direction.InOut),
            "Positive is linear bins, negative is logarithmic",
        )

        self.declareProperty(
            "NumberOfBins", 100, validator=IntBoundedValidator(lower=1), doc="Number of Q bins to use if binning is not supplied"
        )
        self.declareProperty("LogBinning", False, "Produce log binning in Q when true and binning wasn't supplied")
        self.declareProperty("AlignWithDecades", False, "If True and log binning was chosen, the Q points will be aligned with Q decades")
        self.declareProperty("NumberOfSubpixels", 1, "Number of sub-pixels per side of a detector pixel: use with care")
        self.declareProperty("ErrorWeighting", False, "Backward compatibility option: use with care")
        self.declareProperty("ComputeResolution", False, "If true the Q resolution will be computed")

        self.declareProperty(
            "ReductionProperties",
            "__sans_reduction_properties",
            validator=StringMandatoryValidator(),
            doc="Property manager name for the reduction",
        )

        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", direction=Direction.Output), "I(q) workspace")

        self.declareProperty("NumberOfWedges", 2, "Number of wedges to calculate")
        self.declareProperty("WedgeAngle", 30.0, "Angular opening of each wedge, in degrees")
        self.declareProperty("WedgeOffset", 0.0, "Angular offset for the wedges, in degrees")
        # self.declareProperty(WorkspaceGroupProperty("WedgeWorkspace", "",
        #                                             Direction.Output,
        #                                             PropertyMode.Optional),
        #                     "I(q) wedge workspaces")
        self.declareProperty("OutputMessage", "", direction=Direction.Output, doc="Output message")

    def PyExec(self):
        # Warn user if error-weighting was turned on
        error_weighting = self.getProperty("ErrorWeighting").value
        if error_weighting:
            msg = "The ErrorWeighting option is turned ON. "
            msg += "This option is NOT RECOMMENDED"
            Logger("SANSAzimuthalAverage").warning(msg)

        # Warn against sub-pixels
        n_subpix = self.getProperty("NumberOfSubpixels").value
        if n_subpix != 1:
            msg = "NumberOfSubpixels was set to %s: " % str(n_subpix)
            msg += "The recommended value is 1"
            Logger("SANSAzimuthalAverage").warning(msg)

        # Q binning options
        binning = self.getProperty("Binning").value
        binning_prop = self.getPropertyValue("Binning")

        workspace = self.getProperty("InputWorkspace").value
        output_ws_name = self.getPropertyValue("OutputWorkspace")

        # Q range
        pixel_size_x = workspace.getInstrument().getNumberParameter("x-pixel-size")[0]
        pixel_size_y = workspace.getInstrument().getNumberParameter("y-pixel-size")[0]

        if len(binning) == 0 or (binning[0] == 0 and binning[1] == 0 and binning[2] == 0):
            # Wavelength. Read in the wavelength bins. Skip the first one which is not set up properly for EQ-SANS
            x = workspace.dataX(1)
            x_length = len(x)
            if x_length < 2:
                raise RuntimeError("Azimuthal averaging expects at least one wavelength bin")
            wavelength_max = (x[x_length - 2] + x[x_length - 1]) / 2.0
            wavelength_min = (x[0] + x[1]) / 2.0
            if wavelength_min == 0 or wavelength_max == 0:
                raise RuntimeError("Azimuthal averaging needs positive wavelengths")
            qmin, qstep, qmax = self._get_binning(workspace, wavelength_min, wavelength_max)
            align = self.getProperty("AlignWithDecades").value
            log_binning = self.getProperty("LogBinning").value
            if log_binning and align:
                binning_prop = self._get_aligned_binning(qmin, qmax)
            else:
                binning_prop = "%g, %g, %g" % (qmin, qstep, qmax)
                workspace.getRun().addProperty("qstep", float(qstep), True)
            self.setPropertyValue("Binning", binning_prop)
        else:
            qmin = binning[0]
            qmax = binning[2]
        logger.debug("Qmin = %s" % qmin)
        logger.debug("Qmax = %s" % qmax)
        workspace.getRun().addProperty("qmin", float(qmin), True)
        workspace.getRun().addProperty("qmax", float(qmax), True)
        # If we kept the events this far, we need to convert the input workspace
        # to a histogram here
        if workspace.id() == "EventWorkspace":
            alg = AlgorithmManager.create("ConvertToMatrixWorkspace")
            alg.initialize()
            alg.setChild(True)
            alg.setProperty("InputWorkspace", workspace)
            alg.setPropertyValue("OutputWorkspace", "__tmp_matrix_workspace")
            alg.execute()
            workspace = alg.getProperty("OutputWorkspace").value

        alg = AlgorithmManager.create("Q1DWeighted")
        alg.initialize()
        alg.setChild(True)
        alg.setProperty("InputWorkspace", workspace)
        alg.setPropertyValue("OutputBinning", binning_prop)
        alg.setProperty("NPixelDivision", n_subpix)
        alg.setProperty("PixelSizeX", pixel_size_x)
        alg.setProperty("PixelSizeY", pixel_size_y)
        alg.setProperty("ErrorWeighting", error_weighting)
        alg.setPropertyValue("OutputWorkspace", output_ws_name)
        # wedge_ws_name = self.getPropertyValue("WedgeWorkspace")
        n_wedges = self.getProperty("NumberOfWedges").value
        wedge_angle = self.getProperty("WedgeAngle").value
        wedge_offset = self.getProperty("WedgeOffset").value
        alg.setPropertyValue("WedgeWorkspace", output_ws_name + "_wedges")
        alg.setProperty("NumberOfWedges", n_wedges)
        alg.setProperty("WedgeAngle", wedge_angle)
        alg.setProperty("WedgeOffset", wedge_offset)
        alg.execute()
        output_ws = alg.getProperty("OutputWorkspace").value
        wedge_ws = alg.getProperty("WedgeWorkspace").value

        alg = AlgorithmManager.create("ReplaceSpecialValues")
        alg.initialize()
        alg.setChild(True)
        alg.setProperty("InputWorkspace", output_ws)
        alg.setPropertyValue("OutputWorkspace", output_ws_name)
        alg.setProperty("NaNValue", 0.0)
        alg.setProperty("NaNError", 0.0)
        alg.setProperty("InfinityValue", 0.0)
        alg.setProperty("InfinityError", 0.0)
        alg.execute()
        output_ws = alg.getProperty("OutputWorkspace").value

        # Q resolution
        compute_resolution = self.getProperty("ComputeResolution").value
        if compute_resolution:
            alg = AlgorithmManager.create("ReactorSANSResolution")
            alg.initialize()
            alg.setChild(True)
            alg.setProperty("InputWorkspace", output_ws)
            alg.execute()

        for i in range(wedge_ws.getNumberOfEntries()):
            wedge_i = wedge_ws.getItem(i)
            identifier = i
            if wedge_i.getRun().hasProperty("wedge_angle"):
                identifier = int(wedge_i.getRun().getProperty("wedge_angle").value)
            wedge_i_name = "%s_wedge_%s" % (output_ws_name, identifier)

            alg = AlgorithmManager.create("ReplaceSpecialValues")
            alg.initialize()
            alg.setChild(True)
            alg.setProperty("InputWorkspace", wedge_i)
            alg.setProperty("OutputWorkspace", wedge_i_name)
            alg.setProperty("NaNValue", 0.0)
            alg.setProperty("NaNError", 0.0)
            alg.setProperty("InfinityValue", 0.0)
            alg.setProperty("InfinityError", 0.0)
            alg.execute()
            wedge_i = alg.getProperty("OutputWorkspace").value

            if compute_resolution:
                alg = AlgorithmManager.create("ReactorSANSResolution")
                alg.initialize()
                alg.setChild(True)
                alg.setProperty("InputWorkspace", wedge_i)
                alg.execute()

            self.declareProperty(MatrixWorkspaceProperty("WedgeWorkspace_%s" % i, "", direction=Direction.Output))
            self.setPropertyValue("WedgeWorkspace_%s" % i, wedge_i_name)
            self.setProperty("WedgeWorkspace_%s" % i, wedge_i)

        msg = "Performed radial averaging between Q=%g and Q=%g" % (qmin, qmax)
        self.setProperty("OutputMessage", msg)
        self.setProperty("OutputWorkspace", output_ws)

    def _get_binning(self, workspace, wavelength_min, wavelength_max):
        log_binning = self.getProperty("LogBinning").value
        nbins = self.getProperty("NumberOfBins").value
        if workspace.getRun().hasProperty("qmin") and workspace.getRun().hasProperty("qmax"):
            qmin = workspace.getRun().getProperty("qmin").value
            qmax = workspace.getRun().getProperty("qmax").value
        else:
            #  Checked 8/10/2017 -  this is using the right distance for calculating q
            sample_detector_distance = workspace.getRun().getProperty("sample_detector_distance").value
            nx_pixels = int(workspace.getInstrument().getNumberParameter("number-of-x-pixels")[0])
            ny_pixels = int(workspace.getInstrument().getNumberParameter("number-of-y-pixels")[0])
            pixel_size_x = workspace.getInstrument().getNumberParameter("x-pixel-size")[0]
            pixel_size_y = workspace.getInstrument().getNumberParameter("y-pixel-size")[0]

            if workspace.getRun().hasProperty("beam_center_x") and workspace.getRun().hasProperty("beam_center_y"):
                beam_ctr_x = workspace.getRun().getProperty("beam_center_x").value
                beam_ctr_y = workspace.getRun().getProperty("beam_center_y").value
            else:
                property_manager_name = self.getProperty("ReductionProperties").value
                property_manager = PropertyManagerDataService[property_manager_name]
                if property_manager.existsProperty("LatestBeamCenterX") and property_manager.existsProperty("LatestBeamCenterY"):
                    beam_ctr_x = property_manager.getProperty("LatestBeamCenterX").value
                    beam_ctr_y = property_manager.getProperty("LatestBeamCenterY").value
                else:
                    raise RuntimeError("No beam center information can be found on the data set")

            # Q min is one pixel from the center, unless we have the beam trap size
            if workspace.getRun().hasProperty("beam-trap-diameter"):
                mindist = workspace.getRun().getProperty("beam-trap-diameter").value / 2.0
            else:
                mindist = min(pixel_size_x, pixel_size_y)
            qmin = 4 * math.pi / wavelength_max * math.sin(0.5 * math.atan(mindist / sample_detector_distance))

            dxmax = pixel_size_x * max(beam_ctr_x, nx_pixels - beam_ctr_x)
            dymax = pixel_size_y * max(beam_ctr_y, ny_pixels - beam_ctr_y)
            maxdist = math.sqrt(dxmax * dxmax + dymax * dymax)
            qmax = 4 * math.pi / wavelength_min * math.sin(0.5 * math.atan(maxdist / sample_detector_distance))

        if not log_binning:
            qstep = (qmax - qmin) / nbins
            f_step = (qmax - qmin) / qstep
            n_step = math.floor(f_step)
            if f_step - n_step > 10e-10:
                qmax = qmin + qstep * n_step
            return qmin, qstep, qmax
        else:
            # Note: the log binning in Mantid is x_i+1 = x_i * ( 1 + dx )
            qstep = (math.log10(qmax) - math.log10(qmin)) / nbins
            f_step = (math.log10(qmax) - math.log10(qmin)) / qstep
            n_step = math.floor(f_step)
            if f_step - n_step > 10e-10:
                qmax = math.pow(10.0, math.log10(qmin) + qstep * n_step)
            return qmin, -(math.pow(10.0, qstep) - 1.0), qmax

    def _get_aligned_binning(self, qmin, qmax):
        npts = self.getProperty("NumberOfBins").value

        x_min = math.pow(10, math.floor(npts * math.log10(qmin)) / npts)
        x_max = math.pow(10, math.ceil(npts * math.log10(qmax)) / npts)
        b = 1.0 / npts
        nsteps = int(1 + math.ceil(npts * math.log10(x_max / x_min)))

        # binning = str(x_min)
        x_bound = x_min - (x_min * math.pow(10, b) - x_min) / 2.0
        binning2 = str(x_bound)

        x = x_min
        for dummy_i in range(nsteps):
            x_bound = 2 * x - x_bound
            x *= math.pow(10, b)
            # binning += ",%g,%g" % (x,x)
            binning2 += ",%g,%g" % (x_bound, x_bound)

        return binning2


#############################################################################################

AlgorithmFactory.subscribe(SANSAzimuthalAverage1D)
