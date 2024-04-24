# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.


class ObservingPresenter(object):
    """
    This class provides some common functions for classes that need to be observable.
    It is not a GUI class, and if used, should be inherited by the presenter.
    It is designed to be used with a view that inherits `ObservingView`.

    The presenter that is inheriting this must initialise a model and a view,
    that implement the methods required in the functions below.
    """

    def clear_observer(self):
        """
        If the observer is not cleared here then the C++ object is never freed,
        and observers keep getting created, and triggering on ADS events.

        This method is called from the view's close_later to ensure that
        observer are deleted both when the workspace is deleted and when
        the view window is closed manually (via the X)
        """
        self.ads_observer = None

    def close(self, workspace_name):
        if self.current_workspace_equals(workspace_name):
            self.clear_observer()
            self.container.emit_close()

    def force_close(self):
        self.clear_observer()
        self.container.emit_close()

    def current_workspace_equals(self, name):
        """
        This function is provided as a means of objects inheriting to override the
        workspace name comparison based on difference in storage.

        An example is the InstrumentWidget presenter, which doesn't have a model,
        but a variable holding the workspace name
        :param name: Name of the workspace
        :return: True if the name matches the current workspace, otherwise False
        """
        return self.model.workspace_equals(name)

    def replace_workspace(self, workspace_name, workspace):
        if self.model.workspace_equals(workspace_name):
            self.container.emit_replace(workspace_name, workspace)

    def rename_workspace(self, old_name, new_name):
        if self.model.workspace_equals(old_name):
            self.container.emit_rename(new_name)

    def group_update(self, ws_name, workspace):
        return
