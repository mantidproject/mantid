# import mantid algorithms, numpy and matplotlib
from mantid.api import AlgorithmManager, AnalysisDataService as ADS
import numpy as np
from mantid.kernel import UnitConversion, DeltaEModeType, UnitParams, UnitParametersMap
from typing import Annotated, TypeAlias, Tuple, Sequence

# Forward declarations
Workspace2D: TypeAlias = Annotated[type, "Workspace2D"]


def exec_alg(alg_name: str, **kwargs):
    alg = AlgorithmManager.create(alg_name)
    alg.initialize()
    alg.setAlwaysStoreInADS(True)
    alg.setLogging(True)
    alg.setProperties(kwargs)
    alg.execute()
    out_props = tuple(
        ADS.retrieve(alg.getPropertyValue(prop)) if prop == "OutputWorkspace" else alg.getProperty(prop).value
        for prop in alg.outputProperties()
    )
    return out_props[0] if len(out_props) == 1 else out_props


def load_poldi(
    fpath_data: str,
    fpath_idf: str,
    xmax: float = 3000,
    chopper_speed: float = 5000,
    t0: float = 0.025701,
    t0_const: float = 85.0,
    output_workspace: str = "ws",
):
    """
    Function to load POLDI v2 data from ASCII file containing array of counts
    :param fpath_data: filepath of ASCII file with counts
    :param fpath_idf: filepath to instrument definition file
    :param xmax: maximum x-value (mus) recorded in file (x-axis is arrival time modulo cycle period)
    :param chopper_speed: chopper speed used on instrument (rpm)
    :param t0: time offset to applied to slit openings (mus)
    :param t0_const: TZERO diffractometer constant for instrument
    :param output_workspace: output workspace name
    :return ws: workspace containing POLDI data
    """
    ws = exec_alg("LoadEmptyInstrument", Filename=fpath_idf, OutputWorkspace=output_workspace)
    dat = np.loadtxt(fpath_data)
    bin_width = xmax / dat.shape[-1]
    ws = exec_alg("Rebin", InputWorkspace=ws, Params=f"0,{bin_width},{xmax}", OutputWorkspace=ws.name())
    for ispec in range(ws.getNumberHistograms()):
        ws.setY(ispec, dat[ispec, :])
    ws = exec_alg("ConvertToPointData", InputWorkspace=ws, OutputWorkspace=ws.name())
    # add some logs (will eventually be set in file)
    exec_alg("AddSampleLog", Workspace=ws.name(), LogName="chopperspeed", LogText=str(chopper_speed), LogType="Number")
    # set t0 (would normally live in parameter file)
    exec_alg(
        "SetInstrumentParameter", Workspace=ws.name(), ComponentName="chopper", ParameterName="t0", ParameterType="Number", Value=str(t0)
    )
    exec_alg(
        "SetInstrumentParameter",
        Workspace=ws.name(),
        ComponentName="chopper",
        ParameterName="t0_const",
        ParameterType="Number",
        Value=str(t0_const),
    )
    return ws


def get_instrument_settings_from_log(ws: Workspace2D) -> Tuple[float, np.ndarray[float], float, float]:
    """
    Function to get instrument settings from logs stored on workspace
    :param ws: POLDI workspace
    :return cycle_time: Period of cycle (mus)
    :return slit_offsets: array of slit offsets in time (mus)
    :return t0_const: TZERO diffractometer constant for instrument
    :return l1_chop: path length from source to chopper (m)
    """
    inst = ws.getInstrument()
    source = inst.getSource()  # might not need these
    chopper = inst.getComponentByName("chopper")
    l1_chop = (chopper.getPos() - source.getPos()).norm()
    t0 = chopper.getNumberParameter("t0")[0]
    t0_const = chopper.getNumberParameter("t0_const")[0]
    chopper_speed = ws.run().getPropertyAsSingleValue("chopperspeed")  # rpm
    # chopper has 32 slits, 8 slits per section (4*8 - quarter repeated 4 times).
    cycle_time = 60.0 / (4.0 * chopper_speed) * 1.0e6  # mus
    # get chopper offsets in time
    nslits = chopper.nelements()
    slit_offsets = np.array([chopper[islit].getPos()[0] * cycle_time + t0 for islit in range(nslits)])
    return cycle_time, slit_offsets, t0_const, l1_chop


def get_dspac_limits(tth_min: float, tth_max: float, lambda_min: float, lambda_max: float) -> Tuple[float, float]:
    """
    Function to calculate min and max d-spacing to consider in auto-correlation given two-theta coverage and wavelength range.
    :param tth_min: minimum two theta to consider
    :param tth_max: maximum two theta to consider
    :param lambda_min: minimum wavelength to consider
    :param lambda_max: maximum wavelength to consider
    :return (dspac_min, dspac_max): min and max d-spacing
    """
    dspac_min = lambda_min / (2 * np.sin(tth_max / 2))
    dspac_max = lambda_max / (2 * np.sin(tth_min / 2))
    return dspac_min, dspac_max


def get_final_dspac_array(bin_width: float, dspac_min: float, dspac_max: float, time_max: float) -> np.ndarray[float]:
    """
    Function to d-spacing bins given time bin width and maximum arrival time
    :param bin_width: bin width in time (mus) in workspace
    :param dspac_min: minimum d-spacing to consider
    :param dspac_max: maximum d-spacing to consider
    :param time_max: maximum wavelength to consider
    :return np.ndarray: array of d-spacing bins
    """
    delta_d = dspac_max * bin_width / time_max
    nbins_dspac = int((dspac_max - dspac_min) / delta_d)  # 2435
    return np.linspace(dspac_min, dspac_max, nbins_dspac)


def get_max_tof_from_chopper(l1: float, l1_chop: float, l2s: Sequence[float], tths: Sequence[float], lambda_max: float) -> float:
    """
    Function to calculate TOF of longest wavelength neutron to reach detector with the largest total path
    :param l1: path length from source to sample (m)
    :param l1_chop: path length from source to chopper (m)
    :param l2s: list of path lengths from sample to each detector
    :param tths: list of two-theta for each detector
    :param lambda_max: maximum wavelength (Ang) to consider
    :param offset: time offset corresponding to first slit opening
    :return time_max: arrival time (mus) of longest wavelength neutron from first slit opening
    """
    imax = np.argmax(l2s)
    params = UnitParametersMap()
    params[UnitParams.l2] = l2s[imax]
    params[UnitParams.twoTheta] = tths[imax]
    # calc tof of longest wavelength neutron to reach detector with the largest total path
    time_max = UnitConversion.run("Wavelength", "TOF", lambda_max, l1 - l1_chop, DeltaEModeType.Elastic, params)
    return time_max
