from __future__ import (absolute_import, division, print_function)
from recon.tools.abstract_tool import AbstractTool


class AstraTool(AbstractTool):
    """
    Uses TomoPy's integration of Astra
    """

    @staticmethod
    def tool_supported_methods():
        return ['FP', 'FP_CUDA', 'BP', 'BP_CUDA', 'FBP',
                'FBP_CUDA', 'SIRT', 'SIRT_CUDA', 'SART', 'SART_CUDA', 'CGLS', 'CGLS_CUDA']

    def __init__(self):
        AbstractTool.__init__(self)

        # we import tomopy so that we can use Astra through TomoPy's
        # implementation
        self._tomopy = self.import_self()

    def import_self(self):
        # use Astra through TomoPy
        from recon.tools.tomopy_tool import TomoPyTool
        t = TomoPyTool()
        return t.import_self()

    def check_algorithm_compatibility(self, config):
        algorithm = config.func.algorithm.upper()  # get full caps

        if algorithm not in AstraTool.tool_supported_methods():
            raise ValueError(
                "The selected algorithm {0} is not supported by Astra.".format(algorithm))

    @staticmethod
    def _import_astra():
        try:
            import astra
        except ImportError as exc:
            raise ImportError("Cannot find and import the astra toolbox package: {0}".
                              format(exc))

        min_astra_version = 1.8
        astra_version = astra.__version__
        if isinstance(astra_version, float) and astra_version >= min_astra_version:
            print("Imported astra successfully. Version: {0}".format(
                astra_version))
        else:
            raise RuntimeError(
                "Could not find the required version of astra. Found version: {0}".format(astra_version))

        print("Astra using CUDA: {0}".format(astra.astra.use_cuda()))
        return astra

    def run_reconstruct(self, data, config, h):
        import numpy as np
        from recon.helper import Helper

        h = Helper.empty_init() if h is None else h

        h.check_data_stack(data)

        num_proj = data.shape[0]
        inc = float(config.func.max_angle) / num_proj

        proj_angles = np.arange(0, num_proj * inc, inc)

        proj_angles = np.radians(proj_angles)

        alg = config.func.algorithm.upper()  # get upper case
        cor = config.func.cor
        num_iter = config.func.num_iter
        cores = config.func.cores

        # remove xxx_CUDA from the string with the [0:find..]
        iterative_algorithm = False if alg[
            0:alg.find('_')] in ['FBP', 'FB', 'BP'] else True

        # TODO needs to be in config
        # are we using a CUDA algorithm
        proj_type = 'cuda' if alg[alg.find('_') + 1:] == 'CUDA' else 'linear'

        # run the iterative algorithms
        if iterative_algorithm:
            options = {
                'proj_type': proj_type,
                'method': alg
            }

            h.pstart(
                "Starting iterative method with Astra. Center of Rotation: {0}, Algorithm: {1}, "
                "number of iterations: {2}...".format(cor, alg, num_iter))

            recon = self._tomopy.recon(tomo=data, theta=proj_angles,
                                       center=cor, ncores=cores, algorithm=self._tomopy.astra, options=options)

        else:  # run the non-iterative algorithms

            h.pstart(
                "Starting non-iterative reconstruction algorithm with Astra. "
                "Center of Rotation: {0}, Algorithm: {1}...".format(cor, alg))

            options = {
                'proj_type': proj_type,
                'method': alg
            }

            recon = self._tomopy.recon(tomo=data, theta=proj_angles,
                                       center=cor, ncores=cores, algorithm=self._tomopy.astra, options=options)

        h.pstop(
            "Reconstructed 3D volume. Shape: {0}, and pixel data type: {1}.".
            format(recon.shape, recon.dtype))

        return recon

    def astra_reconstruct(self, data, config):
        import tomorec.tool_imports as tti
        astra = tti.import_tomo_tool('astra')

        sinograms = np.swapaxes(data, 0, 1)

        plow = (data.shape[2] - cor * 2)
        phigh = 0

        # minval = np.amin(sinograms)
        sinograms = np.pad(sinograms, ((0, 0), (0, 0), (plow, phigh)),
                           mode='reflect')

        proj_geom = astra.create_proj_geom('parallel3d', .0, 1.0,
                                           data.shape[1],
                                           sinograms.shape[2], proj_angles)
        sinogram_id = astra.data3d.create('-sino', proj_geom, sinograms)

        vol_geom = astra.create_vol_geom(
            data.shape[1], sinograms.shape[2], data.shape[1])
        recon_id = astra.data3d.create('-vol', vol_geom)
        alg_cfg = astra.astra_dict(alg_cfg.algorithm)
        alg_cfg['ReconstructionDataId'] = recon_id
        alg_cfg['ProjectionDataId'] = sinogram_id
        alg_id = astra.algorithm.create(alg_cfg)

        number_of_iters = 100
        astra.algorithm.run(alg_id, number_of_iters)
        recon = astra.data3d.get(recon_id)

        astra.algorithm.delete(alg_id)
        astra.data3d.delete(recon_id)
        astra.data3d.delete(sinogram_id)

        return recon
