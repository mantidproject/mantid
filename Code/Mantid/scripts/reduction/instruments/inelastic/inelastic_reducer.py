"""
    Inelastic specific implementation of the Reducer.
"""
import mantid
from reduction import Reducer
# Validate_step is a decorator that allows both Python algorithms and
# ReductionStep objects to be passed to the Reducer.
# It also does minimal type checking to ensure that the object that is passed is valid

from reduction import validate_step
from reduction import validate_loader

## Version number
__version__ = '0.01'

class InelasticReducer(Reducer):
    """
        Inelastic-specific implementation of the Reducer
    """

    ## Data loader
    _data_loader = None
    _offset_banks = None
    _ei_calculator = None
    _normalise = None

    ## Incident energy calculator
    _ei_calculator = None

    ## Detector vanadium correction
    _detector_vanadium = None

    ## Apply Ki/Kf correction
    _kikf_corrector = None

    ## Detector Efficiency
    _detector_efficiency_corrector = None

    ## Background subtraction
    _background_subtractor = None

    ## Masking
    _masker = None

    ## Save NXSPE file
    _nxspe_saver = None

    ## Save Mantid NeXus file
    _nexus_saver = None

    ## Save SPE file
    _spe_saver = None



    def __init__(self):
        super(InelasticReducer, self).__init__()
        # Setup default loader
        self._data_loader = InelasticLoader()
        # TODO: Some defaults?

    @validate_loader
    def set_data_loader(self, loader):
        """
            Set the algorithm to load the data files
            @param loader: Workflow algorithm object
        """
        self._data_loader = loader

    @validate_step
    def set_ei_calculator(self, calculator):
        """
            Set the algorithm to calculate (or fix) the incident energy
            @param calculator: Workflow algorithm object
        """
        self._ei_calculator = calculator

    @validate_step
    def set_detector_vandium(self, normaliser):
        """
            Set the step to perform the detector vanadium normalisation
            @param normaliser: Workflow algorithm object
        """
        self._detector_vanadium = normaliser

    @validate_step
    def set_kikf_correction(self, corrector):
        """
            Set the step that will apply the Ki/Kf scaling correction
            @param corrector: Workflow algorithm object
        """
        self._kikf_corrector = corrector

    @validate_step
    def set_detector_efficiency(self, corrector):
        """
            Set the step that will apply the detector efficiency correction
            @param corrector: Workflow algorithm object
        """
        self._detector_efficiency_corrector = corrector

    @validate_step
    def set_background_subtractor(self, subtractor):
        """
            Set the step that will remove a background
            @param subtractor: Workflow algorithm object
        """
        self._background_subtractor = subtractor

    @validate_step
    def set_masker(self, masker):
        """
            Set the step that will apply the mask
            @param masker: Workflow algorithm object
        """
        self._masker = masker

    @validate_step
    def set_nxspe_saver(self, saver):
        """
            Set the algorithm that will save the NXSPE file
            @param saver: Workflow algorithm object
        """
        self._nxspe_saver = saver

    @validate_step
    def set_spe_saver(self, saver):
        """
            Set the algorithm that will save the SPE file
            @param saver: Workflow algorithm object
        """
        self._spe_saver = saver

    @validate_step
    def set_nexus_saver(self, saver):
        """
            Set the algorithm that will save the (Processed) NeXus file
            @param saver: Workflow algorithm object
        """
        self._nexus_saver = saver



    def pre_process(self):
        """
            Create the list of algorithms that will be run.
        """
        if self._data_loader is not None:
            self.append_step(self._data_loader)

        if self._offset_banks is not None:
            self.append_step(self._offset_banks)

        if self._ei_calculator is not None:
            self.append_step(self._ei_calculator)

        if self._normalise is not None:
            self.append_step(self._normalise)




#    def _absolute_norm_steps(self):
#        """
#            Creates a list of steps for each data set in the
#            processing queue in order to calculate the absolute units.
#        """


if __name__ == '__main__':
    # Instantiate the Reducer object
    r = InelasticReducer()
    r.set_loader(InelasticLoader)
    r.set_ei_calculator(InelasticFixEi, Ei=20.0)

    r.reduce()
