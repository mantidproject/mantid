# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.kernel import logger

try:
    import pytest
    from mantid.simpleapi import mtd, DeleteWorkspace
    from typing import List

    logger.debug("Pytest fixtures available")

    @pytest.fixture
    def clean_workspace():
        r"""
        Fixture that will delete workspaces if registered in the Analysis Data Service when the test function exits.


        :param str, ~mantid.kernel.Workspace workspace: Workspace handle or the name of the workspace to be deleted
        :yields: the name of the workspace marked for deletion
        """
        workspaces: List[str] = []

        def _clean_workspace(workspace):
            workspaces.append(str(workspace))
            return str(workspace)

        yield _clean_workspace

        workspaces = list(set(workspaces))
        for workspace in workspaces:
            if mtd.doesExist(str(workspace)):
                try:
                    DeleteWorkspace(str(workspace))
                    logger.debug(f"cleaned {workspace} workspace")

                except ValueError as ex:
                    if "Invalid value for property Workspace" not in str(ex):
                        raise ex

    @pytest.fixture
    def temp_workspace_name(clean_workspace):
        r"""
        Fixture that returns a string guaranteed not to represent an already existing workspace so that it can be
        associated to a new workspace. The workspace will be deleted when the function exists or upon exception.
        This fixture has no input parameters and depends on the clean_workspace fixture.

        :yields: unique string to be used as a workspace name
        """

        def _temp_workspace():
            name = mtd.unique_hidden_name()
            clean_workspace(name)
            logger.debug(f"created {name} workspace")
            return name

        return _temp_workspace

except ImportError:
    logger.warning("Pytest not installed, pytest fixtures are unavailable")
