# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.simpleapi import (
    LoadEmptyInstrument,
    GroupDetectors,
    SetSampleShape,
    LoadSampleShape,
    MonteCarloAbsorption,
    SetSampleMaterial,
    CreateSimulationWorkspace,
    ConvertUnits,
    CopySample,
)
from Engineering.EnggUtils import GROUP, CALIB_DIR
from Engineering.common.calibration_info import CalibrationInfo
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.show_sample.show_sample_model import ShowSampleModel
from Engineering.common.xml_shapes import get_cube_xml
from mantidqt.plotting.sample_shape import plot_sample_only
import os
import numpy as np
from scipy.spatial.transform import Rotation
import matplotlib.pyplot as plt
from mantid.kernel import logger
from Engineering.texture.TextureUtils import convert_to_sscanss_frame
from Engineering.texture.correction.correction_model import read_attenuation_coefficient_at_value


class TexturePlannerModel(object):
    def __init__(self, instrument="ENGINX", projection="azimuthal"):
        # probably static
        self.wsname = "__texture_planning_ws"
        self.instr_wsname = "__texture_planning_instr"
        self.sense_vals = {"Clockwise": -1, "Counterclockwise": 1}  # mantid convention
        self.sense_names = {"-1": "Clockwise", "1": "Counterclockwise"}
        self.supported_groups = ("Texture20", "Texture30", "banks")
        self.gon_colors = ("hotpink", "orange", "purple", "goldenrod", "plum", "saddlebrown")
        self.dir_cols = ("red", "green", "blue")
        self.axis_dict = {"x": (1, 0, 0), "y": (0, 1, 0), "z": (0, 0, 1)}

        # properties which will be updated
        self.ws = None
        self.instr_ws = None
        self.instr = instrument
        self.calib_info = None
        self.group = None
        self.ax_transform = np.eye(3)
        self.dir_names = ["D1", "D2", "D3"]
        self.gRs = []
        self.R = Rotation.identity()
        self.detQs_lab = None
        self.projection = projection
        self.gonio_index = 0
        self.n_gonio = 2
        self.orientation_index = 0
        self.n_output_points = 1
        self.plot_attenuation = False

        # stl_settings
        self.stl_kwargs = {"Scale": "cm", "XDegrees": 0, "YDegrees": 0, "ZDegrees": "0", "TranslationVector": "0,0,0"}

        # euler_file_settings
        self.orientation_kwargs = {"Axes": "YXY", "Senses": "-1,-1,-1"}

        self.mc_kwargs = {
            "InputWorkspace": "__mc_ws",
            "OutputWorkspace": "__abs_ws",
            "EventsPerPoint": 50,
            "MaxScatterPtAttempts": 1000,
            "SimulateScatteringPointIn": "SampleOnly",
            "ResimulateTracksForDifferentWavelengths": True,
        }

        self.attenuation_kwargs = {"point": 1.5, "unit": "dSpacing", "material": "Fe"}

        # data structure
        self.saved_orientations = {0: self.get_default_info_dict()}

        # init func calls
        self.update_ws()

    def update_ws(self):
        if not self.ws:
            self.instr_ws = LoadEmptyInstrument(InstrumentName=self.instr, OutputWorkspace=self.instr_wsname)
            self.ws = CreateSimulationWorkspace(Instrument=self.instr, BinParams="1,0.5,2", OutputWorkspace=self.wsname, UnitX="dSpacing")
            SetSampleShape(self.ws, get_cube_xml("default_cube", 0.01))
            self.set_material()
        # add handling here for copying over sample shape if the instrument is changed

    def set_material(self):
        SetSampleMaterial(self.ws, self.attenuation_kwargs["material"])

    def set_material_string(self, material):
        try:
            self.attenuation_kwargs["material"] = material
            self.set_material()
        except Exception:
            logger.error("Invalid Material 'ChemicalFormula' - see SetSampleMaterial docs for more info")

    def load_stl(self, stl_file):
        LoadSampleShape(InputWorkspace=self.ws, Filename=stl_file, OutputWorkspace=self.ws, **self.stl_kwargs)
        self.set_material()

    def load_xml(self, xml_file):
        with open(xml_file, "r") as f:
            xml_string = f.read()
        SetSampleShape(self.ws, xml_string)
        self.set_material()

    def load_orientation_file(self, txt_file):
        logger.notice("Loading Orientations from file")
        with open(txt_file, "r") as f:
            goniometer_strings = [line.strip().replace("\t", ",") for line in f]
            goniometer_lists = [[float(x) for x in gs.split(",")] for gs in goniometer_strings]
        print(goniometer_lists)
        if len(goniometer_lists) == 0:
            logger.warning("No orientations found in file provided")
            return 3
        num_entries = len(goniometer_lists[0])
        euler_angles = num_entries <= 6
        if not euler_angles:
            for goniometer_list in goniometer_lists:
                R_mat = np.asarray(goniometer_list[:9]).reshape((3, 3))
                R = Rotation.from_matrix(R_mat)
                vecs = [(0, 1, 0), (1, 0, 0), (0, 1, 0)]
                senses = [1, 1, 1]
                angles = R.as_euler("YXY", degrees=True)
                self.add_orientation()
                self.update_gonio_string(vecs, senses, np.round(angles, 2), self.get_num_orientations() - 1)
                self.update_gRs(vecs, senses, np.round(angles, 2), self.get_num_orientations() - 1)
            return 3
        msg = ""
        axes, senses = self.orientation_kwargs["Axes"], self.orientation_kwargs["Senses"].split(",")
        num_ax, num_senses = len(axes), len(senses)
        if num_entries != num_ax:
            msg += f"Number of Angles ({num_entries}) does not match number of goniometer axes ({num_ax} \n"
        if num_entries != num_senses:
            msg += f"Number of Angles ({num_entries}) does not match number of goniometer senses ({num_senses} \n"
        if msg != "":
            logger.error(msg)
            return 3
        vecs = [self.axis_dict[ax.lower()] for ax in axes]
        senses = [int(sense) for sense in senses]
        for angles in goniometer_lists:
            self.add_orientation()
            self.update_gonio_string(vecs, senses, angles, self.get_num_orientations() - 1)
            self.update_gRs(vecs, senses, np.round(angles, 2), self.get_num_orientations() - 1)
        return num_ax

    def set_n_gonio(self, val):
        self.n_gonio = val

    def set_group(self, group_str):
        self.group = GROUP(group_str)
        self.update_calib_info()

    def set_ax_transform(self, vec1, vec2, vec3):
        vec1, vec2, vec3 = vec_string_to_norm_array(vec1), vec_string_to_norm_array(vec2), vec_string_to_norm_array(vec3)
        self.ax_transform = np.concatenate((vec1[:, None], vec2[:, None], vec3[:, None]), axis=1)

    def set_dir_names(self, name1, name2, name3):
        self.dir_names = [name1, name2, name3]

    def set_gonio_index(self, index):
        self.gonio_index = index

    def set_plot_attenuation(self, val):
        self.plot_attenuation = val

    def update_calib_info(self):
        self.calib_info = CalibrationInfo(group=self.group)

    @staticmethod
    def get_default_texture_directions():
        return ("RD", "ND", "TD"), ((1, 0, 0), (0, 1, 0), (0, 0, 1))

    def get_orientation_index(self):
        return self.orientation_index

    def set_orientation_index(self, index):
        self.orientation_index = index

    def get_vecs(self, all_vec_strings, num_gonios):
        vec_strings = all_vec_strings[:num_gonios]
        return [vec_string_to_norm_array(vec_string) for vec_string in vec_strings]

    def get_senses(self, senses, num_gonios):
        return [self.sense_vals[x] for x in senses[:num_gonios]]

    def get_angles(self, angles, num_gonios):
        return [float(x) for x in angles[:num_gonios]]

    def update_gonio_index(self, num_gonios):
        max_ind = num_gonios - 1
        return min(self.gonio_index, max_ind)

    def calc_gRs(self, vecs, senses, angles):
        gRs = [Rotation.identity()]
        R = Rotation.identity()
        for i, vec in enumerate(vecs):
            sense = -senses[i]  # mantid convention is that ccw = 1, we need cw = 1
            r_step = Rotation.from_davenport(vec, "extrinsic", sense * angles[i], degrees=True)
            R = R * r_step
            gRs.append(R)
        return gRs, R

    def update_gRs(self, vecs, senses, angles, current_index):
        gRs, R = self.calc_gRs(vecs, senses, angles)
        self.saved_orientations[current_index]["gRs"] = gRs
        self.saved_orientations[current_index]["R"] = R

    def update_selected(self, selected_inds):
        for k, v in self.saved_orientations.items():
            v["select"] = k in selected_inds

    def update_included(self, included_inds):
        for k, v in self.saved_orientations.items():
            v["include"] = k in included_inds

    def select_all(self):
        for k, v in self.saved_orientations.items():
            v["select"] = True

    def deselect_all(self):
        for k, v in self.saved_orientations.items():
            v["select"] = False

    def delete_selected(self):
        # iterate through the table and find which orientations are being kept
        to_keep = []
        new_orientations = {}
        for k, v in self.saved_orientations.items():
            if not v.get("select", True):
                to_keep.append(k)
        # if nothing is kept instantiate new table
        if len(to_keep) == 0:
            self.saved_orientations = {0: self.get_default_info_dict()}
        # otherwise copy across
        else:
            for i, k in enumerate(to_keep):
                new_orientations[i] = self.saved_orientations[k]
            self.saved_orientations = new_orientations
        # if the orientation corresponding to the current index has been kept, update the index, otherwise 0
        new_orientation_index = to_keep.index(self.orientation_index) if self.orientation_index in to_keep else 0
        self.set_orientation_index(new_orientation_index)

    def get_default_info_dict(self):
        info = {}
        vecs, senses, angles = (
            [
                (1, 0, 0),
            ]
            * 6,
            [
                -1,
            ]
            * 6,
            [
                0.0,
            ]
            * 6,
        )
        gRs, R = self.calc_gRs(vecs[: self.n_gonio], senses[: self.n_gonio], angles[: self.n_gonio])  # number of GRs controls the plot size
        for i, vec in enumerate(vecs):
            info[f"g{i}"] = self.get_goniometer_string(vec, senses[i], angles[i])
        info["gRs"] = gRs
        info["R"] = R
        info["include"] = True
        info["select"] = True
        return info

    def get_goniometer_string(self, vec, sense, angle):
        return f"{angle},{np.round(vec[0], 3)},{np.round(vec[1], 3)},{np.round(vec[2], 3)},{sense}"

    def update_gonio_string(self, vecs, senses, angles, index):
        for i, vec in enumerate(vecs):
            self.saved_orientations[index][f"g{i}"] = self.get_goniometer_string(vec, senses[i], angles[i])
        for i in range(len(vecs), 6):
            # for the extra goniometers, just set to default axis and null transformations
            self.saved_orientations[index][f"g{i}"] = self.get_goniometer_string((1, 0, 0), 1, 0)

    def read_goniometer_string(self, goniometer_string):
        angle, v1, v2, v3, sense = goniometer_string.split(",")
        return f"{v1},{v2},{v3}", self.sense_names[sense], float(angle)

    def get_goniometer_values(self, index):
        info = self.saved_orientations[index]
        if "g0" not in info.keys():
            return (
                [
                    "1,0,0",
                ]
                * 6,
                [
                    "Clockwise",
                ]
                * 6,
                [
                    0.0,
                ]
                * 6,
            )
        vecs, senses, angles = [], [], []
        for i in range(6):
            vec, sense, angle = self.read_goniometer_string(info[f"g{i}"])
            vecs.append(vec)
            senses.append(sense)
            angles.append(angle)
        return vecs, senses, angles

    def get_detQ_lab(self):
        group_ws = GroupDetectors(
            InputWorkspace=self.instr_ws,
            MapFile=os.path.join(CALIB_DIR, self.calib_info.get_group_file()),
            OutputWorkspace="group_ws",
            StoreInADS=False,
        )
        self.ws = GroupDetectors(
            InputWorkspace=self.ws,
            MapFile=os.path.join(CALIB_DIR, self.calib_info.get_group_file()),
            OutputWorkspace=self.wsname,
        )
        spec_info = group_ws.spectrumInfo()
        comp_info = group_ws.componentInfo()
        det_pos = np.asarray(
            [spec_info.position(i) / np.linalg.norm(spec_info.position(i)) for i in range(1, group_ws.getNumberHistograms())]
        )
        ki = -np.array(comp_info.sourcePosition())
        ki_norm = ki / np.linalg.norm(ki)
        detQs_lab = det_pos - ki_norm
        self.detQs_lab = detQs_lab / np.linalg.norm(detQs_lab, axis=1)[:, None]

    def update_all_projected_data(self):
        for i in self.saved_orientations.keys():
            self.update_projected_data(i)

    def update_projected_data(self, index):
        R = self.saved_orientations[index]["R"]

        rot_pos = R.inv().apply(self.detQs_lab) @ self.ax_transform

        cart_pos = get_alpha_beta_from_cart(rot_pos.T)
        self.saved_orientations[index]["pf_points"] = ster_proj(*cart_pos.T) if self.projection == "ster" else azim_proj(*cart_pos.T)
        if self.plot_attenuation:
            self.calc_monte_carlo_absorption_val_for_index(index)

    def add_orientation(self):
        # create a new orientation, initially just as a copy of the current orientation
        self.saved_orientations[self.get_num_orientations()] = self.saved_orientations[self.orientation_index].copy()

    def get_num_orientations(self):
        return len(self.saved_orientations.keys())

    def get_table_info(self):
        table_info = []
        for index, val in self.saved_orientations.items():
            row_info = []
            row_info.append([val[f"g{x}"] for x in range(6)])
            row_info.append(val.get("include", True))
            row_info.append(val.get("select", True))
            table_info.append(row_info)
        return table_info

    def calc_monte_carlo_absorption_val_for_index(self, index):
        R = self.saved_orientations[index]["R"]
        mc_ws = ConvertUnits(InputWorkspace=self.wsname, Target="Wavelength", OutputWorkspace="__mc_ws")
        mc_ws.run().getGoniometer().setR(R.as_matrix())
        self.ws.run().getGoniometer().setR(np.eye(3))
        CopySample(
            InputWorkspace=self.wsname,
            OutputWorkspace="__mc_ws",
            CopyShape=True,
            CopyMaterial=True,
            CopyEnvironment=False,
            CopyLattice=False,
        )
        MonteCarloAbsorption(**self.mc_kwargs)
        # first entry is null group so we can ignore it
        mu = read_attenuation_coefficient_at_value("__abs_ws", self.attenuation_kwargs["point"], self.attenuation_kwargs["unit"])[1:]
        self.saved_orientations[index]["mu"] = mu

    def calc_all_monte_carlo_absorption_vals(self):
        for i in self.saved_orientations.keys():
            self.calc_monte_carlo_absorption_val_for_index(i)

    def update_plot(self, vecs, senses, angles, fig, lab_ax, proj_ax, current_index):
        lab_ax.clear()
        proj_ax.clear()

        gRs = self.saved_orientations[current_index]["gRs"]
        R = self.saved_orientations[current_index]["R"]
        nGon = len(gRs)

        gVecs = []

        detQs_lab = self.detQs_lab
        ax_transform = self.ax_transform

        self.ws.run().getGoniometer().setR(R.as_matrix())

        shape_mesh = self.ws.sample().getShape().getMesh()
        extent = (np.linalg.norm(shape_mesh, axis=(1, 2)).max() / 2) * 1.2

        rot_mesh = R.apply(shape_mesh.reshape((-1, 3))).reshape(shape_mesh.shape)

        for i, vec in enumerate(vecs):
            gR = gRs[i]
            gVec = gR.apply(vec)
            gVecs.append(gVec)

            gon_scale = (1 + ((nGon - i) / 2)) * extent

            gon_ring = gR.apply(ring(vec, gon_scale, res=360).T).T

            sense = -senses[i]  # mantid convention is that ccw = 1, we need cw = 1
            angle = angles[i] * sense

            if angle <= 0:
                gon_ring = np.flip(gon_ring, axis=1)  # reverse the ring if the angle is negative

            pos_ind = int(np.abs(angle))

            # 3D goniometer plot

            lab_ax.plot(*gon_ring[:, : pos_ind + 1], color=self.gon_colors[i])
            lab_ax.plot(*gon_ring[:, pos_ind:], color="grey")
            lab_ax.quiver(
                *np.zeros(3), *gVec * extent * 2, color=self.gon_colors[i], ls=("-", "--")[int(i != self.gonio_index)], label=f"Axis {i}"
            )

        lab_ax.legend()

        gPole = R.inv().apply(np.array(gVecs)) @ ax_transform
        cart_gPole = get_alpha_beta_from_cart(gPole.T)
        gPole_xy = ster_proj(*cart_gPole.T) if self.projection == "ster" else azim_proj(*cart_gPole.T)

        # 3D plot
        sample_model = ShowSampleModel()
        sample_model.fig = fig
        sample_model.ws_name = self.wsname
        fig.sca(lab_ax)
        plot_sample_only(fig, rot_mesh * 0.5, 0.5, "grey")
        sample_model.plot_sample_directions(ax_transform, self.dir_names)
        lab_ax.set_xlim([-extent * nGon / 1.5, extent * nGon / 1.5])
        lab_ax.set_ylim([-extent * nGon / 1.5, extent * nGon / 1.5])
        lab_ax.set_zlim([-extent * nGon / 1.5, extent * nGon / 1.5])
        lab_ax.set_aspect("equal")

        # plot incident beam
        comp_info = self.ws.componentInfo()
        ki = -np.array(comp_info.sourcePosition())
        ki_norm = ki / np.linalg.norm(ki)
        ki = ki_norm * extent * nGon / 0.75
        beam = -ki
        lab_ax.quiver(*beam, *ki, arrow_length_ratio=0.05, color="black", alpha=0.25)

        # plot detector Qs
        [lab_ax.quiver(*np.zeros(3), *dQ * 1.25 * extent, arrow_length_ratio=0.05, color="grey", alpha=0.25) for dQ in detQs_lab]
        [
            lab_ax.scatter(
                *dQ * 1.25 * extent,
                color="dodgerblue",
                s=2,
            )
            for i, dQ in enumerate(detQs_lab)
        ]
        lab_ax.set_axis_off()

        # 2D plot
        for i, gP in enumerate(gPole_xy):
            pc = self.gon_colors[i]
            fc = "None" if i != self.gonio_index else pc
            if np.isclose(np.linalg.norm(gP), 1):
                proj_ax.plot((gP[1], -gP[1]), (gP[0], -gP[0]), color=pc, ls=("-", "--")[int(i != self.gonio_index)])
            else:
                proj_ax.scatter(gP[1], gP[0], s=30, edgecolor=pc, facecolor=fc)

        if not self.plot_attenuation:
            for i in self.saved_orientations.keys():
                if self.saved_orientations[i].get("include", True):
                    pf_xy = self.saved_orientations[i]["pf_points"]
                    if i == current_index:
                        proj_ax.scatter(pf_xy[:, 1], pf_xy[:, 0], s=20, c="dodgerblue")
                    else:
                        proj_ax.scatter(pf_xy[:, 1], pf_xy[:, 0], s=20, facecolor="None", edgecolor="dodgerblue")
                elif i == current_index:
                    pf_xy = self.saved_orientations[i]["pf_points"]
                    proj_ax.scatter(pf_xy[:, 1], pf_xy[:, 0], s=20, facecolor="None", edgecolor="grey", alpha=0.5)
        else:
            all_pf_xy = np.concatenate(
                [info["pf_points"] for info in self.saved_orientations.values() if info.get("include", True)], axis=0
            )
            all_mus = np.concatenate([info["mu"] for info in self.saved_orientations.values() if info.get("include", True)], axis=0)
            scatt = proj_ax.scatter(all_pf_xy[:, 1], all_pf_xy[:, 0], s=20, c=all_mus, vmin=0, vmax=1, cmap="jet")
            cax = proj_ax.inset_axes([0.9, 0.15, 0.05, 0.7])
            proj_ax.figure.colorbar(scatt, cax=cax)

        proj_ax.set_aspect("equal")
        proj_ax.set_xlim(-1.5, 1.5)
        proj_ax.set_ylim(-1.5, 1.5)
        [proj_ax.quiver(*np.array((-1, -1)), *bv, color=self.dir_cols[-1 + i], scale=5) for i, bv in enumerate(np.eye(2))]
        circle = plt.Circle((0, 0), 1, color="grey", fill=False, linestyle="-")
        proj_ax.add_patch(circle)
        proj_ax.annotate(self.dir_names[0], (-0.95, -0.8))
        proj_ax.annotate(self.dir_names[2], (-0.8, -0.95))
        proj_ax.set_axis_off()

        fig.canvas.draw_idle()
        proj_ax.figure.canvas.draw_idle()

    def output_as_sscanss(self, save_dir, filename):
        header = [
            "xyz\n",
        ]
        sscanss_lines = []
        save_file = os.path.join(save_dir, filename + ".angles")
        for i in self.saved_orientations.keys():
            if self.saved_orientations[i].get("include", True):
                angs = convert_to_sscanss_frame(self.saved_orientations[i]["R"].as_matrix())
                for _ in range(self.n_output_points):
                    sscanss_lines.append(f"{np.round(angs[0], 2)}\t{np.round(angs[1], 2)}\t{np.round(angs[2], 2)}\n")

        with open(save_file, "w") as f:
            f.writelines(header + sscanss_lines)
        logger.notice(f"Orientation data written to '{save_file}' as Sscanss2 Angles")

    def output_as_matrix(self, save_dir, filename):
        lines = []
        save_file = os.path.join(save_dir, filename + ".txt")
        for i in self.saved_orientations.keys():
            if self.saved_orientations[i].get("include", True):
                rot_mat = self.saved_orientations[i]["R"].as_matrix().reshape(-1)
                for _ in range(self.n_output_points):
                    lines.append("\t".join([str(x) for x in rot_mat]) + "\n")
        with open(save_file, "w") as f:
            f.writelines(lines)
        logger.notice(f"Orientation data written to '{save_file}' as Rotation Matrices")

    def output_as_euler(self, save_dir, filename):
        lines = []
        save_file = os.path.join(save_dir, filename + ".txt")
        for i in self.saved_orientations.keys():
            if self.saved_orientations[i].get("include", True):
                angles = self.saved_orientations[i]["R"].as_euler(self.orientation_kwargs["Axes"], degrees=True)
                angles = [str(float(sense) * angles[i]) for i, sense in enumerate(self.orientation_kwargs["Senses"].split(","))]
                for _ in range(self.n_output_points):
                    lines.append("\t".join(angles) + "\n")
        with open(save_file, "w") as f:
            f.writelines(lines)
        logger.notice(
            f"Orientation data written to '{save_file}' as Euler Angles with "
            f"Scheme ({self.orientation_kwargs['Axes']}) and Senses ({self.orientation_kwargs['Senses']})"
        )


