# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
#
from psutil import virtual_memory

def getMemoryUsed():
    mem_used  = virtual_memory().used
    mem_total = virtual_memory().total
    mem_used_percent = min(int(round(mem_used * 100 / mem_total)),100)
    conversion_factor_to_GB = 1.0 / 1024 / 1024 / 1024
    mem_used_GB = mem_used * conversion_factor_to_GB
    mem_total_GB = mem_total * conversion_factor_to_GB
    return mem_used_percent, mem_used_GB, mem_total_GB
