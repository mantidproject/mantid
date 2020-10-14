# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench
#

from matplotlib import font_manager
import matplotlib as mpl


class PlotsSettingsModel(object):
    def get_font_names(self):
        font_list = font_manager.findSystemFonts()
        fonts = set()
        for font in font_list:
            # This try-except is for a known matplotlib bug where get_name() causes an error for certain fonts.
            try:
                font_name = font_manager.FontProperties(fname=font).get_name()
            except RuntimeError:
                continue
            fonts.add(font_name)
        fonts.add(self.get_current_mpl_font())
        fonts = sorted(fonts)
        return fonts

    @staticmethod
    def get_current_mpl_font():
        if mpl.rcParams['font.family'][0] in ['sans-serif', 'serif', 'cursive', 'fantasy', 'monospace']:
            current_mpl_font = mpl.rcParams['font.' + mpl.rcParams['font.family'][0]][0]
        else:
            current_mpl_font = mpl.rcParams['font.family'][0]
        return current_mpl_font
