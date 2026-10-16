# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
"""Base for the throwaway interface a tutorial drives."""


class InterfaceSandbox:
    """A second instance of an interface for the tour to drive. It is also the context every
    tutorial step receives, which is why it exposes the model, view and presenter."""

    def __init__(self, model, view, presenter):
        self.model = model
        self.view = view
        self.presenter = presenter
        self._torn_down = False

    @property
    def window(self):
        return self.view

    def teardown(self):
        """Close the sandbox interface. Safe to call more than once.

        Closing the view is what should release whatever the interface owns (e.g. its workspaces),
        through its own ``closeEvent``, exactly as when the user closes it.
        """
        if self._torn_down:
            return
        self._torn_down = True
        self.view.close()
        self.view.deleteLater()
        self._cleanup()

    def _cleanup(self):
        """Release anything the sandbox itself created, such as demo files."""
