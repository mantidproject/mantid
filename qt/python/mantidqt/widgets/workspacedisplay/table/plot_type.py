from mantid.py3compat import Enum


class PlotType(Enum):
    LINEAR = 1
    SCATTER = 2
    LINE_AND_SYMBOL = 3
    LINEAR_WITH_ERR = 4
    SCATTER_WITH_ERR = 5
