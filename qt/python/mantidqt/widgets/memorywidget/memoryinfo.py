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
from typing import Tuple

CONVERSION_FACTOR_BYTES_TO_GB = 1.0 / (1024 * 1024 * 1024)


def get_memory_info() -> Tuple[int, float, float]:
    """
    Returns following system memory usage information
    obtained using psutil.virtual_memory
    mem_used_percent : Used system memory in percentage
    mem_used_GB : Used system memory in Gigabytes(GB)
    mem_total_GB : Total available system memory in GB
    """
    mem_used = virtual_memory().used
    mem_total = virtual_memory().total
    mem_used_percent = min(int(round(mem_used * 100 / mem_total)), 100)
    mem_used_GB = mem_used * CONVERSION_FACTOR_BYTES_TO_GB
    mem_total_GB = mem_total * CONVERSION_FACTOR_BYTES_TO_GB
    return mem_used_percent, mem_used_GB, mem_total_GB
