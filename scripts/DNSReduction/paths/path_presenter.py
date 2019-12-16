# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
presenter for dns path panel
"""

from __future__ import (absolute_import, division, print_function)
import os
import glob as glob

from DNSReduction.data_structures.dns_observer import DNSObserver
from DNSReduction.paths.path_view import DNSPath_view


class DNSPath_presenter(DNSObserver):

    # pass the view and model into the presenter
    def __init__(self, parent):
        super(DNSPath_presenter, self).__init__(parent, 'paths')
        self.name = 'paths'
        self.view = DNSPath_view(self.parent.view)
        # connect signals
        self.view.sig_data_path_set.connect(self.set_user_prop_from_datafile)
        self.view.sig_clear_cache.connect(self.clear_cache)

    def set_user_prop_from_datafile(self, dir_name):
        try:
            firstfilename = next(glob.iglob('{}/*.d_dat'.format(dir_name)))
            with open(firstfilename, 'r') as datafile:
                txt = datafile.readline().split('userid=')[1].split(',exp=')
                prop_nb = txt[1].split(',file=')[0]
                user = txt[0]
            self.view.set_prop_number(prop_nb)
            self.view.set_user(user)
        except StopIteration:
            self.view.show_statusmessage(
                'No DNS .d_dat file found in data directory', 30)

    def clear_cache(self):
        path = self.own_dict['data_dir']
        if path:
            os.remove(path + '/last_filelist.txt')
