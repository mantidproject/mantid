# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name, arguments-differ, unused-variable
"""
Implementation of reduction steps for SANS
"""

import math
import pickle
from reduction import ReductionStep
from reduction import validate_step

# Mantid imports
import mantid
from mantid.simpleapi import *

# Define a SANS specific logger
from mantid.kernel import Logger
import mantid.simpleapi as api
from mantid.api import AnalysisDataService

sanslog = Logger("SANS")


class BaseBeamFinder(ReductionStep):
    """
    Base beam finder. Holds the position of the beam center
    and the algorithm for calculates it using the beam's
    displacement under gravity
    """

    def __init__(self, beam_center_x=None, beam_center_y=None):
        """
        Initial beam center is given in pixel coordinates
        @param beam_center_x: pixel position of the beam in x
        @param beam_center_y: pixel position of the beam in y
        """
        super(BaseBeamFinder, self).__init__()
        self._beam_center_x = beam_center_x
        self._beam_center_y = beam_center_y
        self._beam_radius = None
        self._datafile = None
        self._persistent = True

    def set_persistent(self, persistent):
        self._persistent = persistent
        return self

    def get_beam_center(self):
        """
        Returns the beam center
        """
        return [self._beam_center_x, self._beam_center_y]

    def execute(self, _reducer, _workspace=None):
        return "Beam Center set at: %s %s" % (str(self._beam_center_x), str(self._beam_center_y))

    def _find_beam(self, direct_beam, reducer, _workspace=None):
        if self._beam_center_x is not None and self._beam_center_y is not None:
            return "Using Beam Center at: %g %g" % (self._beam_center_x, self._beam_center_y)

        beam_x, beam_y, msg = SANSBeamFinder(
            Filename=self._datafile,
            UseDirectBeamMethod=direct_beam,
            BeamRadius=self._beam_radius,
            PersistentCorrection=self._persistent,
            ReductionProperties=reducer.get_reduction_table_name(),
        )

        self._beam_center_x = beam_x
        self._beam_center_y = beam_y
        return msg


class BaseTransmission(ReductionStep):
    """
    Base transmission. Holds the transmission value
    as well as the algorithm for calculating it.
    TODO: ISIS doesn't use ApplyTransmissionCorrection, perhaps it's in Q1D, can we remove it from here?
    """

    def __init__(self, trans=0.0, error=0.0, theta_dependent=True):
        super(BaseTransmission, self).__init__()
        self._trans = float(trans)
        self._error = float(error)
        self._theta_dependent = theta_dependent
        self._dark_current_data = None
        self._dark_current_subtracter = None
        self._beam_finder = None

    def set_theta_dependence(self, theta_dependence=True):
        """
        Set the flag for whether or not we want the full theta-dependence
        included in the correction. Setting this flag to false will result
        in simply dividing by the zero-angle transmission.
        @param theta_dependence: theta-dependence included if True
        """
        self._theta_dependent = theta_dependence

    def set_dark_current(self, dark_current=None):
        """
        Set the dark current data file to be subtracted from each transmission data file
        @param dark_current: path to dark current data file
        """
        self._dark_current_data = dark_current

    @validate_step
    def set_dark_current_subtracter(self, subtracter):
        self._dark_current_subtracter = subtracter

    def get_transmission(self):
        return [self._trans, self._error]

    @validate_step
    def set_beam_finder(self, beam_finder):
        self._beam_finder = beam_finder

    def execute(self, reducer, workspace):
        if self._theta_dependent:
            ApplyTransmissionCorrection(
                InputWorkspace=workspace, TransmissionValue=self._trans, TransmissionError=self._error, OutputWorkspace=workspace
            )
        else:
            CreateSingleValuedWorkspace(OutputWorkspace="transmission", DataValue=self._trans, ErrorValue=self._error)
            Divide(LHSWorkspace=workspace, RHSWorkspace="transmission", OutputWorkspace=workspace)

        return "Transmission correction applied for T = %g +- %g" % (self._trans, self._error)


