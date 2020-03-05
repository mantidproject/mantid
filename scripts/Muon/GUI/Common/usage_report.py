# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, unicode_literals)

from mantid.kernel import (UsageService, FeatureType)


def report_interface_startup(name):
    #interface startup
    UsageService.registerFeatureUsage(FeatureType.Interface, name, False)
