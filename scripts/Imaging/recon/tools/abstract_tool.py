from __future__ import (absolute_import, division, print_function)


class AbstractTool:
    """
    The AbstractTool class requires the tools to know how to import themselves, and how to run their reconstructions.
    Currently there can only be one reconstruction type supported, and this will have to be changed in future revisions.
    """

    def __init__(self):
        pass

    def run_reconstruct(self, data, config):
        raise NotImplementedError("This is an abstract class. Use a derived class' implementation")

    @staticmethod
    def import_self():
        raise NotImplementedError("This is an abstract class. Use a derived class' implementation")
