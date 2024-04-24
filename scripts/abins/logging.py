from logging import Logger as PythonLogger
from typing import Optional

from mantid.kernel import logger as mantid_kernel_logger
from mantid.kernel import Logger as MantidKernelLogger


Logger = PythonLogger | MantidKernelLogger


def get_logger(logger: Optional[Logger] = None) -> Logger:
    """Get Mantid kernel logger if no logging.Logger provided"""

    if logger is None:
        return mantid_kernel_logger
    else:
        return logger
