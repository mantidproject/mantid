# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
#
from dataclasses import dataclass
import os
import psutil
from psutil import virtual_memory

CONVERSION_FACTOR_BYTES_TO_GB = 1.0 / (1024 * 1024 * 1024)


@dataclass
class MantidMemoryInfo:
    used_percent: int
    used_GB: float
    system_total_GB: float


@dataclass
class SystemMemoryInfo:
    used_percent: int
    used_GB: float
    system_total_GB: float


def get_memory_info() -> SystemMemoryInfo:
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
    return SystemMemoryInfo(used_percent=mem_used_percent, used_GB=mem_used_GB, system_total_GB=mem_total_GB)


def get_mantid_memory_info() -> MantidMemoryInfo:
    """
    Returns following process memory usage information obtained using psutil
    mantid_mem_used_percent : Used  memory by the current process in percentage
    mantid_mem_used_GB : Used memory by the current process in Gigabytes(GB)

    Returns following system memory usage information
    obtained using psutil.virtual_memory
    mem_total_GB : Total available system memory in GB
    """
    current_process = psutil.Process(os.getpid())
    mantid_mem_used = current_process.memory_info().rss
    mem_total = virtual_memory().total
    mantid_mem_used_percent = min(int(round(mantid_mem_used * 100 / mem_total)), 100)
    mantid_mem_used_GB = mantid_mem_used * CONVERSION_FACTOR_BYTES_TO_GB
    mem_total_GB = mem_total * CONVERSION_FACTOR_BYTES_TO_GB
    return MantidMemoryInfo(used_percent=mantid_mem_used_percent, used_GB=mantid_mem_used_GB, system_total_GB=mem_total_GB)
