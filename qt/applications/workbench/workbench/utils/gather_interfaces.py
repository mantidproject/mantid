# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

"""
Helper functions to collect the names of interfaces
"""

import os
from typing import AnyStr, List, Dict

from mantid.kernel import ConfigService, logger
from mantidqt.usersubwindowfactory import UserSubWindowFactory


def get_interface_dir() -> AnyStr:
    """
    Returns the path to the directory containing the mantidqt interfaces launch scripts
    This is the path to the mantidqtinterfaces package
    """
    import mantidqtinterfaces

    return os.path.dirname(mantidqtinterfaces.__file__)


def _discover_python_interfaces(interface_dir) -> Dict[str, List[str]]:
    """Return a dictionary mapping a category to a set of named Python interfaces"""
    items = ConfigService["mantidqt.python_interfaces"].split()
    # detect the python interfaces
    interfaces = {}
    for item in items:
        key, script_name = item.split("/")
        if not os.path.exists(os.path.join(interface_dir, script_name)):
            logger.warning('Failed to find script "{}" in "{}"'.format(script_name, interface_dir))
            continue
        interfaces.setdefault(key, []).append(script_name)

    return interfaces


def _discover_registers_to_run(interface_dir: str) -> Dict[str, List[str]]:
    items = ConfigService["mantidqt.python_interfaces"].split()
    try:
        register_items = ConfigService["mantidqt.python_interfaces_io_registry"].split()
    except KeyError:
        return {}

    registers_to_run = {}
    for item in items:
        key, script_name = item.split("/")
        reg_name = script_name[:-3] + "_register.py"
        if reg_name in register_items and os.path.exists(os.path.join(interface_dir, reg_name)):
            registers_to_run.setdefault(key, []).append(reg_name)

    return registers_to_run


def _discover_cpp_interfaces(interfaces: Dict[str, List[str]]):
    """Add C++ interfaces to passed dictionary"""
    cpp_interface_factory = UserSubWindowFactory.Instance()
    interface_names = cpp_interface_factory.keys()
    for name in interface_names:
        categories = cpp_interface_factory.categories(name)
        if len(categories) == 0:
            categories = ["General"]
        for category in categories:
            if category in interfaces.keys():
                interfaces[category].append(name)
            else:
                interfaces[category] = [name]


def gather_interface_names(python_interface_dir: str = get_interface_dir()) -> Dict[str, List[str]]:
    interfaces = _discover_python_interfaces(python_interface_dir)
    _discover_cpp_interfaces(interfaces)
    return interfaces


def get_registers_to_run(python_interface_dir: str = get_interface_dir()) -> Dict[str, List[str]]:
    registers_to_run = _discover_registers_to_run(python_interface_dir)
    return registers_to_run
