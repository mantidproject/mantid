""" Provides a Python Interface to setup and run a SANS reduction"""
from mantid.kernel import Logger


class Reducer(object):
    def __init__(self):
        super(Reducer, self).__init__()

    def clean(self):
        pass

    def assign_can(self, can_scatter, period):
        pass

    def assign_sample(self, sample_scatter, period):
        pass


class ReductionSingleton(object):
    instance = None

    def __new__(cls):
        if not ReductionSingleton.instance:
            ReductionSingleton.instance = Reducer()
        return ReductionSingleton.instance

    def __getattr__(self, name):
        return getattr(self.instance, name)

    def __setattr__(self, name):
        return setattr(self.instance, name)


# --------------------------------------------------------------------------------
# Old ISIS Commands
# -------------------------------------------------------------------------------

sans_log = Logger("SANS")


_VERBOSE_ = False
LAST_SAMPLE = None


# Print a message and log
def _print_message(msg, log=True, no_console=False):
    if log == True and _VERBOSE_ == True:
        sans_log.notice(msg)
    if not no_console:
        print msg

def Clean():
    """
    An exposed command to allow cleaning of the reducer, and any relate settings.
    """
    ReductionSingleton().clean()


def AssignSample(sample_run, reload=True, period=None):
    _ = reload
    sample_scatter = sample_run
    # TODO
    sample_scatter_period = period if period is not None else None
    ReductionSingleton.assign_sample(sample_scatter, sample_scatter_period)


def AssignCan(can_run, reload=True, period=None):
    _ = reload
    can_scatter = can_run
    # TODO
    can_scatter_period = period if period is not None else None
    ReductionSingleton.assign_can(can_scatter, can_scatter_period)


# -----------------------
# Deprecated
# -----------------------
def SANS2DTUBES():
    """
    Quick, temporary workaround for the IDF problem we're fixing in #9367.
    Simply pass the correct IDF to SANS2D().
    """
    pass


def LOQ(idf_path='LOQ_Definition_20020226-.xml'):
    """
        Initialises the instrument settings for LOQ
        @return True on success
    """
    pass


def LARMOR(idf_path = None):
    """
    Initialises the instrument settings for LARMOR
    @param idf_path :: optionally specify the path to the LARMOR IDF to use.
                       Uses default if none specified.
    @return True on success
    """
    pass