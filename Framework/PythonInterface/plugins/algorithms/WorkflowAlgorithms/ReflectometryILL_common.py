# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.geometry import Instrument
from mantid.api import Run, MatrixWorkspace
from typing import Union
import h5py
import numpy


# constants for summation type options
SUM_IN_LAMBDA = "SumInLambda"
SUM_IN_Q = "SumInQ"


def chopper_opening_angle(sample_logs: Run, instrument: Instrument) -> float:
    """Return the chopper opening angle in degrees.

    Keyword arguments:
    sample_logs -- run object holding workspace metadata
    instrument -- instrument object holding instrument parameters
    """
    instr_name = instrument.getName()
    if instr_name == "D17":
        duration = sample_logs.getProperty("duration").value
        if duration > 30.0:
            chopper1_phase_name = instrument.getStringParameter("chopper1_phase")[0]
            chopper2_phase_name = instrument.getStringParameter("chopper2_phase")[0]
        else:
            chopper1_phase_name = instrument.getStringParameter("chopper1_phase_alt")[0]
            chopper2_phase_name = instrument.getStringParameter("chopper2_phase_alt")[0]
        chopper1_phase = sample_logs.getProperty(chopper1_phase_name).value
        chopper_window = sample_logs.getProperty("ChopperWindow").value
        if chopper1_phase > 360.0:
            # Workaround for broken old D17 NeXus files.
            chopper1_phase = sample_logs.getProperty("VirtualChopper.chopper2_speed_average").value
        chopper2_phase = sample_logs.getProperty(chopper2_phase_name).value
        open_offset = sample_logs.getProperty("VirtualChopper.open_offset").value
        return chopper_window - (chopper2_phase - chopper1_phase) - open_offset
    else:
        first_chopper = int(sample_logs.getProperty("ChopperSetting.firstChopper").value)
        secondChopper = int(sample_logs.getProperty("ChopperSetting.secondChopper").value)
        phase1_entry = "CH{}.phase".format(first_chopper)
        phase2_entry = "CH{}.phase".format(secondChopper)
        if sample_logs.hasProperty(phase1_entry):
            chopper1_phase = sample_logs.getProperty(phase1_entry).value
        else:
            phase_entry = "chopper{}.phase".format(first_chopper)
            chopper1_phase = sample_logs.getProperty(phase_entry).value
        if sample_logs.hasProperty(phase2_entry):
            chopper2_phase = sample_logs.getProperty(phase2_entry).value
        else:
            phase_entry = "chopper{}.phase".format(secondChopper)
            chopper2_phase = sample_logs.getProperty(phase_entry).value
        if chopper1_phase > 360.0:
            # CH1.phase on FIGARO is set to an arbitrary value (999.9)
            chopper1_phase = 0.0
        if sample_logs.hasProperty("CollAngle.open_offset"):
            open_offset = sample_logs.getProperty("CollAngle.open_offset").value
        else:
            open_offset = sample_logs.getProperty("CollAngle.openOffset").value
        return 45.0 - (chopper2_phase - chopper1_phase) - open_offset


def chopper_pair_distance(sample_logs: Run, instrument: Instrument) -> float:
    """Return the gap distance in metres between the two choppers.

    Keyword arguments:
    sample_logs -- run object holding workspace metadata
    instrument -- instrument object holding instrument parameters
    """
    instr_name = instrument.getName()
    if instr_name == "D17":
        # in [m], enforced by the loader
        return sample_logs.getProperty("Distance.ChopperGap").value
    else:
        return sample_logs.getProperty("ChopperSetting.distSeparationChopperPair").value * 1e-3


def chopper_speed(sample_logs: Run, instrument: Instrument) -> float:
    """Return the chopper speed.

    Keyword arguments:
    sample_logs -- run object holding workspace metadata
    instrument -- instrument object holding instrument parameters
    """
    instr_name = instrument.getName()
    if instr_name == "D17":
        duration = sample_logs.getProperty("duration").value
        if duration > 30.0:  # for long durations, chopper speed average is reliable, otherwise rotation speed is used
            chopper1_speed_name = instrument.getStringParameter("chopper1_speed")[0]
        else:
            chopper1_speed_name = instrument.getStringParameter("chopper1_speed_alt")[0]
        return sample_logs.getProperty(chopper1_speed_name).value
    else:
        first_chopper = int(sample_logs.getProperty("ChopperSetting.firstChopper").value)
        speed_entry = "CH{}.rotation_speed".format(first_chopper)
        if sample_logs.hasProperty(speed_entry):
            return sample_logs.getProperty(speed_entry).value
        speed_entry = "chopper{}.rotation_speed".format(first_chopper)
        if sample_logs.hasProperty(speed_entry):
            return sample_logs.getProperty(speed_entry).value


def deflection_angle(sample_logs: Run) -> float:
    """Return the deflection angle in degree.

    Keyword arguments:
    sample_logs -- run object holding workspace metadata
    """
    if sample_logs.hasProperty("CollAngle.actual_coll_angle"):
        # Must be FIGARO
        return sample_logs.getProperty("CollAngle.actual_coll_angle").value
    else:
        return 0.0


