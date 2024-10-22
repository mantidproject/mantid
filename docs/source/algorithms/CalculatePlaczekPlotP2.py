from mantid.api import mtd
from mantid.simpleapi import CalculatePlaczek, ConvertUnits, CropWorkspace, Load, SetSampleMaterial
import matplotlib.pyplot as plt
import numpy as np

# Load incident flux
Load(
    Filename="fluxSmoothedNOM161959.nxs",
    OutputWorkspace="influx",
)
CropWorkspace(InputWorkspace="influx", OutputWorkspace="influx", XMin=0.1, XMax=2.9)

# Load 6 summed spectrum from NOM as input workspace
Load(Filename="inputwsNOM_164109.nxs", OutputWorkspace="NOM_164109")
SetSampleMaterial(InputWorkspace="NOM_164109", ChemicalFormula="Cs-Cl", SampleMassDensity=3.99)
CalculatePlaczek(
    InputWorkspace="NOM_164109",
    IncidentSpectra="influx",
    LambdaD=1.44,
    Order=2,
    SampleTemperature=943.15,
    ScaleByPackingFraction=False,
    CrystalDensity=0.01,
    OutputWorkspace="NOM_P2",
)
# convert to q
ConvertUnits(InputWorkspace="NOM_P2", OutputWorkspace="NOM_P2", Target="MomentumTransfer")

# plot
outws = mtd["NOM_P2"]
x = np.array(outws.readX(0))
x = 0.5 * (x[1:] + x[:-1])
fig, ax1 = plt.subplots(1)
for i in range(6):
    y = np.array(outws.readY(i))
    ax1.plot(x, y, label=f"P1+P2:Spec{i}")
ax1.set_xlabel(r"q($\AA^{-1}$)")
ax1.set_xlim([0.1, 15])
ax1.legend()
ax1.set_ylabel("P2")
ax1.set_title("Full Placzek correction factor (P1+P2)")
