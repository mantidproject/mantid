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
from mantidqt.interfacemanager import InterfaceManager


def get_interface_dir() -> AnyStr:
    """
    Returns the path to the directory containing the mantidqt interfaces launch scripts
    This is the path to the mantidqtinterfaces package
    """
    import mantidqtinterfaces

    return os.path.dirname(mantidqtinterfaces.__file__)


def _discover_python_interfaces(interface_dir) -> Dict[str, List[str]]:
    interfaces = {}
    for category, script_name in [interface.split("/") for interface in ConfigService["mantidqt.python_interfaces"].split()]:
        if not os.path.exists(os.path.join(interface_dir, script_name)):
            logger.warning(f'Failed to find script "{script_name}" in "{interface_dir}"')
            continue
        interfaces.setdefault(category, []).append(script_name)

    return interfaces


def _discover_registers_to_run(interface_dir: str) -> Dict[str, List[str]]:
    try:
        register_items = ConfigService["mantidqt.python_interfaces_io_registry"].split()
    except KeyError:
        return {}

    registers_to_run = {}
    for category, script_name in [interface.split("/") for interface in ConfigService["mantidqt.python_interfaces"].split()]:
        reg_name = script_name[:-3] + "_register.py"
        if reg_name in register_items and os.path.exists(os.path.join(interface_dir, reg_name)):
            registers_to_run.setdefault(category, []).append(reg_name)

    return registers_to_run


def _discover_cpp_interfaces(interfaces: Dict[str, List[str]]):
    """Add C++ interfaces to passed dictionary"""
    # need to initialise this so cpp interfaces are registered
    InterfaceManager()
    cpp_interface_factory = UserSubWindowFactory.Instance()
    interface_names = cpp_interface_factory.keys()
    for name in interface_names:
        categories = cpp_interface_factory.categories(name)
        if len(categories) == 0:
            categories = ["General"]
        for category in categories:
            interfaces.setdefault(category, []).append(name)


def gather_interface_names(python_interface_dir: str = get_interface_dir()) -> Dict[str, List[str]]:
    """
    Returns a dictionary where keys are the interface categories and
    values are lists of interface names in each category
    """
    interfaces = _discover_python_interfaces(python_interface_dir)
    _discover_cpp_interfaces(interfaces)
    return interfaces


def gather_python_interface_names(python_interface_dir: str = get_interface_dir()) -> Dict[str, List[str]]:
    """
    Returns a dictionary where keys are the interface categories and
    values are lists of interface names in each category
    """
    return _discover_python_interfaces(python_interface_dir)


def gather_cpp_interface_names() -> Dict[str, List[str]]:
    """
    Returns a dictionary where keys are the interface categories and
    values are lists of interface names in each category
    """
    interfaces = {}
    _discover_cpp_interfaces(interfaces)
    return interfaces


def get_registers_to_run(python_interface_dir: str = get_interface_dir()) -> Dict[str, List[str]]:
    registers_to_run = _discover_registers_to_run(python_interface_dir)
    return registers_to_run
