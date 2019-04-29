from mantid.py3compat import Enum


class MantidAxType(Enum):
    BIN = 0
    SPECTRUM = 1


class MantidAxKwargs(object):
    ERRORS_VISIBLE = "errors_visible"