def detector_angle(run: Union[str, list]) -> float:
    """Return the detector angle in degrees .Throws a runtime error if the metadata with detector angle is not found.

    Keyword arguments:
    run -- numor string or list of numor strings
    """
    if isinstance(run, list):
        run = run[0]
    try:
        with h5py.File(run, "r") as nexus:
            if nexus.get("entry0/instrument/DAN") is not None:
                return float(numpy.array(nexus.get("entry0/instrument/DAN/value"), dtype="float"))
            elif nexus.get("entry0/instrument/dan") is not None:
                return float(numpy.array(nexus.get("entry0/instrument/dan/value"), dtype="float"))
            elif nexus.get("entry0/instrument/VirtualAxis/DAN_actual_angle") is not None:
                return float(numpy.array(nexus.get("entry0/instrument/VirtualAxis/DAN_actual_angle"), dtype="float"))
            else:
                raise RuntimeError("Cannot retrieve detector angle from Nexus file {}.".format(run))
    except OSError:
        raise RuntimeError("Cannot load file {}.".format(run))


def detector_resolution() -> float:
    """Return the detector resolution in mm."""
    return 0.0022


def instrument_name(ws: MatrixWorkspace) -> str:
    """Return the instrument's name validating it is either D17 or FIGARO.

    Keyword arguments:
    ws -- workspace object with defined instrument
    """
    name = ws.getInstrument().getName()
    if name != "D17" and name != "FIGARO":
        raise RuntimeError("Unrecognized instrument {}. Only D17 and FIGARO are supported.".format(name))
    return name


def pixel_size(instr_name: str) -> float:
    """Return the pixel size in mm.

    Keyword arguments:
    instr_name -- instrument name
    """
    return 0.001195 if instr_name == "D17" else 0.0012


def sample_angle(run: Union[str, list]) -> float:
    """Return the sample theta angle in degrees. Throws a runtime error if the metadata with sample angle is not found.

    Keyword arguments:
    run -- numor string or list of numor strings
    """
    if isinstance(run, list):
        run = run[0]
    try:
        with h5py.File(run, "r") as nexus:
            if nexus.get("entry0/instrument/SAN") is not None:
                return float(numpy.array(nexus.get("entry0/instrument/SAN/value"), dtype="float"))
            elif nexus.get("entry0/instrument/san") is not None:
                return float(numpy.array(nexus.get("entry0/instrument/san/value"), dtype="float"))
            else:
                raise RuntimeError("Cannot retrieve sample angle from Nexus file {}.".format(run))
    except OSError:
        raise RuntimeError("Cannot load file {}.".format(run))


def slit_size_log_entry(instr_name: str, slit_number: int) -> str:
    """Return the sample log entry which contains the slit size for the given slit.

    Keyword arguments:
    instr_name -- instrument name
    slit_number -- id of the slit, either 1 or 2
    """
    if slit_number not in [1, 2]:
        raise RuntimeError("Slit number out of range.")
    entry = "VirtualSlitAxis.s{}w_actual_width" if instr_name == "D17" else "VirtualSlitAxis.S{}H_actual_height"
    return entry.format(slit_number + 1)


def slit_sizes(ws: MatrixWorkspace) -> None:
    """Sets slit sizes with appropriate units to sample logs.

    Keyword arguments:
    ws -- workspace object from which metadata is extracted
    """
    run = ws.run()
    instr_name = instrument_name(ws)
    slit2_width = run.get(slit_size_log_entry(instr_name, 1))
    slit3_width = run.get(slit_size_log_entry(instr_name, 2))
    if slit2_width is None or slit3_width is None:
        run.addProperty(SampleLogs.SLIT2WIDTH, str("-"), "", True)
        run.addProperty(SampleLogs.SLIT3WIDTH, str("-"), "", True)
    else:
        slit2_width_unit = slit2_width.units
        slit3_width_unit = slit3_width.units
        slit3w = slit3_width.value
        if instr_name != "D17":
            bgs3 = float(run.getProperty("BGS3.value").value)
            if bgs3 >= 150.0:
                slit3w += 0.08
            elif 150.0 > bgs3 >= 50.0:
                slit3w += 0.06
            elif -50.0 > bgs3 >= -150.0:
                slit3w -= 0.12
            elif bgs3 < -150.0:
                slit3w -= 0.24
        slit2w = slit2_width.value
        run.addProperty(SampleLogs.SLIT2WIDTH, float(slit2w), slit2_width_unit, True)
        run.addProperty(SampleLogs.SLIT3WIDTH, float(slit3w), slit3_width_unit, True)


class SampleLogs:
    FOREGROUND_CENTRE = "reduction.foreground.centre_workspace_index"
    FOREGROUND_END = "reduction.foreground.last_workspace_index"
    FOREGROUND_START = "reduction.foreground.first_workspace_index"
    LINE_POSITION = "reduction.line_position"
    SLIT2WIDTH = "reduction.slit2width"
    SLIT3WIDTH = "reduction.slit3width"
    SUM_TYPE = "reduction.foreground.summation_type"
