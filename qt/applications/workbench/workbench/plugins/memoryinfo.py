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
    memory_used  = virtual_memory().used
    memory_available = virtual_memory().available
    memory_free_percent = int(round(memory_used * 100 / memory_available))
    return memory_free_percent
