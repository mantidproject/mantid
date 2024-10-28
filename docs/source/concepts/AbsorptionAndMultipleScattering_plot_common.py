from mantid.api import mtd
from mantid.simpleapi import (
    AbsorptionCorrection,
    CalculateCarpenterSampleCorrection,
    CarpenterSampleCorrection,
    ConvertUnits,
    CreateSampleWorkspace,
    CreateWorkspace,
    DiscusMultipleScatteringCorrection,
    Divide,
    MayersSampleCorrection,
    MonteCarloAbsorption,
    MultipleScatteringCorrection,
    SetSample,
)
import matplotlib.pyplot as plt

LINE_COLORS = ("blue", "orange", "green", "red", "purple")
ABS_LINE_LABELS = ("Uncorrected", "Mayers", "Carpenter", "Numerical Integration", "Monte Carlo")
MS_LINE_LABELS = ("Uncorrected", "Mayers", "Carpenter", "Numerical Integration", "Discus")


def make_sample_workspace(material, num_density, radius, height):
    # Create a fake workspace with TOF data
    sample_ws = CreateSampleWorkspace(Function="Powder Diffraction", NumBanks=1, BankPixelWidth=1, XUnit="TOF", XMin=1000, XMax=10000)
    # set the sample information
    SetSample(
        InputWorkspace=sample_ws,
        Geometry={"Shape": "Cylinder", "Center": [0.0, 0.0, 0.0], "Height": float(height), "Radius": float(radius)},
        Material={"ChemicalFormula": material, "SampleNumberDensity": num_density},
    )
    return sample_ws


def get_mayers_results(sample_ws):
    # CalculateMayersSampleCorrections
    ConvertUnits(InputWorkspace=sample_ws, OutputWorkspace=sample_ws, Target="TOF")
    mayers_abs = MayersSampleCorrection(sample_ws, MultipleScattering=False)
    mayers_ms = MayersSampleCorrection(sample_ws, MultipleScattering=True)
    mayers_abs_corr = mayers_abs / sample_ws
    mayers_ms_corr = mayers_ms / mayers_abs  # ms result / abs result = 1-ms
    return mayers_abs, mayers_ms, mayers_abs_corr, mayers_ms_corr


def get_carpenter_results(sample_ws, radius):
    ConvertUnits(InputWorkspace=sample_ws, OutputWorkspace=sample_ws, Target="Wavelength")
    corrections = CalculateCarpenterSampleCorrection(InputWorkspace=sample_ws, CylinderSampleRadius=radius)
    absCorr = corrections.getItem(0)
    msCorr = corrections.getItem(1)
    carpenter_abs = Divide(sample_ws, absCorr)
    absCorr = 1.0 / absCorr  # plots are of 1/correction
    carpenter_ms = CarpenterSampleCorrection(sample_ws, CylinderSampleRadius=radius)
    msCorr = carpenter_ms / carpenter_abs  # makes things look like Mayers' function
    return carpenter_abs, carpenter_ms, absCorr, msCorr


def get_numerical_results(sample_ws):
    ConvertUnits(InputWorkspace=sample_ws, OutputWorkspace=sample_ws, Target="Wavelength")
    numerical_abs_corr = AbsorptionCorrection(sample_ws, OutputWorkspace="numerical_abs_corr")
    numerical_abs = sample_ws / numerical_abs_corr
    numerical_abs.setDistribution(False)
    numerical_abs_corr = 1.0 / numerical_abs_corr  # plots are of 1/correction
    corrections = MultipleScatteringCorrection(sample_ws)
    numerical_ms_corr = corrections.getItem(0)
    numerical_ms = sample_ws * numerical_ms_corr / numerical_abs_corr
    numerical_ms.setYUnit(numerical_abs.YUnit())
    numerical_ms.setDistribution(False)
    numerical_ms = numerical_abs - numerical_ms
    numerical_ms_corr = numerical_ms / numerical_abs

    return numerical_abs, numerical_ms, numerical_abs_corr, numerical_ms_corr


