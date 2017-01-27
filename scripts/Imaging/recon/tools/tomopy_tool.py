from __future__ import (absolute_import, division, print_function)
from recon.tools.abstract_tool import AbstractTool


class TomoPyTool(AbstractTool):

    @staticmethod
    def tool_supported_methods():
        return ['art', 'bart', 'fbp', 'gridrec', 'mlem',
                'osem', 'ospml_hybrid', 'ospml_quad', 'pml_hybrid', 'pml_quad', 'sirt']

    def __init__(self):
        AbstractTool.__init__(self)
        self._tomopy = self.import_self()

    def check_algorithm_compatibility(self, config):
        algorithm = config.func.algorithm
        if algorithm not in TomoPyTool.tool_supported_methods():
            raise ValueError(
                "The selected algorithm {0} is not supported by TomoPy.".format(algorithm))

    def find_center(self, **kwargs):
        # just forward to tomopy
        return self._tomopy.find_center(**kwargs)

    def circ_mask(self, **kwargs):
        # just forward to tomopy
        return self._tomopy.circ_mask(**kwargs)

    def import_self(self):
        try:
            import tomopy
            import tomopy.prep
            import tomopy.recon
            import tomopy.misc
            import tomopy.io

        except ImportError as exc:
            raise ImportError("Could not import the tomopy package and its subpackages. Details: {0}".
                              format(exc))

        return tomopy

    def run_reconstruct(self, data, config, h):
        import numpy as np
        from recon.helper import Helper
        h = Helper.empty_init() if h is None else h

        h.check_data_stack(data)

        num_proj = data.shape[0]
        inc = float(config.func.max_angle) / num_proj

        proj_angles = np.arange(0, num_proj * inc, inc)

        proj_angles = np.radians(proj_angles)

        alg = config.func.algorithm
        cor = config.func.cor
        num_iter = config.func.num_iter
        cores = config.func.cores

        iterative_algorithm = False if alg in ['gridrec', 'fbp'] else True

        # run the iterative algorithms
        if iterative_algorithm:
            h.pstart(
                "Starting iterative method with TomoPy. Center of Rotation: {0}, Algorithm: {1}, "
                "number of iterations: {2}...".format(cor, alg, num_iter))

            recon = self._tomopy.recon(tomo=data, theta=proj_angles, center=cor,
                                       algorithm=alg, num_iter=num_iter, ncore=cores)

        else:  # run the non-iterative algorithms
            h.pstart(
                "Starting non-iterative reconstruction algorithm with TomoPy. "
                "Center of Rotation: {0}, Algorithm: {1}...".format(cor, alg))
            recon = self._tomopy.recon(
                tomo=data, theta=proj_angles, center=cor, ncore=cores, algorithm=alg)

        h.pstop(
            "Reconstructed 3D volume. Shape: {0}, and pixel data type: {1}.".
            format(recon.shape, recon.dtype))

        return recon
