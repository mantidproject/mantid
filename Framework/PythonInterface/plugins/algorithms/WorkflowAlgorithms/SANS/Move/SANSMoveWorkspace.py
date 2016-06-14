from abc import (ABCMeta, abstractmethod)
from State.SANSStateMoveWorkspace import (SANSStateMoveWorkspace, SANSStateMoveWorkspaceLOQ)


# -------------------------------------------------
# Move classes
# -------------------------------------------------
class SANSMove(object):
    __metaclass__ = ABCMeta

    @abstractmethod
    def do_execute(self, move_info):
        pass

    def execute(self, move_info):
        SANSMove._validate(move_info)
        return self.do_execute(move_info)

    @staticmethod
    def _validate(move_info):
        if not isinstance(move_info, SANSStateMoveWorkspace):
            raise ValueError("SANSMove: The provided state information is of the wrong type. It must be"
                             " of type SANSStateMoveWorkspace, but was {}".format(str(type(move_info))))
        move_info.validate()


class SANSMoveLOQ(SANSMove):
    def do_execute(self, move_info):
        pass


class SANSMoveFactory(object):
    def __init__(self):
        super(SANSMoveFactory, self).__init__()

    @staticmethod
    def _get_instrument_type(move):
        return move.instrument_type

    @staticmethod
    def create_mover(move):
        instrument_type = SANSMoveFactory._get_instrument_type(data)
        if instrument_type is SANSInstrument.LARMOR or instrument_type is SANSInstrument.LOQ or\
           instrument_type is SANSInstrument.SANS2D:
            loader = SANSLoadISIS()
        else:
            loader = None
            NotImplementedError("SANSLoaderFactory: Other instruments are not implemented yet.")
        return loader