def get_montecarlo_results(sample_ws):
    ConvertUnits(InputWorkspace=sample_ws, OutputWorkspace=sample_ws, Target="Wavelength")
    mc_abs_factor = MonteCarloAbsorption(sample_ws, OutputWorkspace="mc_abs_factor")
    mc_abs = sample_ws / mc_abs_factor
    mc_abs.setDistribution(False)
    # create flat S(Q) for vanadium
    X = [0, 10, 20, 30, 40]
    Y = [1, 1, 1, 1, 1]
    SQ = CreateWorkspace(DataX=X, DataY=Y, UnitX="MomentumTransfer")
    ConvertUnits(InputWorkspace=sample_ws, OutputWorkspace=sample_ws, Target="Momentum")
    DiscusMultipleScatteringCorrection(sample_ws, StructureFactorWorkspace=SQ, NumberScatterings=2, OutputWorkspace="mc_corrections")
    # apply ms correction using ratio method
    multi_scatt_ratio = mtd["mc_corrections_Scatter_1"] / (mtd["mc_corrections_Scatter_1"] + mtd["mc_corrections_Scatter_2"])
    mc_ms = sample_ws * multi_scatt_ratio
    # apply absorption correction after ms correction to get fully corrected result
    ConvertUnits(InputWorkspace=mc_ms, OutputWorkspace=mc_ms, Target="Wavelength")
    mc_ms = mc_ms / mc_abs_factor

    mc_abs_factor = 1.0 / mc_abs_factor
    multi_scatt_ratio = mc_ms / mc_abs

    return mc_abs, mc_ms, mc_abs_factor, multi_scatt_ratio


def plot_stuff(axis, workspaces, line_labels, wkspIndex=0):
    labels = line_labels[len(line_labels) - len(workspaces) :]
    colors = LINE_COLORS[len(LINE_COLORS) - len(workspaces) :]
    for wksp, label, color in zip(workspaces, labels, colors):
        axis.plot(wksp, wkspIndex=wkspIndex, label=label, color=color, normalize_by_bin_width=False)


# sample material
material = "V"
num_density = 0.07261
cyl_height_cm = 4
cyl_radius_cm = 0.25
# Set sample workspace with instrument
sample_ws = make_sample_workspace(material, num_density, cyl_radius_cm, cyl_height_cm)
# Set sample material
# sample_ws = cylinder_sample(sample_ws, cyl_radius_cm, cyl_height_cm)

# Calculate corrections
mayers_abs, mayers_ms, mayers_abs_corr, mayers_ms_corr = get_mayers_results(sample_ws)
carpenter_abs, carpenter_ms, carpenter_abs_corr, carpenter_ms_corr = get_carpenter_results(sample_ws, cyl_radius_cm)
numerical_abs, numerical_ms, numerical_abs_corr, numerical_ms_corr = get_numerical_results(sample_ws)
mc_abs, mc_ms, mc_abs_corr, mc_ms_corr = get_montecarlo_results(sample_ws)

# Get all wksps in wavelength
for name in [
    sample_ws,
    mayers_abs,
    mayers_ms,
    mayers_abs_corr,
    mayers_ms_corr,
    carpenter_abs,
    carpenter_ms,
    carpenter_abs_corr,
    carpenter_ms_corr,
    numerical_abs,
    numerical_ms,
    numerical_abs_corr,
    numerical_ms_corr,
    mc_abs,
    mc_ms,
    mc_abs_corr,
    mc_ms_corr,
]:
    ConvertUnits(InputWorkspace=str(name), OutputWorkspace=str(name), Target="Wavelength")


# Plot
def plot_abs():
    # Plot
    fig, ax = plt.subplots(2, subplot_kw={"projection": "mantid"}, sharex=True, gridspec_kw={"wspace": 0, "hspace": 0})
    plot_stuff(ax[0], (sample_ws, mayers_abs, carpenter_abs, numerical_abs, mc_abs), ABS_LINE_LABELS)
    ax[0].legend()
    plot_stuff(ax[1], (mayers_abs_corr, carpenter_abs_corr, numerical_abs_corr, mc_abs_corr), ABS_LINE_LABELS)
    ax[1].set_ylabel("1/A")
    # plt.show()


def plot_ms():
    fig, ax = plt.subplots(2, subplot_kw={"projection": "mantid"}, sharex=True, gridspec_kw={"wspace": 0, "hspace": 0})
    plot_stuff(ax[0], (sample_ws, mayers_ms, carpenter_ms, numerical_ms, mc_ms), MS_LINE_LABELS)
    ax[0].legend()
    plot_stuff(ax[1], (mayers_ms_corr, carpenter_ms_corr, numerical_ms_corr, mc_ms_corr), MS_LINE_LABELS)
    ax[1].set_ylabel(r"1-$\Delta$")
    # plt.show()
