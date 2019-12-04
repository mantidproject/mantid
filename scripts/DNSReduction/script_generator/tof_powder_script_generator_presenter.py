# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS script generator for TOF powder data
"""

from __future__ import (absolute_import, division, print_function)

from DNSReduction.script_generator.common_script_generator_presenter import DNSScriptGenerator_presenter
from DNSReduction.script_generator.common_script_generator_presenter import list_to_multirange


def create_dataset(data):
    dataset = {}
    rounding_limit = 0.05
    for entry in data:
        if entry['samplename'] == 'leer':  ## compatibility with old names
            datatype = 'empty'
        else:
            datatype = entry['samplename']
        det_rot = entry['det_rot']
        datapath = entry['filename'].replace(
            '_' + str(entry['filenumber']) + '.d_dat', '')
        if datatype in dataset.keys():
            for compare in [
                    x for x in dataset[datatype].keys() if x != 'path'
            ]:
                if abs(compare - det_rot) < rounding_limit:
                    inside = True
                    break
                else:
                    inside = False
            if inside:
                dataset[datatype][compare].append(entry['filenumber'])
            else:
                dataset[datatype][det_rot] = [entry['filenumber']]
        else:
            dataset[datatype] = {}
            dataset[datatype][det_rot] = [entry['filenumber']]
            dataset[datatype]['path'] = datapath
    for sample_typ, det_rots in dataset.items():
        for det_rot, filenumbers in det_rots.items():
            if det_rot != 'path':
                dataset[sample_typ][det_rot] = list_to_multirange(filenumbers)
    return dataset


def format_dataset(dataset):
    """Formating the dictionary to a nicely indented string"""
    llens = max([len(a) for a in dataset.keys()])+6+4
    dataset_string = '{\n'
    for samplename, fields in dataset.items():
        dataset_string += "'{:s}' : {{".format(samplename).rjust(llens)

        dataset_string += "{}'path' : '{}',\n"\
            "".format(" "*(0), dataset[samplename].pop('path'))
        dataset_string += ",\n".join(
            [("{}{:6.2f} : {}").format(" "*llens,
                                       key,
                                       value)
             for key, value in sorted(fields.items())])
        dataset_string += "},\n"
    dataset_string += "}"
    return dataset_string


class DNSTofPowderScriptGenerator_presenter(DNSScriptGenerator_presenter):

    # pass the view and model into the presenter
    def __init__(self, parent):
        super(DNSTofPowderScriptGenerator_presenter,
              self).__init__(parent, 'tof_powder_script_generator')
        self.script = None
        self.number_of_banks = None
        self.number_of_vana_banks = None
        self.number_of_empty_banks = None

    def get_vana_filename(self, standard_data):
        vanafilename = [x for x in standard_data.keys() if '_vana' in x]
        if len(vanafilename) > 1:
            self.raise_error('Only one Vandium filename allowed',
                             critical=True)
        else:
            vanafilename = vanafilename[0]
        return vanafilename

    def get_empty_filename(self, standard_data):
        emptyfilename = [x for x in standard_data.keys() if ('_empty' in x
                                                             or '_leer' in x)
                         ]
        if len(emptyfilename) > 1:
            self.raise_error('Only one Empty filename allowed', critical=True)
        else:
            emptyfilename = emptyfilename[0]
        return emptyfilename

    def get_sample_filename(self, sample_data):
        samplefilename = [x for x in sample_data.keys()]
        if len(samplefilename) > 1:
            self.raise_error('Only one Sample data filename allowed',
                             critical=True)
        else:
            samplefilename = samplefilename[0]
        return samplefilename

    def check_vanadium_correction(self, tof_opt):
        if (self.number_of_vana_banks > 0
                and tof_opt['corrections']
                and tof_opt['det_efficency']):
            return True
        return False

    def check_background_correction(self, tof_opt):
        if not (self.number_of_empty_banks > 0
                and tof_opt['corrections']):
            return False
        elif self.number_of_banks == 0:
            self.raise_error('No data selected.', critical=True)
            return False
        else:
            return True
<<<<<<< HEAD

=======
>>>>>>> 2522bdd228fcb0328e32ba294815076378dfc2d8
    def validate_input(self, tof_opt):
        if (tof_opt['dEstep'] == 0
                or tof_opt['qstep'] == 0
                or tof_opt['qmax'] <= tof_opt['qmin']
                or tof_opt['dEmax'] <= tof_opt['dEmin']):
            self.raise_error('Bin sizes make no sense.', critical=True)
            return False
        if (self.number_of_vana_banks == 0
                and tof_opt['corrections']
                and tof_opt['det_efficency']):
            self.raise_error('No vanadium files selected, but'
                             'Vanadium correction option choosen.')
            return False
        if (self.number_of_empty_banks == 0
                and tof_opt['corrections']
                and (tof_opt['substract_vana_back']
                     or tof_opt['substract_sample_back'])):
            self.raise_error('No Background files selected, but background'
                             ' substraction option choosen.')
            return False
        return True

    def script_maker(self): # noqa: C901
        self.script = [""]

        def l(line=""):
            self.script += [line]

        paths = self.param_dict['paths']
        sample_data = create_dataset(
            self.param_dict['file_selector']['full_data'])
        tof_opt = self.param_dict['tof_powder_options']

        standard_data = create_dataset(
            self.param_dict['file_selector']['standard_data'])
        vanafilename = self.get_vana_filename(standard_data)
        emptyfilename = self.get_empty_filename(standard_data)
        samplefilename = self.get_sample_filename(sample_data)

        self.number_of_banks = len(sample_data.get(samplefilename, ' ')) - 1
        self.number_of_vana_banks = len(standard_data.get(vanafilename,
                                                          ' ')) - 1
        self.number_of_empty_banks = len(standard_data.get(emptyfilename,
                                                           ' ')) - 1
        vanadium_correction = self.check_vanadium_correction(tof_opt)
        background_correction = self.check_background_correction(tof_opt)

        if not self.validate_input(tof_opt):
            return False

        export_path = paths["export_dir"]
        ascii = paths["ascii"] and paths["export"] and export_path
        nexus = paths["nexus"] and paths["export"]  and export_path

        ### startin wrting script
        l("import numpy as np")
        l("from mantid.simpleapi import MonitorEfficiencyCorUser, FindEPP")
        l("from mantid.simpleapi import ComputeCalibrationCoefVan, Divide,"
          "CorrectTOF")
        l('from mantid.simpleapi import SaveAscii, SaveNexus')

        l("from DNSReduction.scripts.dnstof import convert_to_dE, get_sqw, "
          "load_data")
        l()
        l('sample_data = {}'.format(format_dataset(sample_data)))
        l('standard_data = {}'.format(format_dataset(standard_data)))

        backstring = ''
        vanastring = ''
        backtofstring = ''
        if vanadium_correction:
            vanastring = "\n          'vana_temperature' : "\
                         "{},".format(tof_opt['vanadium_temperature'])

        if background_correction and tof_opt['vana_back_factor'] != 1:
            backstring = "\n          'ecVanaFactor'     : "\
                          "{},".format(tof_opt['vana_back_factor'])

        if background_correction and tof_opt['sample_back_factor'] != 1:
            backtofstring = "\n          'ecSampleFactor'   : "\
                            "{},".format(tof_opt['sample_back_factor'])

        paramstring = "params = {{'e_channel'        : {}, " \
                      "\n          'wavelength'       : {}," \
                      "\n          'delete_raw'       : {}," \
                      "{}{}{} }}".format(tof_opt['epp_channel'],
                                         tof_opt['wavelength'],
                                         tof_opt['delete_raw'],
                                         vanastring,
                                         backstring,
                                         backtofstring)
        l(paramstring)
        l()
        l("bins = {{'qmin' : {:7.3f}, 'qmax' : {:7.3f}, 'qstep' : {:7.3f},"
          "\n        'dEmin': {:7.3f}, 'dEmax': {:7.3f}, 'dEstep': {:7.3f}"
          "}}".format(tof_opt['qmin'],
                      tof_opt['qmax'],
                      tof_opt['qstep'],
                      tof_opt['dEmin'],
                      tof_opt['dEmax'],
                      tof_opt['dEstep']))
        l()
        l('load_data(sample_data["{}"], "raw_data1", params)'
          ''.format(samplefilename))
        if background_correction:
            l('load_data(standard_data["{}"], "raw_ec", params)'
              ''.format(emptyfilename))
        if vanadium_correction:
            l('load_data(standard_data["{}"], "raw_vanadium", params)'
              ''.format(vanafilename))
        l("")
        if tof_opt['norm_monitor']:
            l('# normalize')
            l('data1 = MonitorEfficiencyCorUser("raw_data1")')
        else:
            l('data1 = mtd["raw_data1"]')
        if background_correction:
            l('ec =  MonitorEfficiencyCorUser("raw_ec")')
        if vanadium_correction:
            l('vanadium =  MonitorEfficiencyCorUser("raw_vanadium")')
        if background_correction and tof_opt['sample_back_factor'] != 1:
            if self.number_of_empty_banks != self.number_of_banks:
                l('# only one empty can bank')
                l("ec = ec[0]")
            if tof_opt['substract_sample_back']:
                l()
                l('# subtract empty can')
                l("data1 = data1 - ec * params['ecSampleFactor']")

        if vanadium_correction:
            if (tof_opt['substract_vana_back']
                    and background_correction):
                if tof_opt['vana_back_factor'] != 1:
                    l("vanadium = vanadium - ec * params['ecVanaFactor']")
                else:
                    l("vanadium = vanadium - ec")
            if self.number_of_vana_banks != self.number_of_banks:
                l('# only one vandium bank position')
                l('vanadium = vanadium[0]')
                l()
            l('# detector efficciency correction: compute coefficients')
            l('epptable = FindEPP(vanadium)')
            l("coefs = ComputeCalibrationCoefVan(vanadium, epptable,"
              " Temperature=params['vana_temperature'])")
            l()
            if tof_opt['mask_bad_detectors']:
                l('# get list of bad detectors')
                 ## the lastr part should not be necessary,
                 ## script should not return group workspaces
                if (self.number_of_vana_banks > 1
                        or self.number_of_vana_banks == self.number_of_banks):
                    l('badDetectors = np.where(np.array(coefs[0].extractY())'
                      '.flatten() <= 0)[0]')
                else:
                    l('badDetectors = np.where(np.array(coefs.extractY())'
                      '.flatten() <= 0)[0]')
                l('print("Following detectors will be masked: ",'
                  'badDetectors)')
                l('MaskDetectors(data1, DetectorList=badDetectors)')
                l()
            l('# apply detector efficiency correction')
            l('data1 = Divide(data1, coefs)')
            if tof_opt['correct_elastic_peak_position']:
                l()
                l('# correct TOF to get EPP at 0 meV')
                l('data1 = CorrectTOF(data1, epptable)')
                l()
        l('# get Ei')
        l("Ei = data1[0].getRun().getLogData('Ei').value")
        l('print ("Incident Energy is {} meV".format(Ei))')
        l()
        l('# get S(q,w)')
        l("convert_to_dE('data1', Ei)")
        l()
        l('# merge al detector positions together')
        l("get_sqw('data1_dE_S', 'data1', bins)")
        if ascii:
            l("SaveAscii('data1_dE_S', '{}/data1_dE_S.csv', "
              "WriteSpectrumID=False)"
              "".format(export_path))
        if nexus:
            l("SaveNexus('data1_dE_S', '{}/data1_dE_S.csv', )"
              "".format(export_path))

        return self.script
