# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
class HelpWindowModel:
    def __init__(self):
        self.help_url = "https://docs.mantidproject.org/"

    def get_help_url(self):
        """Get the help documentation URL."""
        return self.help_url