class Normalize(ReductionStep):
    """
    Normalize the data to timer or a spectrum, typically a monitor,
    with in the workspace. By default the normalization is done with
    respect to the Instrument's incident monitor
    """

    def __init__(self, normalization_spectrum=None):
        super(Normalize, self).__init__()
        self._normalization_spectrum = normalization_spectrum

    def get_normalization_spectrum(self):
        return self._normalization_spectrum

    def execute(self, reducer, workspace):
        if self._normalization_spectrum is None:
            self._normalization_spectrum = reducer.instrument.get_incident_mon()

        # Get counting time or monitor
        if self._normalization_spectrum == reducer.instrument.NORMALIZATION_MONITOR:
            norm_count = mtd[workspace].getRun().getProperty("monitor").value
            # HFIR-specific: If we count for monitor we need to multiply by 1e8
            Scale(InputWorkspace=workspace, OutputWorkspace=workspace, Factor=1.0e8 / norm_count, Operation="Multiply")
            return "Normalization by monitor: %6.2g counts" % norm_count
        elif self._normalization_spectrum == reducer.instrument.NORMALIZATION_TIME:
            norm_count = mtd[workspace].getRun().getProperty("timer").value
            Scale(InputWorkspace=workspace, OutputWorkspace=workspace, Factor=1.0 / norm_count, Operation="Multiply")
            return "Normalization by time: %6.2g sec" % norm_count
        else:
            logger.notice("Normalization step did not get a valid normalization option: skipping")
            return "Normalization step did not get a valid normalization option: skipping"

    def clean(self):
        DeleteWorkspace(Workspace=norm_ws)