def ring(axis, r=1, res=100, offset=(0, 0, 0)):
    u = np.linspace(0, 2 * np.pi, res)
    a = r * np.cos(u) + offset[0]
    b = r * np.sin(u) + offset[1]
    c = np.zeros_like(a) + offset[2]
    points = np.concatenate((a[None, :], b[None, :], c[None, :]), axis=0)
    R, _ = Rotation.align_vectors(axis, np.array((0, 0, 1)))
    return R.apply(points.T).T


def get_alpha_beta_from_cart(q_sample_cart: np.ndarray) -> np.ndarray:
    """
    get spherical angles from cartesian coordinates
    alpha is angle from positive x towards positive z
    beta is angle from positive y
    """
    q_sample_cart = np.clip(q_sample_cart.copy(), -1.0, 1.0)  # numerical inaccuracies outside this range will give nan in the trig funcs
    q_sample_cart = np.where(q_sample_cart[1] < 0, -q_sample_cart, q_sample_cart)  # invert the southern points
    alphas = np.arctan2(q_sample_cart[2], q_sample_cart[0])
    betas = np.arccos(q_sample_cart[1])
    return np.concatenate([alphas[:, None], betas[:, None]], axis=1)


def spherical_to_cartesian(phi, theta):
    """
    Convert spherical angles to 3D Cartesian unit vector.
    phi: azimuthal angle [0, 2pi]
    theta: polar angle [0, pi/2]
    """
    x = np.sin(theta) * np.cos(phi)
    y = np.cos(theta)
    z = np.sin(theta) * np.sin(phi)
    return np.stack((x, y, z), axis=-1)


