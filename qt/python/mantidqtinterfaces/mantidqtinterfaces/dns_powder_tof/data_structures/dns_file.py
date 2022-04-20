# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
Class which loads and stores a single DNS datafile in a dictionary.
"""

import numpy as np
from mantidqtinterfaces.dns_powder_tof.data_structures.object_dict import ObjectDict
from mantidqtinterfaces.dns_powder_tof.helpers.file_processing import (load_txt, save_txt)


class DNSFile(ObjectDict):
    """
    Class for reading, writing and storing data of a single dns datafile.
    This is a dictionary, but can also be accessed like attributes.
    """
    def __init__(self, datapath, filename):
        super().__init__()
        self.new_format = self.read(datapath, filename)

    def write(self, datapath, filename):
        # mostly stolen form nicos
        txt = ''
        separator = "#" + "-" * 74 + "\n"
        wavelength = self['wavelength'] / 10.0  # written in nm
        txt += f"# DNS Data userid={self['users']},exp={self['proposal']}," \
               f"file={self['file_number']},sample={self['sample']}\n"
        txt += separator
        txt += "# 2\n"
        txt += f"# User: {self['users']}\n"
        txt += f"# Sample: {self['sample']}\n"
        txt += separator

        txt += "# DNS   Mono  d-spacing[nm]  Theta[deg]   " \
               "Lambda[nm]   Energy[meV]   Speed[m/sec]\n"
        txt += f"#      PG-002   {0.3350:6.4f}     " \
               f"    {self['mon_rot']:6.2f}" \
               f"         {wavelength:6.3f}{self['energy']:6.3f}  " \
               f"    {self['speed']:7.2f}\n"
        txt += "# Distances [cm] Sample_Chopper    " \
               "Sample_Detector    Sample_Monochromator\n"
        txt += "#                  36.00            80.00            220.00\n"
        txt += separator

        txt += "# Motors                      Position\n"
        txt += f"# Monochromator              {self['mon_rot']:6.2f} deg\n"
        txt += f"# DeteRota                   {self['det_rot']:6.2f} deg\n"
        txt += "#\n"
        txt += f"# Huber                      {self['sample_rot']:6.2f} deg\n"
        txt += f"# Cradle_lower               {self['cradle_lo']:6.2f} deg\n"
        txt += f"# Cradle_upper               {self['cradle_up']:6.2f} deg\n"
        txt += "#\n"
        txt += "# Slit_i_vertical upper" \
               f"      {self['ap_sam_y_upper']:6.1f} mm\n"
        txt += "#                 lower" \
               f"      {self['ap_sam_y_lower']:6.1f} mm\n"
        txt += "# Slit_i_horizontal left    " \
               f" {self['ap_sam_x_left']:6.1f} mm\n"
        txt += "#                   right" \
               f"    {self['ap_sam_x_right']:6.1f} mm\n"
        txt += "#\n"
        # dummy line
        txt += f"# Slit_f_upper                {0:4d} mm\n"
        # dummy line
        txt += f"# Slit_f_lower                {0:4d} mm\n"
        # dummy line
        txt += f"# Detector_Position_vertical  {0:4d} mm\n"
        txt += "#\n"
        txt += "# Polariser\n"
        txt += f"#    Translation              " \
               f"{int(round(self['pol_trans_x'])):4d} mm\n"
        txt += f"#    Rotation              {self['pol_rot']:6.2f} deg\n"
        txt += "#\n"
        txt += "# Analysers                 undefined\n"
        txt += separator
        # write currents
        txt += "# B-fields                   current[A]  field[G]\n"
        txt += "#   Flipper_precession        " \
               f"{self['Co']:6.3f} A     {0:6.2f} G\n"
        txt += "#   Flipper_z_compensation    " \
               f"{self['Fi']:6.3f} A     {0:6.2f} G\n"
        txt += "#   C_a                       " \
               f"{self['A']:6.3f} A     {0:6.2f} G\n"
        txt += "#   C_b                       " \
               f"{self['B']:6.3f} A     {0:6.2f} G\n"
        txt += "#   C_c                       " \
               f"{self['C']:6.3f} A     {0:6.2f} G\n"
        txt += "#   C_z                       " \
               f"{self['ZT']:6.3f} A     {0:6.2f} G\n"
        txt += separator
        txt += "# Temperatures/Lakeshore      T\n"
        txt += f"#  T1                         {self['temp_tube']:6.3f} K\n"
        txt += f"#  T2                         {self['temp_samp']:6.3f} K\n"
        txt += f"#  sample_setpoint            {self['temp_set']:6.3f} K\n"
        txt += separator

        txt += "# TOF parameters\n"
        txt += f"#  TOF channels                {self['tof_channels']:4d}\n"
        txt += "#  Time per channel            " \
               f"{self['channel_width']:6.1f} microsecs\n"
        txt += "#  Delay time                  " \
               f"{self['tofdelay']:6.1f} microsecs\n"
        txt += "#  Chopper slits\n"
        txt += "#  Elastic time channel\n"
        txt += "#  Chopper frequency\n"
        txt += separator
        txt += "# Active_Stop_Unit           TIMER\n"
        txt += f"#  Timer                    {self['timer']:6.1f} sec\n"
        txt += f"#  Monitor           {self['monitor']:16d}\n"
        txt += "#\n"
        txt += f"#    start   at      {self['starttime']}\n"
        txt += f"#    stopped at      {self['end_time']}\n"
        txt += separator

        txt += "# Extended data\n"
        if self['scan_number']:
            txt += "#  scan_number               " \
                   f"{int(self['scan_number']):8d}\n"
        else:
            txt += "#  scan_number                       \n"
        txt += f"#  scan_command              {self['scan_command']}\n"
        txt += f"#  Scanposition             {self['scanposition']:>8s}\n"
        txt += f"#  pol_trans_x              {self['pol_trans_x']:8.1f} mm\n"
        txt += f"#  pol_trans_y              {self['pol_trans_y']:8.1f} mm\n"
        txt += f"#  field                    {self['field']:>8s}\n"
        txt += f"#  selector_lift            {self['selector_lift']:8.1f} mm\n"
        txt += "#  selector_speed           " \
               f"{self['selector_speed']:8.1f} rpm\n"
        txt += separator

        # write array
        txt += "# DATA (number of detectors, number of TOF channels)\n"
        txt += f"# 64 {self['tof_channels']:4d}\n"
        for ch in range(24):
            txt += f"{ch:2d} "
            for q in range(self['tof_channels']):
                txt += f" {self.counts[ch, q]:8d}"
            txt += "\n"
        for ch in range(24, 64):
            txt += f"{ch:2d} "
            for q in range(self['tof_channels']):
                txt += f" {0:8d}"
            txt += "\n"
        txt = ''.join([line.rstrip() + '\n' for line in txt.splitlines()])
        save_txt(txt, filename, datapath)

    def read(self, datapath, filename):
        txt = load_txt(filename, datapath)
        if len(txt) < 138 or not txt[0].startswith('# DNS Data'):
            del txt
            return False
        self['filename'] = filename
        line = txt[0]
        line = line.split('userid=')[1].split(',exp=')
        self['users'] = line[0]
        line = line[1].split(',file=')
        self['proposal'] = line[0]
        line = line[1].split(',sample=')
        self['file_number'] = line[0]
        self['sample'] = line[1][:-1]
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
        self['tof_channels'] = int(txt[48][25:-1])
        self['channel_width'] = float(txt[49][25:-11])
        self['tofdelay'] = float(txt[50][25:-11])
        self['timer'] = float(txt[56][15:-5])
        self['monitor'] = int(txt[57][15:-1])
        self['starttime'] = txt[59][21:-1]
        self['end_time'] = txt[60][21:-1]
        self['scan_number'] = txt[63][15:-1].strip()
        self['scan_command'] = txt[64][28:-1]
        self['scanposition'] = txt[65][15:-1].strip()
        self['pol_trans_x'] = float(txt[66][15:-4])
        self['pol_trans_y'] = float(txt[67][15:-4])
        self['field'] = txt[68][10:-1].strip()
        self['selector_lift'] = float(txt[69][17:-4])
        self['selector_speed'] = float(txt[70][17:-4])
        if '/' in self['scanposition']:
            self['scan_points'] = self['scanposition'].split('/')[1]
        else:
            self['scan_points'] = ''
        self['counts'] = np.zeros((24, self['tof_channels']),
                                  dtype=int)  # for python 2 use long
        for ch in range(24):
            self['counts'][ch, :] = txt[74 + ch].split()[1:]
        del txt
        return True
