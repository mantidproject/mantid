# -*- coding: utf-8 -*-
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Binding-agnostic registration of the workbench's compiled Qt resources.

PyQt6 ships no Python resource compiler (``pyrcc6`` was removed in Qt6), so the
build instead compiles ``resources.qrc`` to a binary ``workbench.rcc`` with Qt's
own ``rcc`` and we register it at runtime via ``QResource.registerResource`` --
an API present in every Qt binding. Under Qt5 the build still generates a
``pyrcc5`` ``resources.py`` module that self-registers when imported; this loader
falls back to importing that module when no ``.rcc`` is present.
"""

import os

from qtpy.QtCore import QResource

_RCC_PATH = os.path.join(os.path.dirname(__file__), "workbench.rcc")


def register_resources():
    """Make the workbench Qt resources available.

    Must be called before the ``QApplication`` is created, or paths to Qt's
    resources will not be set up correctly.
    """
    if os.path.exists(_RCC_PATH):
        if not QResource.registerResource(_RCC_PATH):
            raise RuntimeError(f"Failed to register workbench resources: {_RCC_PATH}")
        return
    # Qt5 fallback: importing the generated module runs qInitResources() as a side effect.
    try:
        import workbench.app.resources  # noqa: F401
    except (ModuleNotFoundError, ImportError) as exc:
        raise RuntimeError(
            f"workbench resources not found. Expected '{_RCC_PATH}' or a generated 'workbench.app.resources' module."
        ) from exc


def cleanup_resources():
    """Release resources registered by :func:`register_resources`. Safe to call at exit."""
    if os.path.exists(_RCC_PATH):
        QResource.unregisterResource(_RCC_PATH)
        return
    try:
        from workbench.app.resources import qCleanupResources
    except ModuleNotFoundError:
        return
    qCleanupResources()
