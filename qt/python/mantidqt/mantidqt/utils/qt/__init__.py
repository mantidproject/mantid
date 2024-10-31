# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
"""A selection of utility functions related to Qt functionality"""

# stdlib modules
import os
import os.path as osp
from contextlib import contextmanager
from importlib import import_module
import warnings

warnings.filterwarnings(action="ignore", category=DeprecationWarning, module=".*uic.*")

# 3rd-party modules
from qtpy import QT_VERSION  # noqa: E402
from qtpy.QtCore import QPoint  # noqa: E402
from qtpy.QtGui import QKeySequence  # noqa: E402
from qtpy.QtWidgets import QAction, QMenu, QDesktopWidget  # noqa: E402
from qtpy.uic import loadUi, loadUiType  # noqa: E402

LIB_SUFFIX = "qt" + QT_VERSION[0]


def import_qt(modulename, package, attr=None):
    """Import a module built against the version of Qt we are running.
    The provided name is suffixed with the string 'qtX' where
    X is the major version of Qt that the library was built against.
    The version is determined by inspecting the loaded Python Qt
    bindings. If none can be found then Qt5 is assumed.

    If modulename looks like a relative name then the import is first tried as specified
    and if this fails it is attempted as a top-level import without a package name and the relative prefixes removed.
    This allows the built module to live outside of the package in a binary directory of the build. For example, a
    developer build would put the built dynamic library in the bin directory but a package installation
    would have the library in the proper location as indicated by the relative import path. This avoids any build
    products being placed in the source tree.

    :param modulename: The name of the dynamic module to find, possibly a relative name
    :param package: If the name looks like a relative import then the package argument is required to specify the
    parent package
    :param attr: Optional attribute to retrieve from the module
    :return: Either the module object if no attribute is specified of the requested attribute
    """
    if modulename.startswith("."):
        try:
            lib = import_module(modulename + LIB_SUFFIX, package)
        except ImportError as e1:
            try:
                lib = import_module(modulename.lstrip(".") + LIB_SUFFIX)
            except ImportError as e2:
                msg = 'import of "{}" failed with "{}"'
                msg = "First " + msg.format(modulename + LIB_SUFFIX, e1) + ". Second " + msg.format(modulename.lstrip(".") + LIB_SUFFIX, e2)
                raise ImportError(msg)
    else:
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
    return a tuple that contains the Ui_Form and an instance: (Ui_Form, Instance).
    If inheriting, inherit the form, then the instance - class MyClass(Ui_Form, Instance)
    """
    filepath = osp.join(osp.dirname(caller_filename), ui_relfilename)
    if not osp.exists(filepath):
        raise ImportError('File "{}" does not exist'.format(filepath))
    if not osp.isfile(filepath):
        raise ImportError('File "{}" is not a file'.format(filepath))
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


def create_action(
    parent, text, on_triggered=None, shortcut=None, shortcut_context=None, icon_name=None, shortcut_visible_in_context_menu=None
):
    """Create a QAction based on the give properties

    :param parent: The parent object
    :param text: Text string to display
    :param on_triggered: An optional slot to call on the triggered signal
    :param shortcut: An optional shortcut
    :param shortcut_context: An optional context for the supplied shortcut.
    Only applies if a shortcut has been given
    :param icon_name: The name of the qt awesome uri for an icon.
    :param shortcut_visible_in_context_menu: Qt 5.10 decided that all QMenus that are NOT inside a QMenuBar
                                             are context menus, and are styled as such. By default keyboard shortcuts
                                             are NOT shown on context menus. Set this to True to show them.
    :return: A new QAction object
    """
    from ...icons import get_icon

    action = QAction(text, parent)
    if on_triggered is not None:
        action.triggered.connect(on_triggered)
    if shortcut is not None:
        if isinstance(shortcut, tuple) or isinstance(shortcut, list):
            qshortcuts = [QKeySequence(s) for s in shortcut]
            action.setShortcuts(qshortcuts)
        else:
            action.setShortcut(shortcut)

        if shortcut_context is not None:
            action.setShortcutContext(shortcut_context)
    if icon_name is not None:
        action.setIcon(get_icon(icon_name))

    # shortcuts in context menus option is only available after Qt 5.10
    if hasattr(action, "setShortcutVisibleInContextMenu") and shortcut_visible_in_context_menu:
        action.setShortcutVisibleInContextMenu(shortcut_visible_in_context_menu)

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
            raise ValueError("Unexpected action type. " "Expected one of (QAction,QMenu) but found '{}'".format(type(action)))


def toQSettings(settings):
    """Utility function to convert supplied settings object to a qtpy.QtCore.QSettings"""
    if hasattr(settings, "qsettings"):  # workbench.config.user
        return settings.qsettings
    else:  # must be a QSettings already
        return settings


def ensure_widget_is_on_screen(widget):
    """If the supplied widget is off the screen it will be moved so it is on the screen.
    The widget must already be 'shown'"""
    # this gives the maximum screen number if the position is off screen
    desktop = QDesktopWidget()
    screen = desktop.screenNumber(widget.pos())

    # get the window size
    desktop_geom = desktop.availableGeometry(screen)
    # get the widget dimensions with any os added extras
    widget_geom = widget.frameGeometry()
    # and position it on the supplied desktop screen
    x = max(widget_geom.x(), desktop_geom.left())
    y = max(widget_geom.y(), desktop_geom.top())

    if x + widget_geom.width() > desktop_geom.right():
        x = desktop_geom.right() - widget_geom.width()
    if y + widget_geom.height() > desktop_geom.bottom():
        y = desktop_geom.bottom() - widget_geom.height()
    window_pos = QPoint(x, y)
    widget.move(window_pos)


def force_layer_backing_BigSur():
    # Force layer-backing on macOS >= Big Sur (11)
    # or the application hangs
    # A fix inside Qt is yet to be released:
    # https://codereview.qt-project.org/gitweb?p=qt/qtbase.git;a=commitdiff;h=c5d904639dbd690a36306e2b455610029704d821
    # A complication with Big Sur numbering means we check for 10.16 and 11:
    #   https://eclecticlight.co/2020/08/13/macos-version-numbering-isnt-so-simple/
    from packaging.version import Version
    import platform

    mac_vers = Version(platform.mac_ver()[0])
    if mac_vers >= Version("11") or mac_vers == Version("10.16"):
        os.environ["QT_MAC_WANTS_LAYER"] = "1"