class Mask(ReductionStep):
    # pylint: disable = too-many-instance-attributes, redefined-builtin
    """
    Marks some spectra so that they are not included in the analysis

    ORNL & ISIS

    """

    def __init__(self):
        """
        Initialize masking
        """
        super(Mask, self).__init__()
        self._nx_low = 0
        self._nx_high = 0
        self._ny_low = 0
        self._ny_high = 0

        self._xml = []

        # these spectra will be masked by the algorithm MaskDetectors
        self.detect_list = []

        # List of pixels to mask
        self.masked_pixels = []

        # Only apply mask defined in the class and ignore additional
        # information from the run properties
        self._ignore_run_properties = False

    def mask_edges(self, nx_low=0, nx_high=0, ny_low=0, ny_high=0):
        """
        Define a "picture frame" outside of which the spectra from all detectors are to be masked.
        @param nx_low: number of pixels to mask on the lower-x side of the detector
        @param nx_high: number of pixels to mask on the higher-x side of the detector
        @param ny_low: number of pixels to mask on the lower-y side of the detector
        @param ny_high: number of pixels to mask on the higher-y side of the detector
        """
        self._nx_low = nx_low
        self._nx_high = nx_high
        self._ny_low = ny_low
        self._ny_high = ny_high

    def add_xml_shape(self, complete_xml_element):
        """
        Add an arbitrary shape to region to be masked
        @param complete_xml_element: description of the shape to add
        """
        if not complete_xml_element.startswith("<"):
            raise ValueError("Excepted xml string but found: " + str(complete_xml_element))
        self._xml.append(complete_xml_element)

    def _infinite_plane(self, id, plane_pt, normal_pt, complement=False):
        """
        Generates xml code for an infinite plane
        @param id: a string to refer to the shape by
        @param plane_pt: a point in the plane
        @param normal_pt: the direction of a normal to the plane
        @param complement: mask in the direction of the normal or away
        @return the xml string
        """
        return (
            '<infinite-plane id="'
            + str(id)
            + '">'
            + '<point-in-plane x="'
            + str(plane_pt[0])
            + '" y="'
            + str(plane_pt[1])
            + '" z="'
            + str(plane_pt[2])
            + '" />'
            + '<normal-to-plane x="'
            + str(normal_pt[0])
            + '" y="'
            + str(normal_pt[1])
            + '" z="'
            + str(normal_pt[2])
            + '" />'
            + "</infinite-plane>\n"
        )

    def _infinite_cylinder(self, centre, radius, axis, id="shape"):
        """
        Generates xml code for an infintely long cylinder
        @param centre: a tuple for a point on the axis
        @param radius: cylinder radius
        @param axis: cylinder orientation
        @param id: a string to refer to the shape by
        @return the xml string
        """
        return (
            '<infinite-cylinder id="'
            + str(id)
            + '">'
            + '<centre x="'
            + str(centre[0])
            + '" y="'
            + str(centre[1])
            + '" z="'
            + str(centre[2])
            + '" />'
            + '<axis x="'
            + str(axis[0])
            + '" y="'
            + str(axis[1])
            + '" z="'
            + str(axis[2])
            + '" />'
            + '<radius val="'
            + str(radius)
            + '" /></infinite-cylinder>\n'
        )

    def _finite_cylinder(self, centre, radius, height, axis, id="shape"):
        """
        Generates xml code for an infintely long cylinder
        @param centre: a tuple for a point on the axis
        @param radius: cylinder radius
        @param height: cylinder height
        @param axis: cylinder orientation
        @param id: a string to refer to the shape by
        @return the xml string
        """
        return (
            '<cylinder id="'
            + str(id)
            + '">'
            + '<centre-of-bottom-base x="'
            + str(centre[0])
            + '" y="'
            + str(centre[1])
            + '" z="'
            + str(centre[2])
            + '" />'
            + '<axis x="'
            + str(axis[0])
            + '" y="'
            + str(axis[1])
            + '" z="'
            + str(axis[2])
            + '" />'
            + '<radius val="'
            + str(radius)
            + '" /><height val="'
            + str(height)
            + '" /></cylinder>\n'
        )

    def add_cylinder(self, radius, xcentre, ycentre, ID="shape"):
        """Mask the inside of an infinite cylinder on the input workspace."""
        self.add_xml_shape(self._infinite_cylinder([xcentre, ycentre, 0.0], radius, [0, 0, 1], id=ID) + '<algebra val="' + str(ID) + '"/>')

    def add_outside_cylinder(self, radius, xcentre=0.0, ycentre=0.0, ID="shape"):
        """Mask out the outside of a cylinder or specified radius"""
        self.add_xml_shape(self._infinite_cylinder([xcentre, ycentre, 0.0], radius, [0, 0, 1], id=ID) + '<algebra val="#' + str(ID) + '"/>')

    def add_pixel_rectangle(self, x_min, x_max, y_min, y_max):
        """
        Mask out a rectangle area defined in pixel coordinates.
        @param x_min: Minimum x to mask
        @param x_max: Maximum x to mask
        @param y_min: Minimum y to mask
        @param y_max: Maximum y to mask
        """
        for ix in range(x_min, x_max + 1):
            for iy in range(y_min, y_max + 1):
                self.masked_pixels.append([ix, iy])

    def add_detector_list(self, det_list):
        """
        Mask the given detectors
        @param det_list: list of detector IDs
        """
        self.detect_list.extend(det_list)

    def ignore_run_properties(self, ignore=True):
        """
        Only use the mask information set in the current object
        and ignore additional information that may have been
        stored in the workspace properties.
        """
        self._ignore_run_properties = ignore

    def execute(self, reducer, workspace):
        # Check whether the workspace has mask information
        run = mtd[workspace].run()
        if not self._ignore_run_properties and run.hasProperty("rectangular_masks"):
            mask_str = run.getProperty("rectangular_masks").value
            try:
                rectangular_masks = pickle.loads(mask_str)
            except (Exception, Warning):
                rectangular_masks = []
                toks = mask_str.split(",")
                for item in toks:
                    if len(item) > 0:
                        c = item.strip().split(" ")
                        if len(c) == 4:
                            rectangular_masks.append([int(c[0]), int(c[2]), int(c[1]), int(c[3])])
            for rec in rectangular_masks:
                try:
                    self.add_pixel_rectangle(rec[0], rec[1], rec[2], rec[3])
                except (Exception, Warning):
                    mantid.logger.notice("Badly defined mask from configuration file: %s" % str(rec))

        for shape in self._xml:
            api.MaskDetectorsInShape(Workspace=workspace, ShapeXML=shape)

        instrument = reducer.instrument
        # Get a list of detector pixels to mask
        if self._nx_low != 0 or self._nx_high != 0 or self._ny_low != 0 or self._ny_high != 0:
            self.masked_pixels.extend(instrument.get_masked_pixels(self._nx_low, self._nx_high, self._ny_low, self._ny_high, workspace))

        if len(self.detect_list) > 0:
            MaskDetectors(Workspace=workspace, DetectorList=self.detect_list)

        # Mask out internal list of pixels
        if len(self.masked_pixels) > 0:
            # Transform the list of pixels into a list of Mantid detector IDs
            masked_detectors = instrument.get_detector_from_pixel(self.masked_pixels, workspace)
            # Mask the pixels by passing the list of IDs
            MaskDetectors(Workspace=workspace, DetectorList=masked_detectors)

        output_ws, detector_list = ExtractMask(InputWorkspace=workspace, OutputWorkspace="__mask")
        mantid.logger.notice("Mask check %s: %g masked pixels" % (workspace, len(detector_list)))

        return "Mask applied %s: %g masked pixels" % (workspace, len(detector_list))


