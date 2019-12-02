# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
Common script generator for DNS data reduction
"""

from __future__ import (absolute_import, division, print_function)

from DNSReduction.script_generator.common_script_generator_presenter import DNSScriptGenerator_presenter
from DNSReduction.scripts.dnstof_sc import hkl_string_to_mantid_string


class DNSTofScScriptGenerator_presenter(DNSScriptGenerator_presenter):

    # pass the view and model into the presenter
    def __init__(self, parent):
        super(DNSTofScScriptGenerator_presenter,
              self).__init__(parent, 'tof_sc_script_generator')
        self.number_of_banks = None
        self.script = None

    def script_maker(self):
        self.script = [""]

        def l(line=""):
            self.script += [line]

        sampledata = self.param_dict['file_selector']['full_data']
        tof_opt = self.param_dict['tof_sc_options']
        ppsample = self.preprocess_filelist(sampledata)
        self.number_of_banks = len(ppsample)
        sppsample = str(ppsample).replace(
            "]), (", "]),\n                                             (")
        u = hkl_string_to_mantid_string(tof_opt['u'])
        v = hkl_string_to_mantid_string(tof_opt['v'])
        normalization = 'monitor'
        bvector0 = '[100],unit,1,0,0,0'
        bvector1 = '[010],unit,0,1,0,0'
        bvector2 = '[001],unit,0,0,1,0'
        bvector3 = 'dE,meV,0,0,0,1'
        extents = ','.join([
            str(tof_opt[x]) for x in
            ['hmin', 'hmax', 'kmin', 'kmax', 'lmin', 'lmax', 'dEmin', 'dEmax']
        ])
        bins = ','.join(
            [str(tof_opt[x]) for x in ['hbins', 'kbins', 'lbins', 'dEbins']])
        l("from __future__ import absolute_import, " \
          "division, print_function, unicode_literals"
         )
        l("from mantid.simpleapi import *")
        l("from collections import OrderedDict")
        l("from DNSReduction.scripts.dnstof_sc import get_filepath_string")
        l()
        l("data   = {{'data_path'        : '{}',".format(
            self.param_dict['paths']['data_dir']) +
          "\n          'proposal_number'  : '{}',".format(
              self.param_dict['paths']['prop_nb']) +
          "\n          'data_numbers'     : {} }}".format(sppsample))
        l("filepaths = get_filepath_string(data)")
        l()
        l("LoadDNSSCD(FileNames=filepaths," +
          "\n           NormalizationWorkspace='edata_norm'," +
          "\n           Normalization='{}',".format(normalization) +
          "\n           a={},".format(tof_opt['a']) +
          "\n           b={},".format(tof_opt['b']) +
          "\n           c={},".format(tof_opt['c']) +
          "\n           alpha={},".format(tof_opt['alpha']) +
          "\n           beta={},".format(tof_opt['beta']) +
          "\n           gamma={},".format(tof_opt['gamma']) +
          "\n           OmegaOffset={},".format(tof_opt['omega_offset']) +
          "\n           hkl1='{}',".format(u) +
          "\n           hkl2='{}',".format(v) +
          "\n           SaveHuberTo='huber'," +
          "\n           ElasticChannel={},".format(tof_opt['epp_channel']) +
          "\n           OutputWorkspace='raw_edata')")
        l()
        l("## bin data##")
        l("bvector0 = '{}'".format(bvector0))
        l("bvector1 = '{}'".format(bvector1))
        l("bvector2 = '{}'".format(bvector2))
        l("bvector3= '{}'".format(bvector3))
        l("extents = '{}'".format(extents))
        l("bins = '{}'".format(bins))
        l()
        l("raw_data = BinMD('raw_edata', AxisAligned='0', " \
          "BasisVector0=bvector0, BasisVector1=bvector1," \
          "BasisVector2=bvector2,"
          +
          "\n                 BasisVector3=bvector3, OutputExtents=extents," \
          "OutputBins=bins, NormalizeBasisVectors='0')"
         )
        l("data_norm = BinMD('edata_norm', AxisAligned='0', " \
          "BasisVector0=bvector0,  BasisVector1=bvector1, " \
          "BasisVector2=bvector2,"
          +
          "\n                 BasisVector3=bvector3, OutputExtents=extents," \
          "OutputBins=bins, NormalizeBasisVectors='0')"
         )

        l("## normalize ##")
        l("data = raw_data/data_norm")

        return self.script
