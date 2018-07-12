#  This file is part of the mantid workbench.
#
#  Copyright (C) 2017 mantidproject
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
"""A selection of utility functions related to Qt functionality
"""
from __future__ import absolute_import

# stdlib modules
from contextlib import contextmanager
from importlib import import_module
import os.path as osp

# 3rd-party modules
import qtawesome as qta
from qtpy import QT_VERSION
from qtpy.uic import loadUi, loadUiType
from qtpy.QtWidgets import QAction, QMenu

LIB_SUFFIX = 'qt' + QT_VERSION[0]


def import_qtlib(modulename, package, attr=None):
    """Import a module built against a specified major version of Qt.
    The provided name is suffixed with the string 'qtX' where
    X is the major version of Qt that the library was built against.
    The version is determined by inspecting the loaded Python Qt
    bindings. If none can be found then Qt5 is assumed.

    It is assumed that the module is a build product and therefore may not be in a fixed
    location relative to the python source files. First a relative import from the calling
    package is requested followed by a simple search for the binary by name on sys.path
    :param modulename: The name of the dynamic module to find
    :param package: The calling package trying to import the module using dotted syntax
    :param attr: Optional attribute to retrieve from the module
    :return: Either the module object if no attribute is specified of the requested attribute
    """
    try:
        lib = import_module('.' + modulename + LIB_SUFFIX, package)
    except ImportError:
        lib = import_module(modulename + LIB_SUFFIX)
    if attr is not None:
        return getattr(lib, attr)
    else:
        return lib


def load_ui(caller_filename, ui_relfilename, baseinstance=None):
    """Load a ui file assuming it is relative to a given callers filepath. If
    an existing instance is given then the this instance is set as the baseclass
    and the new Ui instance is returned otherwise the loaded Ui class is returned

    :param caller_filename: An absolute path to a file whose basename when
    joined with ui_relfilename gives the full path to the .ui file. Generally
    this called with __file__
    :param ui_relfilename: A relative filepath that when joined with the
    basename of caller_filename gives the absolute path to the .ui file.
    :param baseinstance: An instance of a widget to pass to uic.loadUi
    that becomes the base class rather than a new widget being created.
    :return: A new instance of the form class if baseinstance is given, otherwise
    return the form class
    """
    filepath = osp.join(osp.dirname(caller_filename), ui_relfilename)
    if baseinstance is not None:
        return loadUi(filepath, baseinstance=baseinstance)
    else:
        return loadUiType(filepath)


@contextmanager
def block_signals(widget):
    """
    A context manager that helps to block widget's signals temporarily. Usage:

        with block_signals(widget):
            widget.do_actions_that_emit_signals()

    :param widget: A Qt widget signals from which should be blocked.
    """
    widget.blockSignals(True)
    yield
    widget.blockSignals(False)


@contextmanager
def widget_updates_disabled(widget):
    """Context manager that disables widget updates for the duration of the context
    and reenables them at the end
    :param widget: A widget object to use as context
    """
    widget.setUpdatesEnabled(False)
    yield
    widget.setUpdatesEnabled(True)


def create_action(parent, text, on_triggered=None, shortcut=None,
                  shortcut_context=None, icon_name=None):
    """Create a QAction based on the give properties

    :param parent: The parent object
    :param text: Text string to display
    :param on_triggered: An optional slot to call on the triggered signal
    :param shortcut: An optional shortcut
    :param shortcut_context: An optional context for the supplied shortcut.
    Only applies if a shortcut has been given
    :param icon_name: The name of the qt awesome uri for an icon.
    :return: A new QAction object
    """
    action = QAction(text, parent)
    if on_triggered is not None:
        action.triggered.connect(on_triggered)
    if shortcut is not None:
        action.setShortcut(shortcut)
        if shortcut_context is not None:
            action.setShortcutContext(shortcut_context)
    if icon_name is not None:
        action.setIcon(qta.icon(icon_name))

    return action


def add_actions(target, actions):
    """Add a collection of actions to a relevant
    target (menu or toolbar)
    :param target: An instance of QMenu or QToolbar
    :param actions: A collection of actions to be added
    :raises ValueError: If one of the actions is not an instance of QMenu/QAction
    """
    for action in actions:
        if action is None:
            target.addSeparator()
        elif isinstance(action, QMenu):
            target.addMenu(action)
        elif isinstance(action, QAction):
            target.addAction(action)
        else:
            raise ValueError("Unexpected action type. "
                             "Expected one of (QAction,QMenu) but found '{}'".format(type(action)))