class CorrectToFileStep(ReductionStep):
    """
    Runs the algorithm CorrectToFile()

    ISIS only

    """

    def __init__(self, file="", corr_type="", operation=""):
        """
        Parameters passed to this function are passed to
        the CorrectToFile() algorithm
        @param file: full path of the correction file
        @param corr_type: "deltaE", "TOF", "SpectrumNumber" or any valid setting for CorrectToFile()'s as FirstColumnValue property
        @param operation: set to "Divide" or "Multiply"
        """
        super(CorrectToFileStep, self).__init__()
        self._filename = file
        self._corr_type = corr_type
        self._operation = operation

    def get_filename(self):
        return self._filename

    def set_filename(self, filename):
        self._filename = filename

    def execute(self, reducer, workspace):
        if self._filename:
            CorrectToFile(
                WorkspaceToCorrect=workspace,
                Filename=self._filename,
                OutputWorkspace=workspace,
                FirstColumnValue=self._corr_type,
                WorkspaceOperation=self._operation,
            )


class CalculateNorm(object):
    """
    Generates the normalization workspaces required by Q1D or Qxy from output
    of other, sometimes optional, reduction_steps or specified workspaces.
    Workspaces for wavelength adjustment must have their
    distribution/non-distribution flag set correctly as they maybe converted

    ISIS only
    ORNL doesn't use that approach

    """

    TMP_WORKSPACE_NAME = "__CalculateNorm_loaded_temp"
    WAVE_CORR_NAME = "__Q_WAVE_conversion_temp"
    PIXEL_CORR_NAME = "__Q_pixel_conversion_temp"

    def __init__(self, wavelength_deps=[]):
        super(CalculateNorm, self).__init__()
        self._wave_steps = wavelength_deps
        self._wave_adjs = []
        # if this attribute is set a pixel correction file is read
        self._pixel_file = ""

        # algorithm to be used to load pixel correction files
        self._load = "Load"
        # a parameters string to add as the last argument to the above algorithm
        self._load_params = ""

    def setPixelCorrFile(self, filename):
        """
        Adds a scaling that is a function of the detector (spectrum index)
        from a file
        """
        self._pixel_file = filename

    def getPixelCorrFile(self):
        """
        @return the file that has been set to load as the pixelAdj workspace or '' if none has been set
        """
        return self._pixel_file

    def _is_point_data(self, wksp):
        """
        Tests if the workspace whose name is passed contains point or histogram data
        The test is if the X and Y array lengths are the same = True, different = false
        @param wksp: name of the workspace to test
        @return True for point data, false for histogram
        """
        handle = mtd[wksp]
        if len(handle.readX(0)) == len(handle.readY(0)):
            return True
        else:
            return False

    def calculate(self, reducer, wave_wksps=[]):
        """
        Multiplies all the wavelength scalings into one workspace and all the detector
        dependent scalings into another workspace that can be used by ConvertToQ. It is important
        that the wavelength correction workspaces have a know distribution/non-distribution state
        @param reducer: settings used for this reduction
        @param wave_wksps: additional wavelength dependent correction workspaces to include
        """
        for step in self._wave_steps:
            if step.output_wksp:
                wave_wksps.append(step.output_wksp)

        wave_adj = None
        for wksp in wave_wksps:
            # before the workspaces can be combined they all need to match
            api.RebinToWorkspace(WorkspaceToRebin=wksp, WorkspaceToMatch=reducer.output_wksp, OutputWorkspace=self.TMP_WORKSPACE_NAME)

            if not wave_adj:
                # first time around this loop
                wave_adj = self.WAVE_CORR_NAME
                api.RenameWorkspace(InputWorkspace=self.TMP_WORKSPACE_NAME, OutputWorkspace=wave_adj)
            else:
                api.Multiply(LHSWorkspace=self.TMP_WORKSPACE_NAME, RHSWorkspace=wave_adj, OutputWorkspace=wave_adj)

        # read pixel correction file
        # note the python code below is an attempt to emulate function overloading
        # If a derived class overwrite self._load and self._load_params then
        # a custom specific loading can be achieved
        pixel_adj = ""
        if self._pixel_file:
            pixel_adj = self.PIXEL_CORR_NAME
            load_com = self._load + '(Filename="' + self._pixel_file + '",OutputWorkspace="' + pixel_adj + '"'
            if self._load_params:
                load_com += "," + self._load_params
            load_com += ")"
            eval(load_com)

        if AnalysisDataService.doesExist(self.TMP_WORKSPACE_NAME):
            AnalysisDataService.remove(self.TMP_WORKSPACE_NAME)

        return wave_adj, pixel_adj


