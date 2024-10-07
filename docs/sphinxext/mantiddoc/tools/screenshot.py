"""
mantiddoc.tools.screenshot
~~~~~~~~~~~~~~~~~~~~~~~~~~

Provides functions to take a screenshot of a QWidgets.

:copyright: Copyright 2020
    ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory
"""

from functools import wraps
import os
from mantidqt.interfacemanager import InterfaceManager
from qtpy.QtGui import QPixmap
from qtpy.QtWidgets import QApplication, QWidget

# Global QApplication instance
_QAPP = None


def _ensure_qapplication_started(func):
    """
    Decorator to ensure QApplication has been created
    """

    @wraps(func)
    def wrapper(*args, **kwargs):
        global _QAPP
        if QApplication.instance() is None:
            _QAPP = QApplication(["screenshot"])
        return func(*args, **kwargs)

    return wrapper


@_ensure_qapplication_started
def algorithm_screenshot(name: str, directory: str, version: int = -1, ext: str = ".png") -> str:
    """
    Takes a snapshot of an algorithm dialog and saves it as an image
    named "name_dlg.png"

    Args:
      name: The name of the algorithm
      directory: An directory path where the image should be saved
      version: A version of the algorithm to use (default=latest)
      ext: An optional extension (including the period). Default=.png

    Returns:
      A full path to the image file
    """
    ensure_directory_exists(directory)

    suffix = ""
    if version != -1:
        suffix = f"-v{version}"
    filename = os.path.join(directory, f"{name}{suffix}_dlg{ext}")
    manager = InterfaceManager()
    dialog = manager.createDialogFromName(name, version, None, True)
    dialog.adjustSize()
    try:
        take_picture(dialog, filename)
        picture = Screenshot(filename, dialog.width(), dialog.height())
    finally:
        dialog.close()

    return picture


@_ensure_qapplication_started
def custominterface_screenshot(name: str, directory: str, ext: str = ".png", widget_name: str = None):
    """
    Takes a snapshot of a custom interface and saves it as an image
    named "name.png"

    Args:
      name: The name of the custom interface
      directory: An directory path where the image should be saved
      ext: An optional extension (including the period). Default=.png
      widget_name: An optional child widget of the interface to snapshot

    Returns:
      str: A full path to the image file
    """
    ensure_directory_exists(directory)

    iface_mgr = InterfaceManager()
    window = iface_mgr.createSubWindow(name)
    if window is None:
        raise RuntimeError(f"Interface '{name}' could not be created")
    window.adjustSize()

    image_widget = window
    if widget_name is not None:
        image_widget = window.findChild(QWidget, widget_name)
        if image_widget is None:
            raise RuntimeError(f"Widget '{widget_name}' does not exist in interface '{name}'")
        filename = name.replace(" ", "_") + "_" + widget_name + "_widget" + ext
    else:
        filename = name.replace(" ", "_") + "_interface" + ext
    filepath = os.path.join(directory, filename)
    try:
        take_picture(image_widget, filepath)
        picture = Screenshot(filepath, image_widget.width(), image_widget.height())
    finally:
        window.close()

    return picture


def ensure_directory_exists(directory: str):
    """
    If the given directory does not exist then create it
    :param directory: The path to a directory that might not exist
    """
    if not os.path.exists(directory):
        os.makedirs(directory)


def take_picture(widget: QWidget, filename: str) -> str:
    """
    Takes a screenshot and saves it to the
    filename given, ensuring the call is processed
    through a slot if the call is from a separate
    thread
    """
    if hasattr(widget, "grab"):
        pix = widget.grab()
    else:
        pix = QPixmap.grabWidget(widget)
    success = pix.save(filename)
    if not success:
        raise RuntimeError(f"Error saving widget image to file '{filename}'")


class Screenshot:
    """
    Captures the filename + meta information about it a screeshot
    """

    def __init__(self, filename: str, width: float, height: float):
        """
        :param filename: Filename of existing image
        :param width: The width of the widget
        :param height: The height of the widget
        """
        self.imgpath = filename
        self.width = width
        self.height = height
