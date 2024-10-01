# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +


def get_plotting_module():
    """
    :returns: The plotting module to use. Can be None if Qt is not accessible.
    """
    plotting_module = None
    try:
        import mantidqt.plotting.functions as plotting_module
    except ImportError as exc:
        from mantid.kernel import logger

        logger.debug("Unable to import plotting module {}".format(str(exc)))

    return plotting_module
