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
    """Runs MonteCarloAbsorption per orientation.
    Passes the resulting transmission factors onto the orientation table via the manager model"""

    def __init__(self, model):
        self._model = model

    def calc_for_index(self, index):
        m = self._model
        wsm = m.workspaces
        # create a workspace to run the absorption calculation on
        mc_ws = self._create_mc_ws(wsm)

        # extract goniometer for run index
        R = m.orientations[index].R

        # set sample state (orientated shape, material and gauge volume) for run index
        self._set_mc_sample_state(wsm, mc_ws, R)

        try:
            MonteCarloAbsorption(**m.mc_kwargs)
            transmission = read_attenuation_coefficient_at_value(
                wsm.WS_MC_OUTPUT, wsm.attenuation_kwargs["point"], wsm.attenuation_kwargs["unit"]
            )[m.geometry.starting_ind :]
        except RuntimeError:
            logger.warning("MonteCarloAbsorption has failed, sample is assumed to be outside the gauge volume ")
            transmission = np.zeros(mc_ws.getNumberHistograms() - m.geometry.starting_ind)
        m.orientations.set_transmission_at_index(transmission, index)

    @staticmethod
    def _create_mc_ws(wsm):
        mc_ws = ConvertUnits(InputWorkspace=wsm.wsname, Target="Wavelength", OutputWorkspace=wsm.WS_MC_INPUT)
        mc_ws.run().getGoniometer().setR(np.eye(3))
        return mc_ws

    @staticmethod
    def _set_mc_sample_state(wsm, mc_ws, R):
        # copy sample shape and material from mesh ws (untransformed sample - no init_R, no translation, identity goniometer)
        CopySample(
            InputWorkspace=wsm.mesh_ws,
            OutputWorkspace=wsm.WS_MC_INPUT,
            CopyShape=True,
            CopyMaterial=True,
            CopyEnvironment=False,
            CopyLattice=False,
        )

        # apply the initial translation
        wsm.translate_shape(mc_ws, *wsm.offset)

        # apply both the initial and the goniometer rotations
        shapeR = R * wsm.init_R
        rotvec = shapeR.as_rotvec(degrees=True)
        ang = np.linalg.norm(rotvec)
        if ang != 0:
            vec = rotvec / ang
            RotateSampleShape(wsm.WS_MC_INPUT, f"{ang},{vec[0]},{vec[1]},{vec[2]},1")

        # define the gauge volume
        define_gauge_volume(mc_ws, wsm.gauge_volume_str)

    def calc_all(self):
        for i in self._model.orientations.keys():
            self.calc_for_index(i)
