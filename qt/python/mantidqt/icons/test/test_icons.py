# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#

from __future__ import (absolute_import, division, print_function, unicode_literals)


from mantidqt.utils.qt.testing import GuiTest
from mantidqt.icons import get_icon


class IconsTest(GuiTest):
    def test_get_icon_name(self):
        icon = get_icon("mdi.run-fast")
        self.assertEqual(icon.isNull(), False)

    def test_get_icon_name_color(self):
        icon = get_icon("mdi.run-fast", "red")
        self.assertEqual(icon.isNull(), False)

    def test_get_icon_name_color_scaleFactor(self):
        icon = get_icon("mdi.run-fast", "red", 1.5)
        self.assertEqual(icon.isNull(), False)

    def test_get_icon_list_and_list_of_options(self):
        icon = get_icon(["mdi.run-fast", "mdi.run"], [{"color": "red", "scaleFactor": 1.5},
                                                      {"color": "green", "scaleFactor": 1.2}])
        self.assertEqual(icon.isNull(), False)
