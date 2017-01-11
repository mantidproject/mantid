from __future__ import (absolute_import, division, print_function)


class AbstractTool:
    def __init__(self):
        pass

    def run_reconstruct(self, data, config):
        raise NotImplementedError("This is an abstract class. Use a derived class' implementation")

    @staticmethod
    def import_self():
        raise NotImplementedError("This is an abstract class. Use a derived class' implementation")