# pylint: disable = too-many-instance-attributes
class ConvertToQ(ReductionStep):
    """
    Runs the Q1D or Qxy algorithms to convert wavelength data into momentum transfer, Q

    ISIS only
    ORNL uses WeightedAzimuthalAverage

    """

    # the list of possible Q conversion algorithms to use
    _OUTPUT_TYPES = {"1D": "Q1D", "2D": "Qxy"}
    # defines if Q1D should correct for gravity by default
    _DEFAULT_GRAV = False

    def __init__(self, normalizations):
        """
        @param normalizations: CalculateNormISIS object contains the workspace, ReductionSteps or
        files require for the optional normalization arguments
        """
        super(ConvertToQ, self).__init__()

        if not issubclass(normalizations.__class__, CalculateNorm):
            raise RuntimeError("Error initializing ConvertToQ, invalid normalization object")
        # contains the normalization optional workspaces to pass to the Q algorithm
        self._norms = normalizations

        # this should be set to 1D or 2D
        self._output_type = "1D"
        # the algorithm that corresponds to the above choice
        self._Q_alg = self._OUTPUT_TYPES[self._output_type]
        # if true gravity is taken into account in the Q1D calculation
        self._use_gravity = self._DEFAULT_GRAV
        # used to implement a default setting for gravity that can be over written but doesn't over write
        self._grav_set = False
        # this should contain the rebin parameters
        self.binning = None
        # if set to true the normalization is done out side of the convert to Q algorithm
        self.prenorm = False
        # The minimum distance in metres from the beam center at which all wavelengths are used in the calculation
        self.r_cut = 0.0
        # The shortest wavelength in angstrom at which counts should be summed from all detector pixels in Angstrom
        self.w_cut = 0.0
        # Whether to output parts when running either Q1D2 or Qxy
        self.outputParts = False

    def set_output_type(self, descript):
        """
        Requests the given output from the Q conversion, either 1D or 2D. For
        the 1D calculation it asks the reducer to keep a workspace for error
        estimates
        @param descript: 1D or 2D
        """
        self._Q_alg = self._OUTPUT_TYPES[descript]
        self._output_type = descript

    def get_output_type(self):
        return self._output_type

    output_type = property(get_output_type, set_output_type, None, None)

    def get_gravity(self):
        return self._use_gravity

    def set_gravity(self, flag, override=True):
        """
        Enable or disable including gravity when calculating Q
        @param flag: set to True to enable the gravity correction
        @param override: over write the setting from a previous call to this method (default is True)
        """
        if override:
            self._grav_set = True

        if (not self._grav_set) or override:
            self._use_gravity = bool(flag)
        else:
            msg = "User file can't override previous gravity setting, do gravity correction remains " + str(self._use_gravity)
            print(msg)
            sanslog.warning(msg)

    def execute(self, reducer, workspace):
        """
        Calculate the normalization workspaces and then call the chosen Q conversion algorithm
        """
        # create normalization workspaces
        if self._norms:
            # the empty list at the end appears to be needed (the system test SANS2DWaveloops) is this a bug in Python?
            wave_adj, pixel_adj = self._norms.calculate(reducer, [])
        else:
            raise RuntimeError("Normalization workspaces must be created by CalculateNorm() and passed to this step")

        # If some prenormalization flag is set - normalize data with wave_adj and pixel_adj
        if self.prenorm:
            data = mtd[workspace]
            if wave_adj:
                data /= mtd[wave_adj]
            if pixel_adj:
                data /= mtd[pixel_adj]
            self._deleteWorkspaces([wave_adj, pixel_adj])
            wave_adj, pixel_adj = "", ""

        try:
            if self._Q_alg == "Q1D":
                Q1D(
                    DetBankWorkspace=workspace,
                    OutputWorkspace=workspace,
                    OutputBinning=self.binning,
                    WavelengthAdj=wave_adj,
                    PixelAdj=pixel_adj,
                    AccountForGravity=self._use_gravity,
                    RadiusCut=self.r_cut * 1000.0,
                    WaveCut=self.w_cut,
                    OutputParts=self.outputParts,
                )

            elif self._Q_alg == "Qxy":
                Qxy(
                    InputWorkspace=workspace,
                    OutputWorkspace=workspace,
                    MaxQxy=reducer.QXY2,
                    DeltaQ=reducer.DQXY,
                    WavelengthAdj=wave_adj,
                    PixelAdj=pixel_adj,
                    AccountForGravity=self._use_gravity,
                    RadiusCut=self.r_cut * 1000.0,
                    WaveCut=self.w_cut,
                    OutputParts=self.outputParts,
                )
                ReplaceSpecialValues(InputWorkspace=workspace, OutputWorkspace=workspace, NaNValue="0", InfinityValue="0")
            else:
                raise NotImplementedError("The type of Q reduction has not been set, e.g. 1D or 2D")
        except:
            raise
        finally:
            self._deleteWorkspaces([wave_adj, pixel_adj])

    def _deleteWorkspaces(self, workspaces):
        """
        Deletes a list of workspaces if they exist but ignores any errors
        @param workspaces: list of workspaces to try to delete
        """
        for wk in workspaces:
            try:
                if AnalysisDataService.doesExist(wk):
                    AnalysisDataService.remove(wk)
            except (Exception, Warning):
                # if the workspace can't be deleted this function does nothing
                pass


