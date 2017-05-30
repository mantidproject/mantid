from __future__ import (absolute_import, division, print_function)


class AbstractTool:
    """
    The AbstractTool class requires the tools to know how to import themselves, and how to run their reconstructions.
    Currently there can only be one reconstruction tool supported, and this might have to be changed in future revisions.
    """

    def __init__(self):
        # import the tool to make sure the package is available
        self.import_self()

    @staticmethod
    def tool_supported_methods():
        raise NotImplementedError(
            "This is an abstract class. Use a derived class' implementation")

    @staticmethod
    def check_algorithm_compatibility(algorithm):
        raise NotImplementedError(
            "This is an abstract class. Use a derived class' implementation")

    def run_reconstruct(self, data, config, proj_angles=None, **kwargs):
        raise NotImplementedError(
            "This is an abstract class. Use a derived class' implementation")

    def import_self(self):
        raise NotImplementedError(
            "This is an abstract class. Use a derived class' implementation")
