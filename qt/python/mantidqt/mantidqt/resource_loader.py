# -*- coding: utf-8 -*-
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Binding-agnostic registration of the mantidqt Qt resources (``:/replace.png`` etc.).

PyQt6 ships no Python resource compiler (``pyrcc6`` was removed in Qt6), so the
build instead compiles ``resources.qrc`` to a binary ``mantidqt.rcc`` with Qt's
own ``rcc`` and we register it at runtime via ``QResource.registerResource`` --
an API present in every Qt binding. Under Qt5 the build still generates a
``pyrcc5`` ``resources.py`` module that self-registers when imported; this loader
falls back to importing that module when no ``.rcc`` is present.

These resources are used from C++ (e.g. PropertyWidget's ``:/`` icon paths), so
registration must happen before any such widget is created -- importers call
``register_resources()`` as an import side effect, mirroring the old
``from mantidqt import resources``.
"""

import os

from qtpy.QtCore import QResource

_RCC_PATH = os.path.join(os.path.dirname(__file__), "mantidqt.rcc")


def register_resources():
    """Make the mantidqt Qt resources available process-wide. Safe to call repeatedly."""
    if os.path.exists(_RCC_PATH):
        if not QResource.registerResource(_RCC_PATH):
            raise RuntimeError(f"Failed to register mantidqt resources: {_RCC_PATH}")
        return
    # Qt5 fallback: importing the generated module runs qInitResources() as a side effect.
    try:
        from mantidqt import resources  # noqa: F401
    except (ModuleNotFoundError, ImportError) as exc:
        raise RuntimeError(f"mantidqt resources not found. Expected '{_RCC_PATH}' or a generated 'mantidqt.resources' module.") from exc


def cleanup_resources():
    """Release resources registered by :func:`register_resources`. Safe to call at exit."""
    if os.path.exists(_RCC_PATH):
        QResource.unregisterResource(_RCC_PATH)
        return
    try:
        from mantidqt.resources import qCleanupResources
    except (ModuleNotFoundError, ImportError):
        return
    qCleanupResources()