class GetSampleGeom(ReductionStep):
    """
    Loads, stores, retrieves, etc. data about the geometry of the sample
    On initialisation this class will return default geometry values (compatible with the Colette software)
    There are functions to override these settings
    On execute if there is geometry information in the workspace this will override any unset attributes

    ISIS only
    ORNL only divides by thickness, in the absolute scaling step

    """

    # IDs for each shape as used by the Colette software
    _shape_ids = {1: "cylinder-axis-up", 2: "cuboid", 3: "cylinder-axis-along"}
    _default_shape = "cylinder-axis-along"

    def __init__(self):
        super(GetSampleGeom, self).__init__()

        # string specifies the sample's shape
        self._shape = None
        # sample's width
        self._width = None
        self._thickness = None
        self._height = None

        self._use_wksp_shape = True
        self._use_wksp_width = True
        self._use_wksp_thickness = True
        self._use_wksp_height = True

    def _get_default(self, attrib):
        if attrib == "shape":
            return self._default_shape
        elif attrib == "width" or attrib == "thickness" or attrib == "height":
            return 1.0

    def set_shape(self, new_shape):
        """
        Sets the sample's shape from a string or an ID. If the ID is not
        in the list of allowed values the shape is set to the default but
        shape strings not in the list are not checked
        """
        try:
            # deal with ID numbers as arguments
            new_shape = self._shape_ids[int(new_shape)]
        except ValueError:
            # means that we weren't passed an ID number, the code below treats it as a shape name
            pass
        except KeyError:
            sanslog.warning("Warning: Invalid geometry type for sample: " + str(new_shape) + ". Setting default to " + self._default_shape)
            new_shape = self._default_shape

        self._shape = new_shape
        self._use_wksp_shape = False

        # check that the dimensions that we have make sense for our new shape
        if self._width:
            self.width = self._width
        if self._thickness:
            self.thickness = self._thickness

    def get_shape(self):
        if self._shape is None:
            return self._get_default("shape")
        else:
            return self._shape

    def set_width(self, width):
        self._width = float(width)
        self._use_wksp_width = False
        # For a disk the height=width
        if self._shape and self._shape.startswith("cylinder"):
            self._height = self._width
            self._use_wksp_height = False

    def get_width(self):
        if self._width is None:
            return self._get_default("width")
        else:
            return self._width

    def set_height(self, height):
        self._height = float(height)
        self._use_wksp_height = False

        # For a cylinder and sphere the height=width=radius
        if (self._shape is not None) and (self._shape.startswith("cylinder")):
            self._width = self._height
        self._use_wksp_widtht = False

    def get_height(self):
        if self._height is None:
            return self._get_default("height")
        else:
            return self._height

    def set_thickness(self, thickness):
        """
        Simply sets the variable _thickness to the value passed
        """
        # as only cuboids use the thickness the warning below may be informative
        # if (not self._shape is None) and (not self._shape == 'cuboid'):
        #    mantid.sendLogMessage('::SANS::Warning: Can\'t set thickness for shape "'+self._shape+'"')
        self._thickness = float(thickness)
        self._use_wksp_thickness = False

    def get_thickness(self):
        if self._thickness is None:
            return self._get_default("thickness")
        else:
            return self._thickness

    shape = property(get_shape, set_shape, None, None)
    width = property(get_width, set_width, None, None)
    height = property(get_height, set_height, None, None)
    thickness = property(get_thickness, set_thickness, None, None)

    def execute(self, reducer, workspace):
        """
        Reads the geometry information stored in the workspace
        but doesn't replace values that have been previously set
        """
        wksp = mtd[workspace]
        if isinstance(wksp, mantid.api.WorkspaceGroup):
            wksp = wksp[0]
        sample_details = wksp.sample()

        if self._use_wksp_shape:
            self.shape = sample_details.getGeometryFlag()
        if self._use_wksp_thickness:
            self.thickness = sample_details.getThickness()
        if self._use_wksp_width:
            self.width = sample_details.getWidth()
        if self._use_wksp_height:
            self.height = sample_details.getHeight()

    def __str__(self):
        return (
            "-- Sample Geometry --\n"
            + "    Shape: "
            + self.shape
            + "\n"
            + "    Width: "
            + str(self.width)
            + "\n"
            + "    Height: "
            + str(self.height)
            + "\n"
            + "    Thickness: "
            + str(self.thickness)
            + "\n"
        )


