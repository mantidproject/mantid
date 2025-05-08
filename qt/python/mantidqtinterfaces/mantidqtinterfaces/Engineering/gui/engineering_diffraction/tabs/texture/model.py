from mantid.simpleapi import CreatePoleFigureTableWorkspace, CloneWorkspace, CombineTableWorkspaces
import numpy as np
from mantid.api import AnalysisDataService as ADS
from typing import Optional, Sequence
import matplotlib.pyplot as plt


class TextureProjection:
    def get_ws_info(self, ws_name, parameter_file, select=True):
        return {
            "fit_parameters": parameter_file,
            "crystal": "Not set" if self._has_xtal(ws_name) else "set",
            "select": select,
        }

    def _has_xtal(self, ws):
        return ADS.retrieve(ws).sample().hasCrystalStructure()

    def make_pole_figure_tables(
        self,
        wss: Sequence[str],
        peak_wss: Sequence[str],
        out_ws: str,
        hkl: Optional[Sequence[int]],
        inc_scatt_corr: bool,
        scat_vol_pos: Sequence[float],
        chi2_thresh: Optional[float],
        peak_thresh: Optional[float],
    ) -> None:
        table_workspaces = []
        if len(peak_wss) == len(wss):
            for iws, ws in enumerate(wss):
                ws_str = f"_{iws}_abi_table"
                CreatePoleFigureTableWorkspace(
                    InputWorkspace=ws,
                    PeakParameterWorkspace=peak_wss[iws],
                    OutputWorkspace=ws_str,
                    Reflection=hkl,
                    Chi2Threshold=chi2_thresh,
                    PeakPositionThreshold=peak_thresh,
                    ApplyScatteringPowerCorrection=inc_scatt_corr,
                    ScatteringVolumePosition=scat_vol_pos,
                )
                table_workspaces.append(ws_str)
        else:
            for iws, ws in enumerate(wss):
                ws_str = f"_{iws}_abi_table"
                CreatePoleFigureTableWorkspace(
                    InputWorkspace=ws,
                    PeakParameterWorkspace=None,
                    OutputWorkspace=ws_str,
                    Reflection=hkl,
                    Chi2Threshold=chi2_thresh,
                    PeakPositionThreshold=peak_thresh,
                    ApplyScatteringPowerCorrection=inc_scatt_corr,
                    ScatteringVolumePosition=scat_vol_pos,
                )
                table_workspaces.append(ws_str)
        CloneWorkspace(InputWorkspace=table_workspaces[0], OutputWorkspace=out_ws)
        for tw in table_workspaces[1:]:
            CombineTableWorkspaces(LHSWorkspace=out_ws, RHSWorkspace=tw, OutputWorkspace=out_ws)

    def plot_pole_figure(self, ws, projection: str, **kwargs) -> None:
        if projection == "stereo":
            proj = ster_proj
        else:
            proj = azim_proj
        if isinstance(ws, str):
            ws = ADS.retrieve(ws)
        alphas = np.asarray(ws.column("Alpha"))
        betas = np.asarray(ws.column("Beta"))
        i = np.asarray(ws.column("Intensity"))

        pfi = proj(alphas, betas, i)

        u = np.linspace(0, 2 * np.pi, 100)
        x = np.cos(u)
        y = np.sin(u)
        z = np.zeros_like(x)
        eq = np.concatenate((x[None, :], y[None, :], z[None, :]), axis=0)

        fig = plt.figure()
        ax = fig.add_subplot(1, 1, 1)
        ax.scatter(pfi[:, 1], pfi[:, 0], c=np.log(pfi[:, 2]), s=20, cmap="jet", **kwargs)
        ax.plot(eq[0], eq[1], c="grey")
        ax.set_aspect("equal")
        ax.set_axis_off()
        ax.quiver(-1, -1, 0.2, 0, color="blue", scale=1)
        ax.quiver(-1, -1, 0, 0.2, color="red", scale=1)
        fig.show()


def ster_proj(alphas: np.ndarray, betas: np.ndarray, i: np.ndarray) -> np.ndarray:
    betas = np.pi - betas  # this formula projects onto the north pole, and beta is taken from the south
    r = np.sin(betas) / (1 - np.cos(betas))
    out = np.zeros((len(alphas), 3))
    out[:, 0] = r * np.cos(alphas)
    out[:, 1] = r * np.sin(alphas)
    out[:, 2] = i
    return out


def azim_proj(alphas: np.ndarray, betas: np.ndarray, i: np.ndarray) -> np.ndarray:
    betas = betas / (np.pi / 2)
    xs = (betas * np.cos(alphas))[:, None]
    zs = (betas * np.sin(alphas))[:, None]
    out = np.concatenate([xs, zs, i[:, None]], axis=1)
    return out
