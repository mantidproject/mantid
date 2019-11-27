# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS script generator for elastic powder data
"""

from __future__ import (absolute_import, division, print_function)
import numpy as np

from DNSReduction.script_generator.common_script_generator_presenter import DNSScriptGenerator_presenter
from DNSReduction.script_generator.common_script_generator_presenter import list_to_multirange
from DNSReduction.data_structures.field_names import field_dict


class DNSElasticPowderScriptGenerator_presenter(DNSScriptGenerator_presenter):

    # pass the view and model into the presenter
    def __init__(self, parent):
        super(DNSElasticPowderScriptGenerator_presenter,
              self).__init__(parent, 'elastic_powder_script_generator')
        self.script = None

    def create_dataset(self, data):
        dataset = {}
        for entry in data:
            if entry['samplename'] == 'leer':  ## compatibility with old names
                datatype = 'empty'
            else:
                datatype = entry['samplename'].strip(' _')
            field = field_dict.get(entry['field'], entry['field'])
            datapath = entry['filename'].replace(
                '_' + str(entry['filenumber']) + '.d_dat', '')
            if datatype in dataset.keys():
                if field in dataset[datatype].keys():
                    dataset[datatype][field].append(entry['filenumber'])
                else:
                    dataset[datatype][field] = [entry['filenumber']]
            else:
                dataset[datatype] = {}
                dataset[datatype][field] = [entry['filenumber']]
                dataset[datatype]['path'] = datapath
        for sample_typ, fields in dataset.items():
            for field, filenumbers in fields.items():
                if field != 'path':
                    dataset[sample_typ][field] = list_to_multirange(
                        filenumbers)
        return dataset

    def format_dataset(self, dataset):
        if len(dataset.keys()):
            longest_samplename = max([len(a) for a in dataset.keys()])
        else:
            longest_samplename = ''
        dataset_string = '{'
        for samplename, fields in dataset.items():
            if not dataset_string.endswith('{'):
                dataset_string += ",\n "
            else:
                dataset_string += "\n "
            dataset_string += " " * (longest_samplename - len(samplename)
                                    ) + "'{:s}' : {{".format(samplename)
            spacing = len(" " * (longest_samplename - len(samplename)) +
                          "'{:s}' : {{".format(samplename)) + 1
            dataset_string += "'path'  : '{}'".format(fields['path'])
            for field, filenumbers in sorted(fields.items()):
                if field != 'path':
                    dataset_string += ",\n" + " " * spacing + "'{:s}'".format(
                        field) + " " * (5 - len(field))
                    dataset_string += " : {}".format(filenumbers)
            dataset_string += "}"
        dataset_string += "}"
        return dataset_string

    def script_maker(self):
        self.script = [""]
        options = self.param_dict['elastic_powder_options']

        def l(line=""):
            self.script += [line]

        sample_data = self.param_dict['file_selector']['full_data']

        det_rot = [entry['det_rot'] for entry in sample_data]
        diff_rot = [
            abs(det_rot[i] - det_rot[i + 1]) for i in range(len(det_rot) - 1)
        ]
        diff_rot = [i for i in diff_rot if i >= 0.03]
        avg_diff = np.average(diff_rot)
        if avg_diff <= 0.1 or not diff_rot:
            avg_diff = 1
        stepfactor = round(5 / avg_diff,
                           0) / 5  # stepsize correction for dns uncertainty
        twothetastep = 1 / stepfactor
        twotheta_max = -min(det_rot) + 115
        twotheta_min = -max(det_rot)
        numberofbins = int(
            round((twotheta_max - twotheta_min) * stepfactor + 1, 0))
        twotheta_min = twotheta_min - twothetastep / 2.0
        twotheta_max = twotheta_max + twothetastep / 2.0
        standard_data = self.param_dict['file_selector']['standard_data']
        standard_data = self.create_dataset(standard_data)
        sample_data = self.create_dataset(sample_data)
        importstring = 'from DNSReduction.scripts.md_powder_elastic import ' \
                       'load_all, background_substraction'
        if options['corrections'] and (options['det_efficency']
                                       or options['flipping_ratio']):
            importstring += ', vanadium_correction'
        if options['corrections'] and (options['det_efficency']
                                       or options['flipping_ratio']):
            importstring += ', fliping_ratio_correction'
        if options["separation"] and options['separation_coh_inc']:
            importstring += ", non_mag_sep"
        if options["separation"] and options['separation_xyz']:
            importstring += ', xyz_seperation'
        l(importstring)
        l('from mantid.simpleapi import ConvertMDHistoToMatrixWorkspace, mtd')
        l('sample_data = {}'.format(self.format_dataset(sample_data)))
        l('standard_data = {}'.format(self.format_dataset(standard_data)))
        l()
        if options['norm_monitor']:
            normalizestring = "normalizeto='monitor'"
        else:
            normalizestring = "normalizeto='time'"
        l("binning = ['twoTheta' , {:.2f}, {:.2f}, {:d}] # min, max,"\
          " number_of_bins"
          .format(twotheta_min, twotheta_max, numberofbins))
        l("wss_sample = load_all(sample_data, binning, {})".format(
            normalizestring))
        l("wss_standard = load_all(standard_data, binning, {})".format(
            normalizestring))
        l()
        l("## substract background from vanadium and nicr")
        l("for sample, workspacelist in wss_standard.items(): " +
          "\n    for workspace in workspacelist:" +
          "\n        background_substraction(workspace)")
        l()
        l("## correct sample data")
        correctionstring = ''
        if (options['substract_background_from_sample']
                or (options['corrections'] and options['det_efficency'])):
            if len(sample_data.keys()) == 1:
                samplename = sample_data.keys()[0]
                spacing = 4
                correctionstring = "for workspace in wss_sample['{}']:".format(
                    samplename)
            else:
                spacing = 8
                correctionstring = (
                    "for sample, workspacelist in wss_sample.items(): " +
                    "\n    for workspace in workspacelist:")
        if options['substract_background_from_sample']:
            if options['background_factor'] != 1:
                factorstring = ", factor={}".format(
                    options['background_factor'])
            else:
                factorstring = ""
            correctionstring += "\n" + spacing * " " + "background_substracti"\
                "on(workspace{})".format(factorstring)
        if options['corrections'] and options['det_efficency']:
            if options['det_efficency']:
                correctionstring += "\n" + spacing * " " + "vanadium_correc" \
                    "tion(workspace, vanaset=standard_data['vana']"
                if options['ignore_vana_fields']:
                    correctionstring += " ,ignore_vana_fields=True"
                if options['sum_vana_sf_nsf']:
                    correctionstring += ", sum_vana_sf_nsf=True"
                correctionstring += ")"
        l(correctionstring)
        if options['corrections'] and options['flipping_ratio']:
            if len(sample_data.keys()) == 1:
                samplename = sample_data.keys()[0]
                spacing = 4
                correctionstring = "for workspace in wss_sample['{}']:".format(
                    samplename)
            else:
                spacing = 8
                correctionstring = (
                    "for sample, workspacelist in wss_sample.items(): " +
                    "\n    for workspace in workspacelist:")
            correctionstring += "\n" + spacing * " " + "fliping_ratio_corr" \
                "ection(workspace)"
        l(correctionstring)
        if len(sample_data.keys()) == 1:
            samplename = sample_data.keys()[0]
            spacing = 4
            correctionstring = "for workspace in wss_sample['{}']:".format(
                samplename)
        else:
            spacing = 8
            correctionstring = (
                "for sample, workspacelist in wss_sample.items(): " +
                "\n    for workspace in workspacelist:")
        correctionstring += "\n" + spacing * " " + "ConvertMDHistoToMatrix" \
            "Workspace(workspace, Outputworkspace='mat_' + workspace, " \
            "Normalization='NoNormalization')"
        l(correctionstring)
        if options["separation"] and options['separation_xyz']:
            l()
            l("## do xyz polarization analysis for magnetic powders")
            for sample, fields in sample_data.items():
                if not all(field in fields
                           for field in ['x_sf', 'y_sf', 'z_sf', 'z_nsf']):
                    self.raise_error(
                        'Not all fields necesarry found for XYZ analysis.')
                else:
                    l("{0}_nuclear_coh, {0}_magnetic, {0}_spin_incoh = xyz_" \
                      "seperation(x_sf='{0}_x_sf',"
                      .format(sample) + "\n" + " " * (len(sample) + 64) +
                      "y_sf='{}_y_sf',".format(sample) + "\n" + " " *
                      (len(sample) + 64) + "z_sf='{}_z_sf',".format(sample) +
                      "\n" + " " * (len(sample) + 64) +
                      "z_nsf='{}_z_nsf')".format(sample))
                    l("ConvertMDHistoToMatrixWorkspace('{0}_nuclear_coh', O" \
                      "utputworkspace='mat_{0}_nuclear_coh', Normalization=" \
                      "'NoNormalization')"
                      .format(sample))
                    l("ConvertMDHistoToMatrixWorkspace('{0}_magnetic', Outpu" \
                      "tworkspace='mat_{0}_magnetic', Normalization='NoNorma" \
                      "lization')"
                      .format(sample))
                    l("ConvertMDHistoToMatrixWorkspace('{0}_spin_incoh', Out" \
                      "putworkspace='mat_{0}_spin_incoh', Normalization='NoN" \
                      "ormalization')"
                      .format(sample))
        if options["separation"] and options['separation_coh_inc']:
            nsf_sf_pairs = []
            l()
            for sample, fields in sample_data.items():
                for field in fields.keys():
                    if field.endswith(
                            '_sf'
                    ) and field[:-2] + 'nsf' in sample_data[sample].keys():
                        nsf_sf_pairs.append("{}_{}".format(sample, field[:-3]))
                if not nsf_sf_pairs:
                    self.raise_error(
                        'No pair of SF and NSF measurements found for {}'.
                        format(sample))
            l("## sepearation of coherent and incoherent scattering of non" \
              " magnetic sample"
             )
            for sample_field in sorted(
                    nsf_sf_pairs
            ):  # normally only z is measured but just in case
                l("{0}_nuc_coherent, {0}_spin_incoherent = non_mag_sep" \
                  "('{0}_sf', '{0}_nsf')"
                  .format(sample_field))

        return self.script
