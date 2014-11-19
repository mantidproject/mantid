"""
    mantiddoc.tools.screenshot
    ~~~~~~~~~~~~~~~~~~~~~~~~~~

    Provides functions to take a screenshot of a QWidgets.

    It currently assumes that the functions are called within the
    MantidPlot Python environment

    :copyright: Copyright 2014
        ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory
"""
#--------------------------------------------------------------------------
class Screenshot(object):
    """
    Takes a screenshot of widget records the filename + meta information
    about it.
    """

    def __init__(self, widget, filename, directory):
        from mantidplot import screenshot_to_dir, threadsafe_call

        self.imgpath = screenshot_to_dir(widget=widget, filename=filename, screenshot_dir=directory)
        self.width = widget.width()
        self.height = widget.height()

#--------------------------------------------------------------------------

def algorithm_screenshot(name, directory, version = -1, ext = ".png"):
    """
    Takes a snapshot of an algorithm dialog and saves it as an image
    named "name_dlg.png"

    Args:
      name (str): The name of the algorithm
      directory (str): An directory path where the image should be saved
      version (str): A version of the algorithm to use (default=latest)
      ext (str): An optional extension (including the period). Default=.png

    Returns:
      str: A full path to the image file
    """
    import mantid
    if not mantid.__gui__:
        raise RuntimeError("MantidPlot not available. Cannot take screenshot")

    import mantidqtpython as mantidqt
    from mantidplot import threadsafe_call

    iface_mgr = mantidqt.MantidQt.API.InterfaceManager()
    # threadsafe_call required for MantidPlot
    dlg = threadsafe_call(iface_mgr.createDialogFromName, name, version, None, True)

    suffix = ("-v%d" % version) if version != -1 else ""
    filename = "%s%s_dlg%s" % (name, suffix, ext)

    picture = Screenshot(dlg, filename, directory)
    threadsafe_call(dlg.close)
    return picture

#--------------------------------------------------------------------------

def custominterface_screenshot(name, directory, ext = ".png", widget_name = None):
    """
    Takes a snapshot of a custom interface and saves it as an image
    named "name.png"

    Args:
      name (str): The name of the custom interface
      directory (str): An directory path where the image should be saved
      ext (str): An optional extension (including the period). Default=.png

    Returns:
      str: A full path to the image file
    """
    import mantid
    if not mantid.__gui__:
        raise RuntimeError("MantidPlot not available. Cannot take screenshot")

    import mantidqtpython as mantidqt
    from mantidplot import threadsafe_call
    from PyQt4.QtGui import QWidget

    iface_mgr = mantidqt.MantidQt.API.InterfaceManager()
    # threadsafe_call required for MantidPlot
    dlg = threadsafe_call(iface_mgr.createSubWindow, name, None)

    if widget_name:
      widget = dlg.findChild(QWidget, widget_name)
      picture = Screenshot(widget, name.replace(' ','_') + "_" + widget_name + "_widget" + ext, directory)
    else:
      picture = Screenshot(dlg, name.replace(' ','_') + "_interface" + ext, directory)
    threadsafe_call(dlg.close)
    return picture
