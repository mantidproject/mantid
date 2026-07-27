# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import annotations

import numpy as np

from mantid.simpleapi import ConvertUnits, CopySample, MonteCarloAbsorption, RotateSampleShape
from mantid.api import MatrixWorkspace
from mantid.kernel import logger
from Engineering.texture.correction.correction_model import read_attenuation_coefficient_at_value
from Engineering.texture.texture_helper import define_gauge_volume
from typing import TYPE_CHECKING
from scipy.spatial.transform import Rotation

if TYPE_CHECKING:
    from mantidqtinterfaces.TexturePlanner.model import TexturePlannerModel
    from mantidqtinterfaces.TexturePlanner.helpers.workspace_manager import WorkspaceManager


class AbsorptionCalculator:
    """Runs MonteCarloAbsorption per orientation.
    Passes the resulting transmission factors onto the orientation table via the manager model"""

    def __init__(self, model: TexturePlannerModel):
        self._model = model
        # MonteCarloAbsorption settings; the in/out workspace names come from the workspace manager,
        # which the model constructs before this collaborator.
        self.mc_kwargs = {
            "InputWorkspace": model.workspaces.WS_MC_INPUT,
            "OutputWorkspace": model.workspaces.WS_MC_OUTPUT,
            "EventsPerPoint": 50,
            "MaxScatterPtAttempts": int(1e4),
            "SimulateScatteringPointIn": "SampleOnly",
            "ResimulateTracksForDifferentWavelengths": False,
        }

    def calc_for_index(self, index: int) -> None:
        wsm = self._model.workspaces
        # create a workspace to run the absorption calculation on
        mc_ws = self._create_mc_ws(wsm)

        # extract goniometer for run index
        R = self._model.orientations[index].R

        # set sample state (orientated shape, material and gauge volume) for run index
        self._set_mc_sample_state(wsm, mc_ws, R)

        try:
            MonteCarloAbsorption(**self.mc_kwargs)
            transmission = read_attenuation_coefficient_at_value(
                wsm.WS_MC_OUTPUT, wsm.attenuation_kwargs["point"], wsm.attenuation_kwargs["unit"]
            )
            transmission = [transmission[spec_ind] for spec_ind in self._model.geometry.spec_inds]
        except RuntimeError:
            logger.warning("MonteCarloAbsorption has failed, sample is assumed to be outside the gauge volume ")
            transmission = [0] * len(self._model.geometry.spec_inds)
        self._model.orientations.set_transmission_at_index(transmission, index)

    @staticmethod
    def _create_mc_ws(wsm: WorkspaceManager) -> MatrixWorkspace:
        mc_ws = ConvertUnits(InputWorkspace=wsm.wsname, Target="Wavelength", OutputWorkspace=wsm.WS_MC_INPUT)
        mc_ws.run().getGoniometer().setR(np.eye(3))
        return mc_ws

    @staticmethod
    def _set_mc_sample_state(wsm: WorkspaceManager, mc_ws: MatrixWorkspace, R: Rotation) -> None:
        # copy sample shape and material from mesh ws (untransformed sample - no init_R, no translation, identity goniometer)
        CopySample(
            InputWorkspace=wsm.mesh_ws,
            OutputWorkspace=wsm.WS_MC_INPUT,
            CopyShape=True,
            CopyMaterial=True,
            CopyEnvironment=False,
            CopyLattice=False,
        )

        # apply the initial translation *after* the initial orientation: the offset is a lab-frame
        # shift of the oriented sample, so it is expressed in the sample's pre-orientation frame
        # (see WorkspaceManager.initial_translation_vector) before being baked into the shape.
        wsm.translate_shape(mc_ws, *wsm.initial_translation_vector())

        # apply both the initial and the goniometer rotations
        shapeR = R * wsm.init_R
        rotvec = shapeR.as_rotvec(degrees=True)
        ang = np.linalg.norm(rotvec)
        if ang != 0:
            vec = rotvec / ang
            RotateSampleShape(wsm.WS_MC_INPUT, f"{ang},{vec[0]},{vec[1]},{vec[2]},1")

        # define the gauge volume
        define_gauge_volume(mc_ws, wsm.gauge_volume_str)

    def calc_all(self) -> None:
        for i in self._model.orientations.keys():
            self.calc_for_index(i)
