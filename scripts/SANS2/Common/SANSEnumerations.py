# pylint: disable=too-few-public-methods, invalid-name

from SANS2.Common.SANSConstants import SANSConstants


# -------------------------------------------
# Strongly Typed enum class decorator
# -------------------------------------------
def inner_classes_with_name_space(*inner_classes):
    """
    Class decorator which changes the name of an inner class to include the name of the outer class. The inner class
    gets a method to determine the name of the outer class. This information is needed for serialization at the
    algorithm input boundary
    """
    def inner_class_builder(cls):
        # Add each inner class to the outer class
        for inner_class in inner_classes:
            new_class = type(inner_class, (cls, ), {"outer_class_name": cls.__name__})
            # We set the module of the inner class to the module of the outer class. We have to do this since we
            # are dynamically adding the inner class which gets its module name from the module where it was added,
            # but not where the outer class lives.
            module_of_outer_class = getattr(cls, "__module__")
            setattr(new_class, "__module__", module_of_outer_class)
            # Add the inner class to the outer class
            setattr(cls, inner_class, new_class)
        return cls
    return inner_class_builder


@inner_classes_with_name_space("LOQ", "LARMOR", "SANS2D", "NoInstrument")
class SANSInstrument(object):
    pass


@inner_classes_with_name_space("ISIS", "NoFacility")
class SANSFacility(object):
    pass


def convert_string_to_sans_instrument(to_convert):
    to_convert_cap = to_convert.upper()
    if to_convert_cap == SANSConstants.sans2d:
        selected_instrument = SANSInstrument.SANS2D
    elif to_convert_cap == SANSConstants.loq:
        selected_instrument = SANSInstrument.LOQ
    elif to_convert_cap == SANSConstants.larmor:
        selected_instrument = SANSInstrument.LARMOR
    else:
        selected_instrument = SANSInstrument.NoInstrument
    return selected_instrument


def convert_sans_instrument_to_string(to_convert):
    if to_convert is SANSInstrument.SANS2D:
        selected_instrument = SANSConstants.sans2d
    elif to_convert is SANSInstrument.LOQ:
        selected_instrument = SANSConstants.loq
    elif to_convert == SANSInstrument.LARMOR:
        selected_instrument = SANSConstants.larmor
    else:
        selected_instrument = ""
    return selected_instrument


# --------------------------
#  Coordinate Definitions (3D)
# --------------------------

class Coordinates(object):
    pass


@inner_classes_with_name_space("X", "Y", "Z")
class CanonicalCoordinates(Coordinates):
    pass


# --------------------------
#  Reductions
# --------------------------
class ReductionType(object):
    pass


@inner_classes_with_name_space("Hab", "Lab", "Merged", "Both")
class SANSReductionType(ReductionType):
    pass


@inner_classes_with_name_space("OneDim", "TwoDim")
class ReductionDimensionality(object):
    pass