class SampleGeomCor(ReductionStep):
    """
    Correct the neutron count rates for the size of the sample

    ISIS only
    ORNL only divides by thickness, in the absolute scaling step

    """

    def __init__(self):
        self.volume = 1.0

    def calculate_volume(self, reducer):
        geo = reducer.get_sample().geometry
        assert issubclass(geo.__class__, GetSampleGeom)

        try:
            if geo.shape == "cylinder-axis-up":
                # Volume = circle area * height
                # Factor of four comes from radius = width/2
                volume = geo.height * math.pi
                volume *= math.pow(geo.width, 2) / 4.0
            elif geo.shape == "cuboid":
                # Flat plate sample
                volume = geo.width
                volume *= geo.height * geo.thickness
            elif geo.shape == "cylinder-axis-along":
                # Factor of four comes from radius = width/2
                # Disc - where height is not used
                volume = geo.thickness * math.pi
                volume *= math.pow(geo.width, 2) / 4.0
            else:
                raise NotImplementedError('Shape "' + geo.shape + '" is not in the list of supported shapes')
        # pylint: disable=notimplemented-raised
        except TypeError:
            raise TypeError(
                "Error calculating sample volume with width="
                + str(geo.width)
                + " height="
                + str(geo.height)
                + "and thickness="
                + str(geo.thickness)
            )

        return volume

    def execute(self, reducer, workspace):
        """
        Divide the counts by the volume of the sample
        """
        if not reducer.is_can():
            # it calculates the volume for the sample and may or not apply to the can as well.
            self.volume = self.calculate_volume(reducer)

        ws = mtd[str(workspace)]
        ws /= self.volume


