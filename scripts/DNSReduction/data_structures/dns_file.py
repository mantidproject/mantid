# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
Class which loads and stores a single DNS datafile in a dictionary
"""
from __future__ import (absolute_import, division, print_function)
import os
import numpy as np

from DNSReduction.data_structures.object_dict import ObjectDict


class DNSFile(ObjectDict):
    """
    class for storing data of a single dns datafile
    this is a dictionary  but can also be accessed like atributes
    """
    def __init__(self, datapath, filename):
        super(DNSFile, self).__init__()
        self.new_format = self.read(datapath, filename)

    def write(self, datapath, filename):
        # mostly stolen form nicos
        separator = "#" + "-" * 74 + "\n"
        myfile = open(os.path.join(datapath, filename), 'w')
        w = myfile.write
        wavelength = self['wavelength'] / 10.0  ## written in nm
        w("# DNS Data userid={},exp={},file={},sample={}\n".format(
            self['users'], self['proposal'], self['filenumber'],
            self['sample']))
        w(separator)

        w("# 2\n")
        w("# User: {}\n".format(self['users']))
        w("# Sample: {}\n".format(self['samplename']))
        w(separator)

        w("# DNS   Mono  d-spacing[nm]  Theta[deg]   "
          "Lambda[nm]   Energy[meV]   Speed[m/sec]\n")
        w("#      {}   {:6.4f}         {:6.2f}"
          "         {:6.3f}{:6.3f}      {:7.2f}\n".format("PG-002",
                                                          0.3350,
                                                          self['mon_rot'],
                                                          wavelength,
                                                          self['energy'],
                                                          self['speed']))

        w("# Distances [cm] Sample_Chopper    "
          "Sample_Detector    Sample_Monochromator\n")
        w("#                  36.00            80.00            220.00\n")
        w(separator)

        w("# Motors                      Position\n")
        w("# Monochromator              {:6.2f} deg\n".format(self['mon_rot']))
        w("# DeteRota                   {:6.2f} deg\n".format(self['det_rot']))
        w("#\n")
        w("# Huber                      {:6.2f} deg\n".format(
            self['sample_rot']))
        w("# Cradle_lower               {:6.2f} deg\n".format(
            self['cradle_lo']))
        w("# Cradle_upper               {:6.2f} deg\n".format(
            self['cradle_up']))
        w("#\n")
        w("# Slit_i_vertical upper      {:6.1f} mm\n".format(
            self['ap_sam_y_upper']))
        w("#                 lower      {:6.1f} mm\n".format(
            self['ap_sam_y_lower']))
        w("# Slit_i_horizontal left     {:6.1f} mm\n".format(
            self['ap_sam_x_left']))
        w("#                   right    {:6.1f} mm\n".format(
            self['ap_sam_x_right']))
        w("#\n")
        # dummy line
        w("# Slit_f_upper                {:4d} mm\n".format(0))
        # dummy line
        w("# Slit_f_lower                {:4d} mm\n".format(0))
        # dummy line
        w("# Detector_Position_vertical  {:4d} mm\n".format(0))
        w("#\n")
        w("# Polariser\n")
        w("#    Translation              {:4d} mm\n".format(
            int(round(self['pol_trans_x']))))
        w("#    Rotation              {:6.2f} deg\n".format(self['pol_rot']))
        w("#\n")
        w("# Analysers                 undefined\n")
        w(separator)
        # write currents
        w("# B-fields                   current[A]  field[G]\n")
        w("#   Flipper_precession        {:6.3f} A     {:6.2f} G\n".format(
            self['Co'], 0.0))
        w("#   Flipper_z_compensation    {:6.3f} A     {:6.2f} G\n".format(
            self['Fi'], 0.0))
        w("#   C_a                       {:6.3f} A     {:6.2f} G\n".format(
            self['A'], 0.0))
        w("#   C_b                       {:6.3f} A     {:6.2f} G\n".format(
            self['B'], 0.0))
        w("#   C_c                       {:6.3f} A     {:6.2f} G\n".format(
            self['C'], 0.0))
        w("#   C_z                       {:6.3f} A     {:6.2f} G\n".format(
            self['ZT'], 0.0))
        w(separator)

        w("# Temperatures/Lakeshore      T\n")
        w("#  T1                         {:6.3f} K\n".format(
            self['temp_tube']))
        w("#  T2                         {:6.3f} K\n".format(
            self['temp_samp']))
        w("#  sample_setpoint            {:6.3f} K\n".format(self['temp_set']))
        w(separator)

        w("# TOF parameters\n")
        w("#  TOF channels                {:4d}\n".format(self['tofchannels']))
        w("#  Time per channel            {:6.1f} microsecs\n".format(
            self['channelwidth']))
        w("#  Delay time                  {:6.1f} microsecs\n".format(
            self['tofdelay']))

        w("#  Chopper slits\n")
        w("#  Elastic time channel\n")
        w("#  Chopper frequency\n")
        w(separator)

        w("# Active_Stop_Unit           TIMER\n")
        w("#  Timer                    {:6.1f} sec\n".format(self['timer']))
        w("#  Monitor           {:16d}\n".format(self['monitor']))
        w("#\n")
        w("#    start   at      {}\n".format(self['starttime']))
        w("#    stopped at      {}\n".format(self['endtime']))
        w(separator)

        w("# Extended data\n")
        if self['scannumber']:
            w("#  Scannumber               {:8d}\n".format(
                int(self['scannumber'])))
        else:
            w("#  Scannumber                       \n")
        w("#  Scancommand              {}\n".format(self['scancommand']))
        w("#  Scanposition             {:>8s}\n".format(self['scanposition']))
        w("#  pol_trans_x              {:8.1f} mm\n".format(
            self['pol_trans_x']))
        w("#  pol_trans_y              {:8.1f} mm\n".format(
            self['pol_trans_y']))
        w("#  field                    {:>8s}\n".format(self['field']))
        w("#  selector_lift            {:8.1f} mm\n".format(
            self['selector_lift']))
        w("#  selector_speed           {:8.1f} rpm\n".format(
            self['selector_speed']))
        w(separator)

        # write array
        w("# DATA (number of detectors, number of TOF channels)\n")
        w("# 64 {:4d}\n".format(self['tofchannels']))
        for ch in range(24):
            w("{:2d} ".format(ch))
            for q in range(self['tofchannels']):
                w(" {:8d}".format(self.counts[ch, q]))
            w("\n")
        for ch in range(24, 64):
            w("{:2d} ".format(ch))
            for q in range(self['tofchannels']):
                w(" {:8d}".format(0))
            w("\n")
        myfile.close()

    def read(self, datapath, filename):
        with open(os.path.join(datapath, filename), 'r') as f:
            txt = f.readlines()
        del f
        if len(txt) < 138 or not txt[0].startswith('# DNS Data'):
            del txt
            return False
        self['filename'] = filename
        line = txt[0].split(',')
        self['users'] = line[0][18:]
        self['filenumber'] = line[2][5:]
        self['proposal'] = line[1][4:]
        self['sample'] = line[3][7:-1]
        line = txt[7].split()
        self['mon_rot'] = float(line[3])
        self['wavelength'] = float(line[4]) * 10
        self['energy'] = float(line[5])
        self['speed'] = float(line[6])
        self['mon_rot'] = float(txt[12][25:-5])
        self['det_rot'] = float(txt[13][25:-5])
        self['sample_rot'] = float(txt[15][25:-5])
        self['cradle_lo'] = float(txt[16][25:-5])
        self['cradle_up'] = float(txt[17][25:-5])
        self['ap_sam_y_upper'] = float(txt[19][25:-4])
        self['ap_sam_y_lower'] = float(txt[20][25:-4])
        self['ap_sam_x_left'] = float(txt[21][25:-4])
        self['ap_sam_x_right'] = float(txt[22][25:-4])
        self['pol_trans_x'] = float(txt[29][25:-3])
        self['pol_rot'] = float(txt[30][25:-4])
        self['Co'] = float(txt[35][25:-16])
        self['Fi'] = float(txt[36][27:-16])
        self['A'] = float(txt[37][25:-16])
        self['B'] = float(txt[38][25:-16])
        self['C'] = float(txt[39][25:-16])
        self['ZT'] = float(txt[40][25:-16])
        self['temp_tube'] = float(txt[43][25:-3])
        self['temp_samp'] = float(txt[44][25:-3])
        self['temp_set'] = float(txt[45][25:-3])
        self['tofchannels'] = int(txt[48][25:-1])
        self['channelwidth'] = float(txt[49][25:-11])
        self['tofdelay'] = float(txt[50][25:-11])
        self['timer'] = float(txt[56][15:-5])
        self['monitor'] = int(txt[57][15:-1])
        self['starttime'] = txt[59][21:-1]
        self['endtime'] = txt[60][21:-1]
        self['scannumber'] = txt[63][15:-1].strip()
        self['scancommand'] = txt[64][28:-1]
        self['scanposition'] = txt[65][15:-1].strip()
        self['pol_trans_x'] = float(txt[66][15:-4])
        self['pol_trans_y'] = float(txt[67][15:-4])
        self['field'] = txt[68][10:-1].strip()
        self['selector_lift'] = float(txt[69][17:-4])
        self['selector_speed'] = float(txt[70][17:-4])
        if '/' in self['scanposition']:
            self['scanpoints'] = self['scanposition'].split('/')[1]
        else:
            self['scanpoints'] = ''
        self.counts = np.zeros((24, self['tofchannels']),
                               dtype=int)  ### for python 2 use long
        for ch in range(24):
            self.counts[ch, :] = txt[74 + ch].split()[1:]
        del txt
        return True
