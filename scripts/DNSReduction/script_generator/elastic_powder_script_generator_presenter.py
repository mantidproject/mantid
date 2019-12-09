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
        self.plotlist = []

    def create_dataset(self, data, sample=False):
        """Converting data from fileselector to a smaller dictionary """
        dataset = {}
        for entry in data: ## in case sample is standard datafile
            if (sample
                    and entry['samplename'] in ['leer',
                                                'empty',
                                                'vana',
                                                'nicr']):
                datatype = 'sample_{}'.format(entry['samplename'].strip(' _'))
            elif entry['samplename'] == 'leer':  # compatibility with old names
                datatype = 'empty'
            else:
                datatype = entry['samplename'].strip(' _')
            field = field_dict.get(entry['field'], entry['field'])
            datapath = entry['filename'].replace(
                '_{}.d_dat'.format(entry['filenumber']), '')
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

    def create_plotlist(self, sample_data):
        plotlist= []
        for sample, workspacelist in sample_data.items():
            for workspace in workspacelist:
                if workspace != 'path':
                    plotlist.append("mat_{}_{}".format(sample, workspace))
        return plotlist

    def format_dataset(self, dataset):
        """Formating the dictionary to a nicely indented string"""
        llens = max([len(a) for a in dataset.keys()] +[0])+6+4
        dataset_string = '{\n'
        for samplename, fields in dataset.items():
            lmax = max([len(key) for key in fields.keys()] +[0])

            dataset_string += "'{:s}' : {{".format(samplename).rjust(llens)

            dataset_string += "{}'path' : '{}',\n"\
                "".format(" "*(lmax -4), dataset[samplename].pop('path'))
            dataset_string += ",\n".join(
                [("{}'{:s}' : {}").format(" "*(lmax - len(key) + llens),
                                          key,
                                          value)
                 for key, value in sorted(fields.items())])
            dataset_string += "},\n"
        dataset_string += "}"
        return dataset_string

    def get_normation(self, options):
        if options['norm_monitor']:
            return 'monitor'
        return 'time'

    def script_maker(self): # noqa: C901
        self.script = []
        options = self.param_dict['elastic_powder_options']
        paths = self.param_dict['paths']

        def l(line=""):
            self.script += [line]

        ### get data from dictionary
        sample_data = self.param_dict['file_selector']['full_data']
        standard_data = self.param_dict['file_selector']['standard_data']

        ## shortcuts for options
        vanac = options['corrections'] and options['det_efficency']
        nicrc = options['corrections'] and options['flipping_ratio']
        sampb = options['substract_background_from_sample']
        backfac = options['background_factor']
        ign_vana = str(options['ignore_vana_fields'])
        sum_sfnsf = str(options['sum_vana_sf_nsf'])
        nonmag = options["separation"] and options['separation_coh_inc']
        xyz = options["separation"] and options['separation_xyz']
        backgroundstring = ''
        vanacstring = ''
        export_path = paths["export_dir"]
        ascii = paths["ascii"] and paths["export"] and export_path
        nexus = paths["nexus"] and paths["export"]  and export_path
        ## normation
        norm = self.get_normation(options)
        ### binning determination, using dns uncertainty
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
        ## end binning

        ## making smaller dictrionary for printing
        sample_data = self.create_dataset(sample_data, sample=True)
        standard_data = self.create_dataset(standard_data)

        self.plotlist = self.create_plotlist(sample_data)
        ## loop over multiple or single samples, spac is indention for rest
        if len(sample_data.keys()) == 1:
            loop = "for workspace in wss_sample['{}']:"\
                   "".format(sample_data.keys()[0])
            spac = "\n" + " "*4
        else:
            loop = "for sample, workspacelist in wss_sample.items(): "\
                   "\n    for workspace in workspacelist:"
            spac = "\n" + " "*8

    ##### starting to write file

        ## imports
        l('from DNSReduction.scripts.md_powder_elastic import '
          'load_all, background_substraction'
          '\nfrom DNSReduction.scripts.md_powder_elastic import '
          'vanadium_correction, fliping_ratio_correction'
          '\nfrom DNSReduction.scripts.md_powder_elastic import '
          'non_mag_sep, xyz_seperation')
        l('from mantid.simpleapi import ConvertMDHistoToMatrixWorkspace, mtd')
        l('from mantid.simpleapi import SaveAscii, SaveNexus')
        ## formated sample dict
        l('sample_data = {}'.format(self.format_dataset(sample_data)))
        l('standard_data = {}'.format(self.format_dataset(standard_data)))
        if ascii or nexus:
            l("exportpath = '{}'".format(export_path))
        l()

        ## binning
        l("binning = ['twoTheta' , {:.2f}, {:.2f}, {:d}] # min, max,"
          " number_of_bins".format(twotheta_min,
                                   twotheta_max,
                                   numberofbins))
        l("wss_sample = load_all(sample_data, binning, normalizeto='{}')"
          "".format(norm))
        l("wss_standard = load_all(standard_data, binning, normalizeto='{}',"
          "standard=True)".format(norm))
        l()

        ## background substraction from standard data
        if vanac or nicrc:
            l("## substract background from vanadium and nicr"
              "\nfor sample, workspacelist in wss_standard.items(): "
              "\n    for workspace in workspacelist:"
              "\n        background_substraction(workspace)")
            l()

        if sampb:
            backgroundstring = "{}background_substraction(workspace, "\
                               "factor={})".format(spac, backfac)

        if vanac:
            vanacstring = "{}vanadium_correction(workspace, binning=binning,"\
                          " vanaset=standard_data['vana'], " \
                          "ignore_vana_fields={}, " \
                          "sum_vana_sf_nsf={})".format(spac,
                                                       ign_vana,
                                                       sum_sfnsf)

        ## sample background sumstraction + vanadium correction
        if sampb or vanac:
            l('## correct sample data')
            l("{}{}{}".format(loop, backgroundstring, vanacstring))

        # Nicr correction
        if nicrc:
            l("{}{}fliping_ratio_correction(workspace)".format(loop, spac))

        ## saving
        saving_list = []
        if ascii:
            saving_list.append("{0}SaveAscii('mat_{{}}'.format(workspace), "
                               "'{{}}/{{}}.csv'.format(exportpath, workspace)"
                               ", WriteSpectrumID=False)"
                               "".format(spac))
        if nexus:
            saving_list.append("{0}SaveNexus('mat_{{}}'.format(workspace), "
                               "'{{}}/{{}}.nxs'.format(exportpath,"
                               " workspace))".format(spac))

        ## converting to matrix for plotting
        l("{0}{1}ConvertMDHistoToMatrixWorkspace(workspace,"
          " Outputworkspace='mat_{{}}'.format(workspace), "
          "Normalization='NoNormalization'){2}"
          "".format(loop, spac, "\n".join(saving_list)))
        ## saving raw data

        ### xyz polarization analysis for magnetic samples
        if xyz:
            for sample, fields in sample_data.items():
                if not all(field in fields
                           for field in ['x_sf', 'y_sf', 'z_sf', 'z_nsf']):
                    self.raise_error("Not all fields necesarry found for XYZ"
                                     " analysis.")
                else:
                    l()
                    l("## do xyz polarization analysis for magnetic powders")

                    l("{0}_nuclear_coh, {0}_magnetic, {0}_spin_incoh = xyz_"
                      "seperation(x_sf='{0}_x_sf',"
                      "{1}{2}y_sf='{0}_y_sf',"
                      "{1}{2}z_sf='{0}_z_sf',"
                      "{1}{2}z_nsf='{0}_z_nsf')"
                      "".format(sample, '\n', " "*(len(sample) + 64)))

                    for samp_type in ['nuclear_coh',
                                      'magnetic',
                                      'spin_incoh']:
                        l("ConvertMDHistoToMatrixWorkspace('{0}_{1}', "
                          "Outputworkspace='mat_{0}_{1}',"
                          " Normalization='NoNormalization')"
                          "".format(sample, samp_type))
                        self.plotlist.append('mat_{0}_{1}'
                                             ''.format(sample, samp_type))
                        if ascii:
                            l("SaveAscii('mat_{0}_{1}', '{2}/{0}_{1}.csv', "
                              "WriteSpectrumID=False)"
                              "".format(sample, samp_type, export_path))
                        if nexus:
                            l("SaveNexus('mat_{0}_{1}', '{2}/{0}_{1}.nxs')"
                              "".format(sample, samp_type, export_path))
        ### nuc_coherent + spin_incoherent seperation for non-magnetic samples
        if nonmag:
            nsf_sf_pairs = []
            l()
            for sample, fields in sample_data.items():
                for field in fields.keys():
                    if (field.endswith('_sf')
                            and ('{}nsf'.format(field[:-2])
                                 in sample_data[sample].keys())):
                        nsf_sf_pairs.append("{}_{}".format(sample, field[:-3]))
                if not nsf_sf_pairs:
                    self.raise_error("No pair of SF and NSF measurements"
                                     " found for {}".format(sample))
            l("## sepearation of coherent and incoherent scattering of non"
              " magnetic sample")
            for sample_field in sorted(nsf_sf_pairs):
                # normally only z is measured but just in case
                l("{0}_nuclear_coh, {0}_spin_incoh = non_mag_sep"
                  "('{0}_sf', '{0}_nsf')".format(sample_field))
                for samp_type in ['_nuclear_coh', '_spin_incoh']:
                    l("ConvertMDHistoToMatrixWorkspace('{0}{1}', "
                      "Outputworkspace='mat_{0}{1}', Normalization="
                      "'NoNormalization')".format(sample_field, samp_type))
                    self.plotlist.append('mat_{0}{1}'
                                         ''.format(sample_field, samp_type))
                    if ascii:
                        l("SaveAscii('mat_{0}{1}', '{2}/{0}{1}.csv', "
                          "WriteSpectrumID=False)"
                          "".format(sample_field, samp_type, export_path))
                        if nexus:
                            l("SaveNexus('mat_{0}{1}', '{2}/{0}{1}.nxs')"
                              "".format(sample_field, samp_type, export_path))
        return self.script

    def get_option_dict(self):
        if self.view is not None:
            self.own_dict.update(self.view.get_state())
        self.own_dict['script_path'] = self.scriptpath
        self.own_dict['script_number'] = self.script_number
        self.own_dict['script_text'] = self.scripttext
        self.own_dict['plotlist'] = self.plotlist
        return self.own_dict
