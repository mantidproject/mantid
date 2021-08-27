from __future__ import (absolute_import, division, print_function)

from mantid.simpleapi import *
import matplotlib.pyplot as plt

# Load incident flux
Load(
    Filename='fluxSmoothedNOM161959.nxs',
    OutputWorkspace='influx',
)
CropWorkspace(InputWorkspace='influx', OutputWorkspace='influx', XMin=0.1, XMax=2.9)

# Load 6 summed spectrum from NOM as input workspace
Load(Filename='inputwsNOM_164109', OutputWorkspace='NOM_164109')
SetSampleMaterial(InputWorkspace='NOM_164109', ChemicalFormula='Cs-Cl', SampleMassDensity=3.99)
CalculatePlaczek(
    InputWorkspace="NOM_164109",
    IncidentSpectra="influx",
    LambdaD=1.44,
    Order=1,
    SampleTemperature=943.15,
    ScaleByPackingFraction=False,
    CrystalDensity=0.01,
    OutputWorkspace="NOM_P1",
)

# plot
outws = mtd["NOM_P1"]
x = np.array(outws.readX(0))
fig, ax = plt.subplots(1)
for i in range(6):
    y = np.array(outws.readY(i))
    ax.plot(x, y, label=f"P1_Spec{i}")
ax.set_ylabel("P1 Factor")
