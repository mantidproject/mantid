# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
kernel
=============

Defines Python objects that wrap the C++ Kernel namespace.

"""
from __future__ import (absolute_import, division,
                        print_function)

import os as _os
from contextlib import contextmanager

@contextmanager
def _shared_cextension():
    """Our extensions need to shared symbols amongst them due to:
      - the static boost python type registry
      - static singleton instances marked as weak symbols by clang
    gcc uses an extension to mark these attributes as globally unique
    but clang marks them as weak and without RTLD_GLOBAL each shared
    library has its own copy of each singleton.

    See https://docs.python.org/3/library/sys.html#sys.setdlopenflags
    """
    import sys
    if not sys.platform.startswith('linux'):
        yield
        return

    import six
    if six.PY2:
        import DLFCN as dl
    else:
        import os as dl
    flags_orig = sys.getdlopenflags()
    sys.setdlopenflags(dl.RTLD_NOW | dl.RTLD_GLOBAL)
    yield
    sys.setdlopenflags(flags_orig)


# Imports boost.mpi if applicable
from . import mpisetup

###############################################################################
# Load the C++ library
###############################################################################
from ..utils import import_mantid
with _shared_cextension():
    _kernel = import_mantid('._kernel', 'mantid.kernel')

###############################################################################
# Make modules available in this namespace
###############################################################################
from . import environment
from . import funcinspect
from ._aliases import *

# module alias for backwards-compatability in user scripts
funcreturns = funcinspect

###############################################################################
# Do site-specific setup for packages
###############################################################################
from . import packagesetup as _mantidsite
_mantidsite.set_NEXUSLIB_var()