def ster_proj(alphas: np.ndarray, betas: np.ndarray) -> np.ndarray:
    betas = np.pi - betas  # this formula projects onto the north pole, and beta is taken from the south
    r = np.sin(betas) / (1 - np.cos(betas))
    out = np.zeros((len(alphas), 2))
    out[:, 0] = r * np.cos(alphas)
    out[:, 1] = r * np.sin(alphas)
    return out


def azim_proj(alphas: np.ndarray, betas: np.ndarray) -> np.ndarray:
    betas = betas / (np.pi / 2)
    xs = (betas * np.cos(alphas))[:, None]
    zs = (betas * np.sin(alphas))[:, None]
    out = np.concatenate([xs, zs], axis=1)
    return out


def vec_string_to_norm_array(vec_string):
    if len(vec_string.split(",")) != 3:
        logger.error(f"Vector string provided is not the correct length: {vec_string}, (1,0,0) has been used instead")
        return np.array((1, 0, 0))
    try:
        vec = np.asarray([float(x) for x in vec_string.split(",")])
    except Exception:
        logger.error(f"Invalid vector string provided: {vec_string}, (1,0,0) has been used instead")
        # this is anticipated when entering goniometer axis, we just return it to default on error to prevent crashes
        # from the auto plot updates
        return np.array((1, 0, 0))
    return vec / np.linalg.norm(vec)
