from __future__ import (absolute_import, division, print_function)
from recon.tools.abstract_tool import AbstractTool


class TomoPyTool(AbstractTool):

    def run_reconstruct(self, data, config):
        import numpy as np
        from recon.helper import Helper

        tomopy = self.import_self()

        h = Helper(config)

        h.check_data_stack(data)

        num_proj = data.shape[0]
        inc = float(config.pre.max_angle) / (num_proj - 1)

        projection_angles = np.arange(0, num_proj * inc, inc)

        projection_angles = np.radians(projection_angles)

        alg = config.func.algorithm
        cor = config.func.cor

        h.tomo_print(" * Using center of rotation: {0}".format(cor))

        if 'gridrec' != alg and 'fbp' != alg:
            if not config.func.num_iter:
                tomopy.cfg_num_iter = config.func.num_iter

            # For ref, some typical run times with 4 cores:
            # 'bart' with num_iter=20 => 467.640s ~= 7.8m
            # 'sirt' with num_iter=30 => 698.119 ~= 11.63
            h.pstart(
                " * Starting iterative method with TomoPy. Algorithm: {0}, "
                "number of iterations: {1}...".format(alg, config.func.num_iter))

            rec = tomopy.recon(
                tomo=data,
                theta=projection_angles,
                center=cor,
                algorithm=alg,
                num_iter=config.func.num_iter)  # , filter_name='parzen')

        else:
            h.pstart(
                " * Starting non-iterative reconstruction algorithm with TomoPy. "
                "Algorithm: {0}...".format(alg))
            rec = tomopy.recon(
                tomo=data,
                theta=projection_angles,
                center=cor,
                algorithm=alg)

        h.pstop(
            " * Reconstructed 3D volume. Shape: {0}, and pixel data type: {1}.".
            format(rec.shape, rec.dtype))

        return rec

    @staticmethod
    def import_self():
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
