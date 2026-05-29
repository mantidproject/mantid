# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import numpy as np

from mantid.simpleapi import ConvertUnits, CopySample, MonteCarloAbsorption, RotateSampleShape
from mantid.kernel import logger
from Engineering.texture.correction.correction_model import read_attenuation_coefficient_at_value
from Engineering.texture.texture_helper import define_gauge_volume


class AbsorptionCalculator:
    """Runs MonteCarloAbsorption per orientation and stores the resulting transmission factors."""

    def __init__(self, model):
        self._model = model

    def calc_for_index(self, index):
        m = self._model
        wsm = m.workspaces
        mc_ws = ConvertUnits(InputWorkspace=wsm.wsname, Target="Wavelength", OutputWorkspace=wsm.WS_MC_INPUT)
        mc_ws.run().getGoniometer().setR(np.eye(3))
        CopySample(
            InputWorkspace=wsm.mesh_ws,
            OutputWorkspace=wsm.WS_MC_INPUT,
            CopyShape=True,
            CopyMaterial=True,
            CopyEnvironment=False,
            CopyLattice=False,
        )

        wsm.translate_shape(mc_ws, *wsm.offset)

        R = m.orientations[index].R

        shapeR = R * wsm.init_R
        rotvec = shapeR.as_rotvec(degrees=True)
        ang = np.linalg.norm(rotvec)
        if ang != 0:
            vec = rotvec / ang
            RotateSampleShape(wsm.WS_MC_INPUT, f"{ang},{vec[0]},{vec[1]},{vec[2]},1")

        define_gauge_volume(mc_ws, wsm.gauge_volume_str)
        try:
            bb = mc_ws.sample().getShape().getBoundingBox()
            gv_str = mc_ws.run().getProperty("GaugeVolume").value if mc_ws.run().hasProperty("GaugeVolume") else "<none>"
            print(
                f"[abs idx={index}] R={R.as_euler('xyz', degrees=True)}  init_R={wsm.init_R.as_euler('xyz', degrees=True)}\n"
                f"  sample bbox  min={bb.minPoint()}  max={bb.maxPoint()}\n"
                f"  gauge_volume={gv_str}"
            )

            MonteCarloAbsorption(**m.mc_kwargs)
            transmission = read_attenuation_coefficient_at_value(
                wsm.WS_MC_OUTPUT, wsm.attenuation_kwargs["point"], wsm.attenuation_kwargs["unit"]
            )[m.geometry.starting_ind :]
        except RuntimeError:
            logger.warning("MonteCarloAbsorption has failed, sample is assumed to be outside the gauge volume ")
            transmission = np.zeros(mc_ws.getNumberHistograms() - m.geometry.starting_ind)
        m.orientations[index].transmission = transmission

    def calc_all(self):
        for i in self._model.orientations.keys():
            self.calc_for_index(i)
