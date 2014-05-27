"""
    mantiddoc.tools.screenshot
    ~~~~~~~~~~~~~~~~~~~~~~~~~~

    Provides functions to take a screenshot of a QWidgets.

    It currently assumes that the functions are called within the
    MantidPlot Python environment

    :copyright: Copyright 2014
        ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory
"""

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
        return "NoGUI-ImageNotGenerated.png"

    import mantidqtpython as mantidqt
    from mantidplot import screenshot_to_dir, threadsafe_call

    iface_mgr = mantidqt.MantidQt.API.InterfaceManager()
    # threadsafe_call required for MantidPlot
    dlg = threadsafe_call(iface_mgr.createDialogFromName, name, True, None)

    suffix = ("-v%d" % version) if version != -1 else ""
    filename = "%s%s_dlg%s" % (name, suffix, ext)

    img_path = screenshot_to_dir(widget=dlg, filename=filename, screenshot_dir=directory)
    threadsafe_call(dlg.close)

    return img_path