class StripEndZeros(ReductionStep):
    # ISIS only
    def __init__(self, flag_value=0.0):
        super(StripEndZeros, self).__init__()
        self._flag_value = flag_value

    def execute(self, reducer, workspace):
        result_ws = mtd[workspace]
        if result_ws.getNumberHistograms() != 1:
            # Strip zeros is only possible on 1D workspaces
            return

        y_vals = result_ws.readY(0)
        length = len(y_vals)
        # Find the first non-zero value
        start = 0
        for i in range(0, length):
            if y_vals[i] != self._flag_value:
                start = i
                break
        # Now find the last non-zero value
        stop = 0
        length -= 1
        for j in range(length, 0, -1):
            if y_vals[j] != self._flag_value:
                stop = j
                break
        # Find the appropriate X values and call CropWorkspace
        x_vals = result_ws.readX(0)
        startX = x_vals[start]
        # Make sure we're inside the bin that we want to crop
        endX = 1.001 * x_vals[stop + 1]
        CropWorkspace(InputWorkspace=workspace, OutputWorkspace=workspace, XMin=startX, XMax=endX)


class StripEndNans(ReductionStep):
    # ISIS only
    def __init__(self):
        super(StripEndNans, self).__init__()

    def execute(self, reducer, workspace):
        """
        Trips leading and trailing Nan values from workspace
        @param reducer: unused
        @param workspace: the workspace to convert
        """
        result_ws = mtd[workspace]
        if result_ws.getNumberHistograms() != 1:
            # Strip zeros is only possible on 1D workspaces
            return

        y_vals = result_ws.readY(0)
        length = len(y_vals)
        # Find the first non-zero value
        start = 0
        for i in range(0, length):
            if not math.isnan(y_vals[i]):
                start = i
                break
        # Now find the last non-zero value
        stop = 0
        length -= 1
        for j in range(length, 0, -1):
            if not math.isnan(y_vals[j]):
                stop = j
                break
        # Find the appropriate X values and call CropWorkspace
        x_vals = result_ws.readX(0)
        startX = x_vals[start]
        # Make sure we're inside the bin that we want to crop
        endX = 1.001 * x_vals[stop + 1]
        api.CropWorkspace(InputWorkspace=workspace, OutputWorkspace=workspace, XMin=startX, XMax=endX)
