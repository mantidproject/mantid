from __future__ import (absolute_import, division, print_function)
import math
from abc import (ABCMeta, abstractmethod)
from six import (with_metaclass)
from sans.common.enums import (SANSInstrument, SANSFacility, SampleShape)
from sans.common.general_functions import create_unmanaged_algorithm
from sans.common.constants import EMPTY_NAME


class DivideByVolume(with_metaclass(ABCMeta, object)):
    def __init__(self):
        super(DivideByVolume, self).__init__()

    @abstractmethod
    def divide_by_volume(self, workspace, scale_info):
        pass


class NullDivideByVolume(DivideByVolume):
    def __init__(self):
        super(NullDivideByVolume, self).__init__()

    def divide_by_volume(self, workspace, scale_info):
        _ = scale_info  # noqa
        return workspace


class DivideByVolumeISIS(DivideByVolume):

    def __init__(self):
        super(DivideByVolumeISIS, self).__init__()

    def divide_by_volume(self, workspace, scale_info):
        volume = self._get_volume(scale_info)

        single_valued_name = "CreateSingleValuedWorkspace"
        single_valued_options = {"OutputWorkspace": EMPTY_NAME,
                                 "DataValue": volume}
        single_valued_alg = create_unmanaged_algorithm(single_valued_name, **single_valued_options)
        single_valued_alg.execute()
        single_valued_workspace = single_valued_alg.getProperty("OutputWorkspace").value

        divide_name = "Divide"
        divide_options = {"LHSWorkspace": workspace,
                          "RHSWorkspace": single_valued_workspace}
        divide_alg = create_unmanaged_algorithm(divide_name, **divide_options)
        divide_alg.setPropertyValue("OutputWorkspace", EMPTY_NAME)
        divide_alg.setProperty("OutputWorkspace", workspace)
        divide_alg.execute()
        return divide_alg.getProperty("OutputWorkspace").value

    def _get_volume(self, scale_info):
        thickness = scale_info.thickness if scale_info.thickness is not None else scale_info.thickness_from_file
        width = scale_info.width if scale_info.width is not None else scale_info.width_from_file
        height = scale_info.height if scale_info.height is not None else scale_info.height_from_file
        shape = scale_info.shape if scale_info.shape is not None else scale_info.shape_from_file

        # Now we calculate the volume
        if shape is SampleShape.Cylinder:
            # Volume = circle area * height
            # Factor of four comes from radius = width/2
            volume = height * math.pi
            volume *= math.pow(width, 2) / 4.0
        elif shape is SampleShape.FlatPlate:
            # Flat plate sample
            volume = width * height * thickness
        elif shape is SampleShape.Disc:
            # Factor of four comes from radius = width/2
            # Disc - where height is not used
            volume = thickness * math.pi
            volume *= math.pow(width, 2) / 4.0
        else:
            raise NotImplementedError('DivideByVolumeISIS: The shape {0} is not in the list of '
                                      'supported shapes'.format(shape))
        return volume


class DivideByVolumeFactory(object):
    def __init__(self):
        super(DivideByVolumeFactory, self).__init__()

    @staticmethod
    def create_divide_by_volume(state):
        data = state.data
        facility = data.facility

        if facility is SANSFacility.ISIS:
            divider = DivideByVolumeISIS()
        else:
            raise RuntimeError("DivideVolumeFactory: Other instruments are not implemented yet.")
        return divider


class MultiplyByAbsoluteScale(with_metaclass(ABCMeta, object)):
    DEFAULT_SCALING = 100.0

    def __init__(self):
        super(MultiplyByAbsoluteScale, self).__init__()

    @staticmethod
    def do_scale(workspace, scale_factor):
        single_valued_name = "CreateSingleValuedWorkspace"
        single_valued_options = {"OutputWorkspace": EMPTY_NAME,
                                 "DataValue": scale_factor}
        single_valued_alg = create_unmanaged_algorithm(single_valued_name, **single_valued_options)
        single_valued_alg.execute()
        single_valued_workspace = single_valued_alg.getProperty("OutputWorkspace").value

        multiply_name = "Multiply"
        multiply_options = {"LHSWorkspace": workspace,
                            "RHSWorkspace": single_valued_workspace}
        multiply_alg = create_unmanaged_algorithm(multiply_name, **multiply_options)
        multiply_alg.setPropertyValue("OutputWorkspace", EMPTY_NAME)
        multiply_alg.setProperty("OutputWorkspace", workspace)
        multiply_alg.execute()
        return multiply_alg.getProperty("OutputWorkspace").value

    @abstractmethod
    def multiply_by_absolute_scale(self, workspace, scale_info):
        pass


class MultiplyByAbsoluteScaleLOQ(MultiplyByAbsoluteScale):
    def __init__(self):
        super(MultiplyByAbsoluteScaleLOQ, self).__init__()

    def multiply_by_absolute_scale(self, workspace, scale_info):
        scale_factor = scale_info.scale*self.DEFAULT_SCALING if scale_info.scale is not None else self.DEFAULT_SCALING
        rescale_to_colette = math.pi
        scale_factor /= rescale_to_colette
        return self.do_scale(workspace, scale_factor)


class MultiplyByAbsoluteScaleISIS(MultiplyByAbsoluteScale):
    def __init__(self):
        super(MultiplyByAbsoluteScaleISIS, self).__init__()

    def multiply_by_absolute_scale(self, workspace, scale_info):
        scale_factor = scale_info.scale*self.DEFAULT_SCALING if scale_info.scale is not None else self.DEFAULT_SCALING
        return self.do_scale(workspace, scale_factor)


class MultiplyByAbsoluteScaleFactory(object):
    def __init__(self):
        super(MultiplyByAbsoluteScaleFactory, self).__init__()

    @staticmethod
    def create_multiply_by_absolute(state):
        data = state.data
        instrument = data.instrument
        facility = data.facility

        if instrument is SANSInstrument.LOQ:
            multiplier = MultiplyByAbsoluteScaleLOQ()
        elif facility is SANSFacility.ISIS:
            multiplier = MultiplyByAbsoluteScaleISIS()
        else:
            raise NotImplementedError("MultiplyByAbsoluteScaleFactory: Other instruments are not implemented yet.")
        return multiplier
