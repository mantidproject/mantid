# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""The elements of this module manage and add specific entries in the sample log."""

# pylint: disable=invalid-name

from hashlib import sha224
from mantid.api import MatrixWorkspace, WorkspaceGroup


def get_hash_value(value):
    """
    Converts a value into a hash

    :param value: a hashable value
    :return: a hashed value.
    """
    hash_value = sha224(str(value).encode("utf8")).hexdigest()
    if not hash_value or hash_value is None:
        raise RuntimeError("SANSLogTagger: Something went wrong when trying to get the hash for {0}.".format(str(value)))
    return hash_value


def check_if_valid_tag_and_workspace(tag, workspace):
    """
    Checks if a tag and a workspace a valid for tags
    :param tag: the tag
    :param workspace: the workspace
    """
    if not (isinstance(tag, str) and isinstance(workspace, MatrixWorkspace)):
        raise RuntimeError(
            "SANSLogTagger: Either tag {0} or workspace are invalid. The tag has to be a string"
            " and the workspace {1} has to be a MatrixWorkspace".format(str(tag), str(workspace))
        )


def set_tag(tag, value, workspace):
    """
    Adds/ sets a tag on a workspace

    :param tag: the tag name
    :param value: the tag value
    :param workspace: the workspace to which the tag will be added.
    """
    check_if_valid_tag_and_workspace(tag, workspace)
    run = workspace.getRun()
    run.addProperty(tag, value, True)


def get_tag(tag, workspace):
    """
    Extracts a tag from the workspace.

    :param tag: the tag name
    :param workspace: the workspace with the tag.
    :return: the tag value if it exists else None
    """
    check_if_valid_tag_and_workspace(tag, workspace)
    run = workspace.getRun()
    return run[tag].value if tag in run else None


def has_tag(tag, workspace):
    """
    Checks if workspace has a certain tag.

    :param tag: the tag name.
    :param workspace: the workspace to check.
    :return: true if the tag exists else false.
    """
    if workspace is None:
        return False
    check_if_valid_tag_and_workspace(tag, workspace)
    run = workspace.getRun()
    return tag in run


def set_hash(tag, hashed_value, workspace):
    """
    Sets a value as a hashed tag on a workspace.

    :param tag: the tag name
    :param hashed_value: a hashed value
    :param workspace: the workspace
    """
    check_if_valid_tag_and_workspace(tag, workspace)
    set_tag(tag, hashed_value, workspace)


def has_hash(tag, hashed_value, workspace):
    """
    Checks if a hash exists on a workspace.

    :param tag: the tag as a hash.
    :param hashed_value: the hash which is being checked
    :param workspace: the workspace.
    :return: true if the hash exists on the workspace else false.
    """
    if isinstance(workspace, WorkspaceGroup):
        return False

    try:
        check_if_valid_tag_and_workspace(tag, workspace)
    except RuntimeError:
        return False

    if not has_tag(tag, workspace):
        return False

    saved_hash = get_tag(tag, workspace)
    return saved_hash == hashed_value
