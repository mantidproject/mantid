from mantid.simpleapi import CreatePoleFigureTableWorkspace, CloneWorkspace, CombineTableWorkspaces, logger
import numpy as np
from mantid.api import AnalysisDataService as ADS
from typing import Optional, Sequence
import matplotlib.pyplot as plt
from mantid.geometry import CrystalStructure


class TextureProjection:
    def get_ws_info(self, ws_name, parameter_file, select=True):
        return {
            "fit_parameters": parameter_file,
            "crystal": "Not set" if not self._has_xtal(ws_name) else self._get_xtal_info(ws_name),
            "select": select,
        }

    def _has_xtal(self, ws):
        return ADS.retrieve(ws).sample().hasCrystalStructure()

    def _get_xtal_info(self, ws):
        xtal = ADS.retrieve(ws).sample().getCrystalStructure()
        sg = xtal.getSpaceGroup().getHMSymbol()  # get the HM symbol
        scatts = ", ".join([scat.split(" ")[0] for scat in xtal.getScatterers()])
        return f"{scatts}: {sg}"

    def check_param_ws_for_columns(self, wss):
        has_chi2, has_x0 = True, True
        for ws in wss:
            param_ws = ADS.retrieve(ws)
            column_names = param_ws.getColumnNames()
            # check if any param ws are missing chi2 or x0
            if "chi2" not in column_names:
                has_chi2 = False
            if "x0" not in column_names:
                has_x0 = False
            return has_chi2, has_x0

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

    def plot_pole_figure(self, ws, projection: str, fig=None, **kwargs) -> None:
        if projection.lower() == "stereographic":
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

        fig = plt.figure() if not fig else fig
        ax = fig.add_subplot(1, 1, 1)
        ax.scatter(pfi[:, 1], pfi[:, 0], c=np.log(pfi[:, 2]), s=20, cmap="jet", **kwargs)
        ax.plot(eq[0], eq[1], c="grey")
        ax.set_aspect("equal")
        ax.set_axis_off()
        ax.quiver(-1, -1, 0.2, 0, color="blue", scale=1)
        ax.quiver(-1, -1, 0, 0.2, color="red", scale=1)

    def get_pf_table_name(self, wss, fit_params, hkl):
        fws, lws = ADS.retrieve(wss[0]), ADS.retrieve(wss[-1])
        run_range = f"{fws.run().getLogData('run_number').value}-{lws.run().getLogData('run_number').value}"
        instr = fws.getInstrument().getName()
        try:
            # try and get a peak reference either from hkl or from the X0 column of param
            peak = "".join([str(ind) for ind in hkl]) if hkl else str(np.round(np.mean(ADS.retrieve(fit_params[0]).column("X0")), 2))
            table_name = f"{instr}_{run_range}_{peak}_pf_table"
        except Exception:
            # if no param table given, no peak reference
            table_name = f"{instr}_{run_range}_pf_table"
        return table_name

    def _parse_hkl(self, H, K, L):
        try:
            return [int(H), int(K), int(L)]
        except Exception:
            return None

    def set_ws_xtal(self, ws: str, lattice: str, space_group: str, basis: str) -> None:
        ws = ADS.retrieve(ws)
        ws.sample().setCrystalStructure(CrystalStructure(lattice, space_group, basis))
        logger.notice("Crystal Structure Set")

    def set_all_ws_xtal(self, wss: Sequence[str], lattice: str, space_group: str, basis: str) -> None:
        for ws in wss:
            self.set_ws_xtal(ws, lattice, space_group, basis)

    def copy_xtal_to_all(self, ref_ws: str, wss: Sequence[str]) -> None:
        xtal = ADS.retrieve(ref_ws).sample().getCrystalStructure()
        for ws in wss:
            ADS.retrieve(ws).sample().setCrystalStructure(xtal)


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
