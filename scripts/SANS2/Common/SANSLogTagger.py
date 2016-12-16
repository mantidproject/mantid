# pylint: disable=invalid-name

from hashlib import sha224
from mantid.api import MatrixWorkspace


def get_hash_value(value):
    hash_value = sha224(str(value).encode("utf8")).hexdigest()
    if not hash_value or hash_value is None:
        raise RuntimeError("SANSLogTagger: Something went wrong when trying to get the hash for {0}.".format(str(value)))
    return hash_value


def check_if_valid_tag_and_workspace(tag, workspace):
    if not(isinstance(tag, basestring) and isinstance(workspace, MatrixWorkspace)):
        raise RuntimeError("SANSLogTagger: Either tag {0} or workspace are invalid. The tag has to be a string"
                           " and the workspace {1} has to be a MatrixWorkspace".format(str(tag), str(workspace)))


def set_tag(tag, value, workspace):
    check_if_valid_tag_and_workspace(tag, workspace)
    run = workspace.getRun()
    run.addProperty(tag, value, True)


def get_tag(tag, workspace):
    check_if_valid_tag_and_workspace(tag, workspace)
    run = workspace.getRun()
    return run[tag].value if tag in run else None


def has_tag(tag, workspace):
    check_if_valid_tag_and_workspace(tag, workspace)
    run = workspace.getRun()
    return tag in run


def set_hash(tag, value, workspace):
    check_if_valid_tag_and_workspace(tag, workspace)
    hash_value = get_hash_value(str(value))
    set_tag(tag, hash_value, workspace)


def has_hash(tag, value, workspace):
    check_if_valid_tag_and_workspace(tag, workspace)
    same_hash = False
    if has_tag(tag, workspace):
        saved_hash = get_tag(tag, workspace)
        to_check_hash = get_hash_value(str(value))
        same_hash = True if saved_hash == to_check_hash else False
    return same_hash
