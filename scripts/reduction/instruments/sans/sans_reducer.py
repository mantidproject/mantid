# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
SANS-specific implementation of the Reducer. The SANSReducer class implements
a predefined set of reduction steps to be followed. The actual ReductionStep objects
executed for each of those steps can be modified.
"""

from reduction import Reducer
from reduction import ReductionStep
from reduction import validate_step
import sans_reduction_steps
import mantid.simpleapi as api
import warnings
import inspect

## Version number
__version__ = "0.0"


class SANSReducer(Reducer):
    """
    SANS-specific implementation of the Reducer
    """

    ## Normalization options
    # TODO: those also correspond to the timer and monitor spectra -> store this in instr conf instead
    NORMALIZATION_NONE = None
    NORMALIZATION_TIME = 1
    NORMALIZATION_MONITOR = 0

    ## Reduction setup
    _reduction_setup = None

    ## Beam center finder ReductionStep object
    _beam_finder = None

    ## Normalization option
    _normalizer = None
    _absolute_scale = None

    ## Dark current data file
    _dark_current_subtracter = None
    _dark_current_subtracter_class = None

    ## Sensitivity correction ReductionStep object
    _sensitivity_correcter = None

    ## Solid angle correcter
    _solid_angle_correcter = None

    ## Azimuthal averaging
    _azimuthal_averager = None

    ## I(Qx,Qy)
    _two_dim_calculator = None

    ## Transmission calculator
    _transmission_calculator = None

    ## Masking step
    _mask = None

    ## Output saving step
    _save_iq = None

    ## Background subtracter
    _background_subtracter = None

    ## Data loader
    _data_loader = None

    ## Q resolution calculation
    _resolution_calculator = None

    ## Extra data files
    _extra_files = {}

    geometry_correcter = None

    def __init__(self):
        super(SANSReducer, self).__init__()

        # Default beam finder
        self._beam_finder = sans_reduction_steps.BaseBeamFinder()

        # Default normalization
        self._normalizer = None

        # Default data loader
        self._data_loader = None

        # Default mask object
        self._mask = sans_reduction_steps.Mask()

        # Default dark current subtracter class
        self._dark_current_subtracter_class = api.HFIRDarkCurrentSubtraction

        # Resolution calculator
        self._resolution_calculator = api.ReactorSANSResolution

        # Sample geometry correction
        self.geometry_correcter = None

    def set_instrument(self, configuration):
        """
        Sets the instrument and put in the default beam center (usually the
        center of the detector)
        @param configuration: instrument object
        """
        super(SANSReducer, self).set_instrument(configuration)
        center = self.instrument.get_default_beam_center()
        self._beam_finder = sans_reduction_steps.BaseBeamFinder(center[0], center[1])

    @validate_step
    def set_normalizer(self, normalizer):
        """
        Set normalization
        @param normalizer: normalization step object
        """
        self._normalizer = normalizer

    def get_normalizer(self):
        return self._normalizer

    @validate_step
    def set_reduction(self, setup_algorithm):
        self._reduction_setup = setup_algorithm

    @validate_step
    def set_geometry_correcter(self, correcter):
        """
        Set the ReductionStep object that takes care of the geometry correction
        @param subtracter: ReductionStep object
        """
        self.geometry_correcter = correcter

    def set_transmission(self, trans):
        """
        Set the reduction step that will apply the transmission correction
        @param trans: ReductionStep object
        """
        if issubclass(trans.__class__, sans_reduction_steps.BaseTransmission) or trans is None:
            self._transmission_calculator = trans
        else:
            raise RuntimeError("Reducer.set_transmission expects an object of class ReductionStep")

    def get_transmission(self):
        return self._transmission_calculator

    @validate_step
    def set_mask(self, mask):
        """
        Set the reduction step that will apply the mask
        @param mask: ReductionStep object
        """
        self._mask = mask

    def get_mask(self):
        return self._mask

    def get_beam_center(self):
        """
        Return the beam center position
        """
        return self._beam_finder.get_beam_center()

    def set_beam_finder(self, finder):
        """
        Set the ReductionStep object that finds the beam center
        @param finder: BaseBeamFinder object
        """
        if issubclass(finder.__class__, sans_reduction_steps.BaseBeamFinder) or finder is None:
            self._beam_finder = finder
        else:
            raise RuntimeError("Reducer.set_beam_finder expects an object of class ReductionStep")

    @validate_step
    def set_absolute_scale(self, scaler):
        """ """
        self._absolute_scale = scaler

    def set_data_loader(self, loader):
        """
        Set the loader for all data files
        @param loader: ReductionStep object
        """
        if issubclass(loader.__class__, ReductionStep):
            self._data_loader = loader
        else:
            raise RuntimeError("Reducer.set_data_loader expects an object of class ReductionStep")

    def get_data_loader(self):
        return self._data_loader

    @validate_step
    def set_sensitivity_correcter(self, correcter):
        """
        Set the ReductionStep object that applies the sensitivity correction.
        The ReductionStep object stores the sensitivity, so that the object
        can be re-used on multiple data sets and the sensitivity will not be
        recalculated.
        @param correcter: ReductionStep object
        """
        self._sensitivity_correcter = correcter

    def get_sensitivity_correcter(self):
        return self._sensitivity_correcter

    @validate_step
    def set_sensitivity_beam_center(self, beam_center):
        if self._sensitivity_correcter is not None:
            self._sensitivity_correcter.set_beam_center(beam_center)
        else:
            raise RuntimeError("Set the sensitivity correction before setting its beam center")

    @validate_step
    def set_dark_current_subtracter(self, subtracter):
        """
        Set the ReductionStep object that subtracts the dark current from the data.
        The loaded dark current is stored by the ReductionStep object so that the
        subtraction can be applied to multiple data sets without reloading it.
        @param subtracter: ReductionStep object
        """
        self._dark_current_subtracter = subtracter

    @validate_step
    def set_solid_angle_correcter(self, correcter):
        """
        Set the ReductionStep object that performs the solid angle correction.
        @param subtracter: ReductionStep object
        """
        if issubclass(correcter.__class__, ReductionStep) or correcter is None:
            self._solid_angle_correcter = correcter
        else:
            raise RuntimeError("Reducer.set_solid_angle_correcter expects an object of class ReductionStep")

    @validate_step
    def set_azimuthal_averager(self, averager):
        """
        Set the ReductionStep object that performs azimuthal averaging
        and transforms the 2D reduced data set into I(Q).
        @param averager: ReductionStep object
        """
        self._azimuthal_averager = averager

    def get_azimuthal_averager(self):
        """
        Return the azimuthal average reduction step
        """
        return self._azimuthal_averager

    @validate_step
    def set_save_Iq(self, save_iq):
        """
        Set the ReductionStep object that saves the I(q) output
        @param averager: ReductionStep object
        """
        self._save_iq = save_iq

    def set_bck_transmission(self, trans):
        """
        Set the reduction step that will apply the transmission correction
        @param trans: ReductionStep object
        """
        lineno = inspect.currentframe().f_code.co_firstlineno
        warnings.warn_explicit(
            "SANSReducer.set_bck_transmission id deprecated: use get_background().set_transmission()", DeprecationWarning, __file__, lineno
        )

        if issubclass(trans.__class__, sans_reduction_steps.BaseTransmission) or trans is None:
            self._background_subtracter.set_transmission(trans)
        else:
            raise RuntimeError("Reducer.set_bck_transmission expects an object of class ReductionStep")

    @validate_step
    def set_IQxQy(self, calculator):
        """
        Set the algorithm to compute I(Qx,Qy)
        @param calculator: ReductionStep object
        """
        self._two_dim_calculator = calculator

    def pre_process(self):
        """
        Reduction steps that are meant to be executed only once per set
        of data files. After this is executed, all files will go through
        the list of reduction steps.
        """
        if self.instrument is None:
            raise RuntimeError("SANSReducer: trying to run a reduction with no instrument specified")

        if self._reduction_setup is not None:
            result = self._reduction_setup.execute(self)
            self.log_text += "%s\n" % str(result)

        if self._beam_finder is not None:
            result = self._beam_finder.execute(self)
            self.log_text += "%s\n" % str(result)

        # Create the list of reduction steps
        self._to_steps()

    def _2D_steps(self):
        """
        Creates a list of reduction steps to be applied to
        each data set, including the background file.
        Only the steps applied to a data set
        before azimuthal averaging are included.
        """
        reduction_steps = []

        # Load file
        reduction_steps.append(self._data_loader)

        # Dark current subtraction
        if self._dark_current_subtracter is not None:
            reduction_steps.append(self._dark_current_subtracter)

        # Normalize
        if self._normalizer is not None:
            reduction_steps.append(self._normalizer)

        # Mask
        if self._mask is not None:
            reduction_steps.append(self._mask)

        # Solid angle correction
        if self._solid_angle_correcter is not None:
            reduction_steps.append(self._solid_angle_correcter)

        # Sensitivity correction
        if self._sensitivity_correcter is not None:
            reduction_steps.append(self._sensitivity_correcter)

        return reduction_steps

    def _to_steps(self):
        """
        Creates a list of reduction steps for each data set
        following a predefined reduction approach. For each
        predefined step, we check that a ReductionStep object
        exists to take of it. If one does, we append it to the
        list of steps to be executed.
        """
        # Get the basic 2D steps
        self._reduction_steps = self._2D_steps()

        # Apply transmission correction
        if self._transmission_calculator is not None:
            self.append_step(self._transmission_calculator)

        # Subtract the background
        if self._background_subtracter is not None:
            self.append_step(self._background_subtracter)

        if self._absolute_scale is not None:
            self.append_step(self._absolute_scale)

        # Sample geometry correction
        if self.geometry_correcter is not None:
            self.append_step(self.geometry_correcter)

        # Perform azimuthal averaging
        if self._azimuthal_averager is not None:
            self.append_step(self._azimuthal_averager)

        # Perform I(Qx,Qy) calculation
        if self._two_dim_calculator is not None:
            self.append_step(self._two_dim_calculator)

        # Save output to file
        if self._save_iq is not None:
            self.append_step(self._save_iq)
