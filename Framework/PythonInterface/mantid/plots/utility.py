from mantid.py3compat import Enum


class MantidAxType(Enum):
    BIN = 0
    SPECTRUM = 1


class MantidAxKwargs(object):
    ERRORS_VISIBLE = "errors_visible"


def find_errorbar_container(line, containers):
    """
    Finds the ErrorbarContainer associated with the plot line.

    :param line: Line that is looked for
    :param containers: Collection of containers that contain `ErrorbarContainer`s
    :return: The container that contains the line
    """
    for container in containers:
        if line == container[0]:
            return container
