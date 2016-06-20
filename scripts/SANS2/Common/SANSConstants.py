
class SANSConstants(object):
    monitor_suffix = "_monitors"
    input_workspace = "InputWorkspace"

    file_name = "Filename"

    output_workspace = "OutputWorkspace"
    output_workspace_group = output_workspace + "_"

    output_monitor_workspace = "MonitorWorkspace"
    output_monitor_workspace_group = output_monitor_workspace + "_"

    sans_suffix = "sans"
    trans_suffix = "trans"

    high_angle_bank = "HAB"
    low_angle_bank = "LAB"

    sans2d = "SANS2D"
    larmor = "LARMOR"
    loq = "LOQ"

    class Calibration(object):
        calibration_workspace_tag = "sans_applied_calibration_file"


class SANSInstrument(object):
    class LOQ(object):
        pass

    class LARMOR(object):
        pass

    class SANS2D(object):
        pass

    class NoInstrument(object):
        pass


class SANSFacility(object):
    class ISIS(object):
        pass

    class NoFacility(object):
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


class CanonicalCoordinates(object):
    class X(Coordinates):
        pass

    class Y(Coordinates):
        pass

    class Z(Coordinates):
        pass
