# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +


def get_logger(name, app):
    """
    Compatability layer between Sphinx v1.2 & latest
    to wrap changes in logging
    :param name: The name of the logger (unused in Sphinx < v2)
    :param app: The application object (unused in Sphinx > v2)
    :return:
    """
    try:
        # Sphinx > v2
        from sphinx.util import logging

        return logging.getLogger(name)
    except ImportError:
        return app
