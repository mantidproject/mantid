# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#

class RawDataExplorerModel(object):
    """
    TODO
    """

    def __init__(self, presenter):
        """
        Initialise the model, keeping references to the presenter and
        GlobalFigureManager
        :param presenter: The presenter controlling this model
        """
        self.presenter = presenter
