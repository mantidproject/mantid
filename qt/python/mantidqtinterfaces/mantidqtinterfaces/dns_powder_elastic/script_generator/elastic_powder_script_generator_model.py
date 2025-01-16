# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
DNS script generator model for elastic powder data.
"""

import numpy as np

from mantidqtinterfaces.dns_powder_elastic.data_structures.dns_elastic_powder_dataset import DNSElasticDataset
from mantidqtinterfaces.dns_powder_tof.helpers.list_range_converters import get_normalisation
from mantidqtinterfaces.dns_powder_tof.script_generator.common_script_generator_model import DNSScriptGeneratorModel


class DNSElasticPowderScriptGeneratorModel(DNSScriptGeneratorModel):
    # pylint: disable=too-many-instance-attributes
    # having the options as instance attributes, is much better readable
    # none of them are public

    def __init__(self, parent):
        super().__init__(parent)
        self._script = []
        self._plot_list = []
        self._sample_data = None
        self._standard_data = None
        self._loop = None
        self._spacing = None
        self._vana_correction = None
        self._nicr_correction = None
        self._sample_background_correction = None
        self._background_factor = None
        self._ignore_vana = None
        self._sum_sf_nsf = None
        self._non_magnetic = None
        self._xyz = None
        self._corrections = None
        self._export_path = None
        self._ascii = None
        self._nexus = None
        self._norm = None

    def _check_xyz_separation(self, fields):
        ts = all(field in fields for field in ["x_sf", "y_sf", "z_sf", "z_nsf"])
        return ts

    @staticmethod
    def _get_xyz_field_string(sample):
        st = "# do xyz polarization analysis for magnetic powders \n"
        st += f"{sample}_nuclear_coh, {sample}_magnetic, {sample}_spin_incoh"
        st += "= xyz_separation(\n"
        st += f"    x_sf='{sample}_x_sf',"
        st += f" y_sf='{sample}_y_sf',"
        st += f" z_sf='{sample}_z_sf',"
        st += f" z_nsf='{sample}_z_nsf')"
        return [st]

    @staticmethod
    def _get_to_matrix_string(sample, sample_type):
        return (
            f"ConvertMDHistoToMatrixWorkspace('{sample}_{sample_type}', "
            f"Outputworkspace='mat_{sample}_{sample_type}',"
            " Normalization='NoNormalization', FindXAxis=False)"
            ""
        )

    def _get_sep_save_string(self, sample, sample_type):
        lines = ""
        if self._ascii:
            lines += f"SaveAscii('mat_{sample}_{sample_type}', '{self._export_path}/{sample}_{sample_type}.csv', WriteSpectrumID=False)"
        if self._nexus:
            lines += f"\nSaveNexus('mat_{sample}_{sample_type}', '{self._export_path}/{sample}_{sample_type}.nxs')"
        return lines

    def _get_xyz_lines(self):
        lines = []
        if not self._xyz:
            return lines
        for sample, fields in self._sample_data.data_dic.items():
            lines += self._get_xyz_field_string(sample)
            for sample_type in ["nuclear_coh", "magnetic", "spin_incoh"]:
                lines += [self._get_to_matrix_string(sample, sample_type)]
                self._plot_list.append(f"mat_{sample}_{sample_type}")
                lines += [self._get_sep_save_string(sample, sample_type)]
        return lines

    def _get_nsf_sf_pairs(self):
        nsf_sf_pairs = []
        for sample, fields in self._sample_data.data_dic.items():
            for field in fields.keys():
                if field.endswith("_sf") and (f"{field[:-2]}nsf" in fields.keys()):
                    nsf_sf_pairs.append(f"{sample}_{field[:-3]}")
        return nsf_sf_pairs

    def _get_non_magnetic_lines(self):
        lines = []
        if not self._non_magnetic:
            return lines

        nsf_sf_pairs = self._get_nsf_sf_pairs()
        lines += ["# separation of coherent and incoherent scattering of non magnetic sample"]
        for sample_field in sorted(nsf_sf_pairs):
            # normally only z is measured but just in case
            lines += [
                f"{sample_field}_nuclear_coh, {sample_field}_spin_incoh = "
                "non_magnetic_separation"
                f"('{sample_field}_sf', '{sample_field}_nsf')"
            ]
            for sample_type in ["nuclear_coh", "spin_incoh"]:
                lines += [self._get_to_matrix_string(sample_field, sample_type)]
                self._plot_list.append(f"mat_{sample_field}_{sample_type}")
                lines += [self._get_sep_save_string(sample_field, sample_type)]

        return lines

    def _setup_sample_data(self, paths, file_selector):
        self._sample_data = DNSElasticDataset(data=file_selector["full_data"], path=paths["data_dir"], is_sample=True)
        self._plot_list = ["mat_" + x for x in self._sample_data.create_subtract()]

    def _setup_standard_data(self, paths, file_selector):
        if self._corrections:
            self._standard_data = DNSElasticDataset(
                data=file_selector["standard_data_tree_model"],
                path=paths["standards_dir"],
                is_sample=False,
                banks=self._sample_data["banks"],
                fields=self._sample_data["fields"],
                ignore_van=self._ignore_vana,
            )

    @staticmethod
    def _get_header_lines():
        lines = [
            "from mantidqtinterfaces.dns_powder_elastic.scripts.md_powder_elastic import load_all, background_subtraction",
            "from mantidqtinterfaces.dns_powder_elastic.scripts.md_powder_elastic import vanadium_correction, flipping_ratio_correction",
            "from mantidqtinterfaces.dns_powder_elastic.scripts.md_powder_elastic import non_magnetic_separation, xyz_separation",
            "from mantid.simpleapi import ConvertMDHistoToMatrixWorkspace, mtd",
            "from mantid.simpleapi import SaveAscii, SaveNexus",
            "",
        ]
        return lines

    def _get_sample_data_lines(self):
        return [f"sample_data = {self._sample_data.format_dataset()}"]

    def _get_standard_data_lines(self):
        if self._corrections:
            return [f"standard_data = {self._standard_data.format_dataset()}"]
        return [""]

    def _get_binning_lines(self, options):
        bin_edge_min = options["two_theta_min"] - options["two_theta_bin_size"] / 2
        bin_edge_max = options["two_theta_max"] + options["two_theta_bin_size"] / 2
        num_bins = int(round((bin_edge_max - bin_edge_min) / options["two_theta_bin_size"]))
        binning = [bin_edge_min, bin_edge_max, num_bins]
        return ["", f"binning = {binning}"]

    def _get_load_data_lines(self):
        lines = [f"wss_sample = load_all(sample_data, binning, normalize_to='{self._norm}')"]
        if self._corrections:
            lines += [f"wss_standard = load_all(standard_data, binning, normalize_to='{self._norm}')"]
        return lines

    def _set_loop(self):
        if len(self._sample_data.keys()) == 1:
            self._loop = f"for workspace in wss_sample['{list(self._sample_data.keys())[0]}']:"
            self._spacing = "\n" + " " * 4
        else:
            self._loop = "for sample, workspacelist in wss_sample.items(): \n    for workspace in workspacelist:"
            self._spacing = "\n" + " " * 8

    def _get_subtract_bg_from_standard_lines(self):
        if self._vana_correction or self._nicr_correction:
            return [
                "",
                "# subtract background from vanadium and nicr",
                "for sample, workspacelist in wss_standard.items(): "
                "\n    for workspace in workspacelist:"
                "\n        background_subtraction(workspace)",
            ]
        return []

    def _get_background_string(self):
        if self._sample_background_correction:
            return f"{self._spacing}background_subtraction(workspace, factor={self._background_factor})"
        return ""

    def _get_vana_correction_string(self):
        if self._vana_correction:
            return (
                f"{self._spacing}vanadium_correction(workspace,"
                " binning, "
                "vana_set=standard_data['vana'], "
                f"ignore_vana_fields={self._ignore_vana}, "
                f"sum_vana_sf_nsf={self._sum_sf_nsf})"
            )
        return ""

    def _get_sample_corrections_lines(self):
        background_string = self._get_background_string()
        vana_correction_string = self._get_vana_correction_string()
        if self._sample_background_correction or self._vana_correction:
            return ["", "# correct sample data", f"{self._loop}{background_string}{vana_correction_string}"]
        return []

    def _get_nicr_cor_lines(self):
        if self._nicr_correction:
            return [f"{self._loop}{self._spacing}flipping_ratio_correction(workspace)"]
        return []

    def _get_ascii_save_string(self):
        if self._ascii:
            return (
                f"{self._spacing}"
                "SaveAscii('mat_{}'.format(workspace), "
                f"'{self._export_path}/"
                "{}.csv'.format(workspace)"
                ", WriteSpectrumID=False)"
            )
        return ""

    def _get_nexus_save_string(self):
        if self._nexus:
            return f"{self._spacing}SaveNexus('mat_{{}}'.format(workspace), '{self._export_path}/{{}}.nxs'.format(workspace))"
        return ""

    def _get_convert_to_matrix_lines(self, save_string):
        lines = [
            "",
            "# convert the output sample MDHistoWorkspaces to MatrixWorkspaces",
            f"{self._loop}{self._spacing}ConvertMDHistoToMatrixWorkspace"
            "(workspace, Outputworkspace='mat_{}'.format(workspace), "
            f"Normalization='NoNormalization', FindXAxis=False){save_string}",
            "",
        ]
        return lines

    def _get_save_string(self):
        ascii_string = self._get_ascii_save_string()
        nexus_string = self._get_nexus_save_string()
        save_strings = [x for x in [ascii_string, nexus_string] if x]
        return "".join(save_strings)

    def script_maker(self, options, paths, file_selector=None):
        self._script = []

        # options shortcut
        self._vana_correction = options["corrections"] and options["det_efficiency"]
        self._nicr_correction = options["corrections"] and options["flipping_ratio"]
        self._sample_background_correction = options["corrections"] and options["subtract_background_from_sample"]
        self._background_factor = options["background_factor"]
        self._ignore_vana = str(options["ignore_vana_fields"])
        self._sum_sf_nsf = str(options["sum_vana_sf_nsf"])
        self._non_magnetic = options["separation"] and options["separation_coh_inc"]
        self._xyz = options["separation"] and options["separation_xyz"]
        self._corrections = self._sample_background_correction or self._vana_correction or self._nicr_correction
        self._export_path = paths["export_dir"]
        self._ascii = paths["ascii"] and paths["export"] and bool(self._export_path)
        self._nexus = paths["nexus"] and paths["export"] and bool(self._export_path)
        self._norm = get_normalisation(options)

        self._setup_sample_data(paths, file_selector)
        self._setup_standard_data(paths, file_selector)
        self._set_loop()
        # validate if input makes sense, otherwise return
        # an empty script and error message
        error = self._check_errors_in_selected_files()
        if error:
            self._script = [""]
            error_message = error
        else:
            error_message = ""
            # starting writing script
            self._add_lines_to_script(self._get_header_lines())
            self._add_lines_to_script(self._get_sample_data_lines())
            self._add_lines_to_script(self._get_standard_data_lines())
            self._add_lines_to_script(self._get_binning_lines(options))
            self._add_lines_to_script(self._get_load_data_lines())
            self._add_lines_to_script(self._get_subtract_bg_from_standard_lines())
            self._add_lines_to_script(self._get_sample_corrections_lines())
            self._add_lines_to_script(self._get_nicr_cor_lines())
            save_string = self._get_save_string()
            self._add_lines_to_script(self._get_convert_to_matrix_lines(save_string))
            self._add_lines_to_script(self._get_xyz_lines())
            self._add_lines_to_script(self._get_non_magnetic_lines())

        return self._script, error_message

    def get_plot_list(self):
        return self._plot_list

    def _vana_not_in_standard(self):
        return "vana" not in self._standard_data.data_dic.keys()

    def _nicr_not_in_standard(self):
        return "nicr" not in self._standard_data.data_dic.keys()

    def _empty_not_in_standard(self):
        return "empty" not in self._standard_data.data_dic.keys()

    def _xyz_separation_not_possible(self):
        selected_angle_fields_data = self._sample_data["angle_fields_data"]
        separation_possibility_list = []
        incomplete_banks_list = []
        for det_bank_angle in selected_angle_fields_data.keys():
            separation_possible = self._check_xyz_separation(selected_angle_fields_data[det_bank_angle])
            separation_possibility_list.append(separation_possible)
            if not separation_possible:
                incomplete_banks_list.append(det_bank_angle)
        is_possible = all(separation_possibility_list)
        angles = ", ".join(map(str, sorted(incomplete_banks_list)))
        return not is_possible, angles

    def _coh_incoh_separation_not_possible(self):
        selected_angle_fields_data = self._sample_data["angle_fields_data"]
        coh_incoh_separation_possibility_list = []
        incomplete_banks_list = []
        for det_bank_angle in selected_angle_fields_data.keys():
            sf_fields = sorted([field for field in selected_angle_fields_data[det_bank_angle] if field.endswith("_sf")])
            sf_fields_components = [field.replace("_sf", "") for field in sf_fields]
            nsf_fields = sorted([field for field in selected_angle_fields_data[det_bank_angle] if field.endswith("_nsf")])
            nsf_fields_components = [field.replace("_nsf", "") for field in nsf_fields]
            if nsf_fields_components == sf_fields_components:
                coh_incoh_separation_possible = True
            else:
                coh_incoh_separation_possible = False
                incomplete_banks_list.append(det_bank_angle)
            coh_incoh_separation_possibility_list.append(coh_incoh_separation_possible)
        is_possible = all(coh_incoh_separation_possibility_list)
        angles = ", ".join(map(str, sorted(incomplete_banks_list)))
        return not is_possible, angles

    def _flipping_ratio_not_possible(self):
        selected_angle_fields_data = self._sample_data["angle_fields_data"]
        flipping_ratio_possibility_list = []
        incomplete_banks_list = []
        for det_bank_angle in selected_angle_fields_data.keys():
            sf_fields = sorted([field for field in selected_angle_fields_data[det_bank_angle] if field.endswith("_sf")])
            sf_fields_components = [field.replace("_sf", "") for field in sf_fields]
            nsf_fields = sorted([field for field in selected_angle_fields_data[det_bank_angle] if field.endswith("_nsf")])
            nsf_fields_components = [field.replace("_nsf", "") for field in nsf_fields]
            if nsf_fields_components == sf_fields_components:
                flipping_ratio_possible = True
            else:
                flipping_ratio_possible = False
                incomplete_banks_list.append(det_bank_angle)
            flipping_ratio_possibility_list.append(flipping_ratio_possible)
        is_possible = all(flipping_ratio_possibility_list)
        angles = ", ".join(map(str, sorted(incomplete_banks_list)))
        return not is_possible, angles

    def _bank_positions_not_compatible(self, tol=0.05):
        if self._corrections:
            sorted_sample_banks = np.array(sorted(self._sample_data["banks"]))
            sorted_standard_banks = np.array(sorted(self._standard_data["banks"]))
            sample_banks_size = sorted_sample_banks.size
            standard_banks_size = sorted_standard_banks.size
            if sample_banks_size == standard_banks_size:
                banks_match = np.allclose(sorted_sample_banks, sorted_standard_banks, atol=tol)
            elif sample_banks_size < standard_banks_size:
                count = 0
                for sample_element in sorted_sample_banks:
                    for standard_element in sorted_standard_banks:
                        if np.allclose(sample_element, standard_element, atol=tol):
                            print(sample_element)
                            count += 1
                if count == sample_banks_size:
                    banks_match = True
                else:
                    banks_match = False
            else:
                banks_match = False
            return not banks_match

    def _check_errors_in_selected_files(self):
        """
        If any inconsistencies are found in the files selected for
        data reduction, then this method will return a string with
        the corresponding error_message. If no inconsistencies are
        found then the empty string will be returned.
        """
        if self._corrections and not self._standard_data:
            return "Standard data have to be selected to perform reduction of sample data."
        if self._bank_positions_not_compatible():
            return "Detector rotation angles of the selected standard data do not match the angles of the selected sample data."
        if self._vana_correction and self._vana_not_in_standard():
            return (
                "Detector efficiency correction option is chosen, "
                "but detector bank rotation angles of the selected "
                "vanadium files do not correspond to those of the "
                "selected sample files."
            )
        if self._vana_correction and self._empty_not_in_standard():
            return (
                "Detector efficiency correction option is chosen, "
                'but detector bank rotation angles of the selected "empty" '
                "files do not correspond to those of the selected "
                "sample files."
            )
        if self._nicr_correction and self._nicr_not_in_standard():
            return (
                "Flipping ratio correction option is chosen, but "
                "detector bank rotation angles of the selected NiCr "
                "files do not correspond to those of the selected "
                "sample files."
            )
        if self._nicr_correction and self._empty_not_in_standard():
            return (
                "Flipping ratio correction option is chosen, but "
                'detector bank rotation angles of the selected "empty" '
                "files do not correspond to those of the selected "
                "sample files."
            )
        if self._sample_background_correction and self._empty_not_in_standard():
            return (
                "Background subtraction from sample is chosen, but "
                'detector bank rotation angles of the selected "empty" '
                "files do not correspond to those of the selected "
                "sample files."
            )
        if self._nicr_correction and self._flipping_ratio_not_possible()[0]:
            return (
                "Flipping ratio correction option is chosen, but "
                "an incomplete set of pairs of SF and NSF measurements "
                "is found for the following bank rotation angles: "
                f"{self._flipping_ratio_not_possible()[1]}"
            )
        if self._non_magnetic and self._coh_incoh_separation_not_possible()[0]:
            return (
                "No complete pairs of SF and NSF measurements are "
                "found for performing separation of coherent and "
                "incoherent contributions for the selected sample "
                "files for for the following bank rotation angles: "
                f"{self._coh_incoh_separation_not_possible()[1]}."
            )
        if self._xyz and self._xyz_separation_not_possible()[0]:
            return (
                "Not all fields necessary for performing XYZ "
                "separation analysis are found in the selected "
                "sample files for the following bank rotation "
                f"angles: {self._xyz_separation_not_possible()[1]}."
            )
        return ""
