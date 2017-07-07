from __future__ import (absolute_import, division, print_function)
from abc import (ABCMeta, abstractmethod)
from six import with_metaclass
from math import sqrt
from sans.common.constants import EMPTY_NAME
from sans.common.enums import (SANSInstrument)
from sans.common.general_functions import create_unmanaged_algorithm


# ----------------------------------------------------------------------------------
# Free Functions
# ----------------------------------------------------------------------------------
def load_sigma_moderator_workspace(file_name):
    """
    Gets the sigma moderator workspace.
    :param file_name: the file name of the sigma moderator
    :returns the sigma moderator workspace
    """
    load_name = "LoadRKH"
    load_option = {"Filename": file_name,
                   "OutputWorkspace": EMPTY_NAME,
                   "FirstColumnValue": "Wavelength"}
    load_alg = create_unmanaged_algorithm(load_name, **load_option)
    load_alg.execute()
    moderator_workspace = load_alg.getProperty("OutputWorkspace").value

    convert_name = "ConvertToHistogram"
    convert_options = {"InputWorkspace": moderator_workspace,
                       "OutputWorkspace": EMPTY_NAME}
    convert_alg = create_unmanaged_algorithm(convert_name, **convert_options)
    convert_alg.execute()
    return convert_alg.getProperty("OutputWorkspace").value


def get_aperture_diameters(convert_to_q):
    """
    Gets the aperture diameters for the sample and the source
    If all fields are specified for a rectangular aperture then this is used, else a circular aperture is
    used.
    :param convert_to_q: a SANSStateConvertToQ object.
    :return: aperture diameter for the source, aperture diameter for the sample
    """
    def set_up_diameter(height, width):
        """
        Prepare the diameter parameter. If there are corresponding H and W values, then
        use them instead. Richard provided the formula: A = 2*sqrt((H^2 + W^2)/6)
        """
        return 2*sqrt((height*height+width*width)/6)
    h1 = convert_to_q.q_resolution_h1
    h2 = convert_to_q.q_resolution_h2
    w1 = convert_to_q.q_resolution_w1
    w2 = convert_to_q.q_resolution_w2

    if all(element is not None for element in [h1, h2, w1, w2]):
        a1 = set_up_diameter(h1, w1)
        a2 = set_up_diameter(h2, w2)
    else:
        a1 = convert_to_q.q_resolution_a1
        a2 = convert_to_q.q_resolution_a2

    return a1, a2


def create_q_resolution_workspace(convert_to_q, data_workspace):
    """
    Provides a q resolution workspace
    :param convert_to_q: a SANSStateConvertToQ object.
    :param data_workspace: the workspace which is to be reduced.
    :return: a q resolution workspace
    """
    # Load the sigma moderator
    file_name = convert_to_q.moderator_file
    sigma_moderator = load_sigma_moderator_workspace(file_name)

    # Get the aperture diameters
    a1, a2 = get_aperture_diameters(convert_to_q)

    # We need the radius, not the diameter in the TOFSANSResolutionByPixel algorithm
    sample_radius = 0.5 * a2
    source_radius = 0.5 * a1

    # The radii and the deltaR are expected to be in mm
    sample_radius *= 1000.
    source_radius *= 1000.
    delta_r = convert_to_q.q_resolution_delta_r
    delta_r *= 1000.

    collimation_length = convert_to_q.q_resolution_collimation_length
    use_gravity = convert_to_q.use_gravity
    gravity_extra_length = convert_to_q.gravity_extra_length

    resolution_name = "TOFSANSResolutionByPixel"
    resolution_options = {"InputWorkspace": data_workspace,
                          "OutputWorkspace": EMPTY_NAME,
                          "DeltaR": delta_r,
                          "SampleApertureRadius": sample_radius,
                          "SourceApertureRadius": source_radius,
                          "SigmaModerator": sigma_moderator,
                          "CollimationLength": collimation_length,
                          "AccountForGravity": use_gravity,
                          "ExtraLength": gravity_extra_length}
    resolution_alg = create_unmanaged_algorithm(resolution_name, **resolution_options)
    resolution_alg.execute()
    return resolution_alg.getProperty("OutputWorkspace").value


# ----------------------------------------------------------------------------------
# QResolution Classes
# ----------------------------------------------------------------------------------
class QResolutionCalculator(with_metaclass(ABCMeta, object)):
    def __init__(self):
        super(QResolutionCalculator, self).__init__()

    @abstractmethod
    def get_q_resolution_workspace(self, convert_to_q_info, data_workspace):
        """
        Calculates the q resolution workspace which is required for the Q1D algorithm
        :param convert_to_q_info: a SANSStateConvertToQ object
        :param data_workspace: the workspace which is being reduced.
        :return: a q resolution workspace or None
        """
        pass


class NullQResolutionCalculator(QResolutionCalculator):
    def __init__(self):
        super(QResolutionCalculator, self).__init__()

    def get_q_resolution_workspace(self, convert_to_q_info, data_workspace):
        return None


class QResolutionCalculatorISIS(QResolutionCalculator):
    def __init__(self):
        super(QResolutionCalculatorISIS, self).__init__()

    def get_q_resolution_workspace(self, convert_to_q_info, data_workspace):
        return create_q_resolution_workspace(convert_to_q_info, data_workspace)


class QResolutionCalculatorFactory(object):
    def __init__(self):
        super(QResolutionCalculatorFactory, self).__init__()

    @staticmethod
    def create_q_resolution_calculator(state):
        data = state.data
        instrument = data.instrument
        is_isis_instrument = instrument is SANSInstrument.LARMOR or instrument is SANSInstrument.SANS2D or\
                             instrument is SANSInstrument.LOQ  # noqa
        if is_isis_instrument:
            convert_to_q = state.convert_to_q
            if convert_to_q.use_q_resolution:
                q_resolution = QResolutionCalculatorISIS()
            else:
                q_resolution = NullQResolutionCalculator()
        else:
            raise RuntimeError("QResolutionCalculatorFactory: Other instruments are not implemented yet.")
        return q_resolution
