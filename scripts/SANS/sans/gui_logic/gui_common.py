from sans.common.enums import SANSInstrument, ISISReductionMode


# ----------------------------------------------------------------------------------------------------------------------
# Option column globals
# ----------------------------------------------------------------------------------------------------------------------
OPTIONS_INDEX = 7
OPTIONS_SEPARATOR = ","
OPTIONS_EQUAL = "="
HIDDEN_OPTIONS_INDEX = 8

# ----------------------------------------------------------------------------------------------------------------------
#  Other Globals
# ----------------------------------------------------------------------------------------------------------------------
SANS2D_LAB = "rear"
SANS2D_HAB = "front"

LOQ_LAB = "main-detector"
LOQ_HAB = "Hab"

LARMOR_LAB = "DetectorBench"

DEFAULT_LAB = ISISReductionMode.to_string(ISISReductionMode.LAB)
DEFAULT_HAB = ISISReductionMode.to_string(ISISReductionMode.HAB)
MERGED = "Merged"
ALL = "All"


def get_reduction_mode_strings_for_gui(instrument=None):
    if instrument is SANSInstrument.SANS2D:
        return [SANS2D_LAB, SANS2D_HAB, MERGED, ALL]
    elif instrument is SANSInstrument.LOQ:
        return [LOQ_LAB, LOQ_HAB, MERGED, ALL]
    elif instrument is SANSInstrument.LARMOR:
        return [LARMOR_LAB]
    else:
        return [DEFAULT_LAB, DEFAULT_HAB, MERGED, ALL]


def get_reduction_selection(instrument):
    selection = {ISISReductionMode.Merged: MERGED,
                 ISISReductionMode.All: ALL}
    if instrument is SANSInstrument.SANS2D:
        selection.update({ISISReductionMode.LAB: SANS2D_LAB,
                          ISISReductionMode.HAB: SANS2D_HAB})
    elif instrument is SANSInstrument.LOQ:
        selection.update({ISISReductionMode.LAB: LOQ_LAB,
                          ISISReductionMode.HAB: LOQ_HAB})
    elif instrument is SANSInstrument.LARMOR:
        selection = {ISISReductionMode.LAB: LARMOR_LAB}
    else:
        selection.update({ISISReductionMode.LAB: DEFAULT_LAB,
                          ISISReductionMode.HAB: DEFAULT_HAB})
    return selection


def get_string_for_gui_from_reduction_mode(reduction_mode, instrument):
    reduction_selection = get_reduction_selection(instrument)
    if reduction_selection and reduction_mode in list(reduction_selection.keys()):
        return reduction_selection[reduction_mode]
    else:
        return None


def get_reduction_mode_from_gui_selection(gui_selection):
    if gui_selection == MERGED:
        return ISISReductionMode.Merged
    elif gui_selection == ALL:
        return ISISReductionMode.All
    elif gui_selection == SANS2D_LAB or gui_selection == LOQ_LAB or gui_selection == LARMOR_LAB or gui_selection == DEFAULT_LAB:  # noqa
        return ISISReductionMode.LAB
    elif gui_selection == SANS2D_HAB or gui_selection == LOQ_HAB:
        return ISISReductionMode.HAB
    else:
        raise RuntimeError("Reduction mode selection is not valid.")
