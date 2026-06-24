# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import numpy as np

from mantid.simpleapi import ConvertUnits, CopySample, MonteCarloAbsorption, RotateSampleShape
from mantid.api import MatrixWorkspace
from mantid.kernel import logger
from Engineering.texture.correction.correction_model import read_attenuation_coefficient_at_value
from Engineering.texture.texture_helper import define_gauge_volume
from typing import Any, Protocol, Dict, Tuple
from abc import abstractmethod
from scipy.spatial.transform import Rotation


class _WorkspaceManagerType(Protocol):
    """For the purpose of type hinting while this module is orphaned
    Will be removed and replaced with actual model before final PR"""

    WS_MC_OUTPUT: str
    WS_MC_INPUT: str
    wsname: str
    attenuation_kwargs: Dict[str, str | float]
    offset: Tuple[float | int, float | int, float | int]
    mesh_ws: MatrixWorkspace
    init_R: Rotation
    gauge_volume_str: str | None

    @staticmethod
    @abstractmethod
    def translate_shape(ws: MatrixWorkspace, x_pos: float | int, y_pos: float | int, z_pos: float | int):
        pass


class _BaseModelType(Protocol):
    """For the purpose of type hinting while this module is orphaned
    Will be removed and replaced with actual model before final PR"""

    workspaces: _WorkspaceManagerType
    orientations: Any
    geometry: Any


class AbsorptionCalculator:
    """Runs MonteCarloAbsorption per orientation.
    Passes the resulting transmission factors onto the orientation table via the manager model"""

    def __init__(self, model: _BaseModelType):
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
        m = self._model
        wsm = m.workspaces
        # create a workspace to run the absorption calculation on
        mc_ws = self._create_mc_ws(wsm)

        # extract goniometer for run index
        R = m.orientations[index].R

        # set sample state (orientated shape, material and gauge volume) for run index
        self._set_mc_sample_state(wsm, mc_ws, R)

        try:
            MonteCarloAbsorption(**self.mc_kwargs)
            transmission = read_attenuation_coefficient_at_value(
                wsm.WS_MC_OUTPUT, wsm.attenuation_kwargs["point"], wsm.attenuation_kwargs["unit"]
            )
            transmission = [transmission[spec_ind] for spec_ind in m.geometry.spec_inds]
        except RuntimeError:
            logger.warning("MonteCarloAbsorption has failed, sample is assumed to be outside the gauge volume ")
            transmission = [0] * len(m.geometry.spec_inds)
        m.orientations.set_transmission_at_index(transmission, index)

    @staticmethod
    def _create_mc_ws(wsm: _WorkspaceManagerType) -> MatrixWorkspace:
        mc_ws = ConvertUnits(InputWorkspace=wsm.wsname, Target="Wavelength", OutputWorkspace=wsm.WS_MC_INPUT)
        mc_ws.run().getGoniometer().setR(np.eye(3))
        return mc_ws

    @staticmethod
    def _set_mc_sample_state(wsm: _WorkspaceManagerType, mc_ws: MatrixWorkspace, R: Rotation) -> None:
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

    def calc_all(self) -> None:
        for i in self._model.orientations.keys():
            self.calc_for_index(i)
