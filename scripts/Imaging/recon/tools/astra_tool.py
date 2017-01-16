from __future__ import (absolute_import, division, print_function)
from recon.tools.abstract_tool import AbstractTool


class AstraTool(AbstractTool):
    """
    Uses TomoPy's integration of Astra
    """

    def __init__(self):
        AbstractTool.__init__(self)
        self._tomopy = self.import_self()

    def run_reconstruct(self, data, config):
        import numpy as np
        from recon.helper import Helper

        h = Helper(config)

        h.check_data_stack(data)

        num_proj = data.shape[0]
        inc = float(config.func.max_angle) / (num_proj - 1)

        proj_angles = np.arange(0, num_proj * inc, inc)

        proj_angles = np.radians(proj_angles)

        alg = config.func.algorithm.upper()  # get upper case
        cor = config.func.cor
        num_iter = config.func.num_iter

        h.tomo_print(" * Using center of rotation: {0}".format(cor))

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
                " * Starting iterative method with Astra. Algorithm: {0}, "
                "number of iterations: {1}...".format(alg, num_iter))

            recon = self._tomopy.recon(tomo=data, theta=proj_angles,
                                       center=cor, algorithm=self._tomopy.astra, options=options)
        else:  # run the non-iterative algorithms

            h.pstart(
                " * Starting non-iterative reconstruction algorithm with Astra. "
                "Algorithm: {0}...".format(alg))

            options = {
                'proj_type': proj_type,
                'method': alg
            }

            recon = self._tomopy.recon(tomo=data, theta=proj_angles,
                                       center=cor, algorithm=self._tomopy.astra, options=options)

        h.pstop(
            " * Reconstructed 3D volume. Shape: {0}, and pixel data type: {1}.".
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

    def import_self(self):
        # use Astra through TomoPy
        from recon.tools.tomopy_tool import TomoPyTool
        t = TomoPyTool()
        return t.import_self()

    def _import_astra(self):
        # current astra distributions install here, so check there by default
        ASTRA_LOCAL_PATH = '/usr/local/python/'
        import sys
        sys.path.append(ASTRA_LOCAL_PATH)
        try:
            import astra
        except ImportError as exc:
            raise ImportError("Cannot find and import the astra toolbox package: {0}".
                              format(exc))

        MIN_ASTRA_VERSION = 106
        vers = astra.astra.version()
        if isinstance(vers, int) and vers >= MIN_ASTRA_VERSION:
            print("Imported astra successfully. Version: {0}".format(
                astra.astra.version()))
        else:
            raise RuntimeError(
                "Could not find the required version of astra. Found version: {0}".format(vers))

        print("Astra using CUDA: {0}".format(astra.astra.use_cuda()))
        return astra


def astra_reconstruct3d(self, sinogram, angles, depth, alg_cfg):
    """
    Run a reconstruction with astra

    @param sinogram :: sinogram data
    @param angles :: angles of the image projections
    @param depth :: number of rows in images/sinograms
    @param alg_cfg :: tool/algorithm configuration
    """
    # Some of these have issues depending on the GPU setup
    algs_avail = "[FP3D_CUDA], [BP3D_CUDA]], [FDK_CUDA], [SIRT3D_CUDA], [CGLS3D_CUDA]"

    if alg_cfg.algorithm.upper() not in algs_avail:
        raise ValueError(
            "Invalid algorithm requested for the Astra package: {0}. "
            "Supported algorithms: {1}".format(alg_cfg.algorithm,
                                               algs_avail))
    det_rows = sinogram.shape[0]
    det_cols = sinogram.shape[2]

    vol_geom = astra.create_vol_geom(sinograms.shape[0], depth,
                                     sinogram.shape[2])
    proj_geom = astra.create_proj_geom('parallel3d', 1.0, 1.0, det_cols,
                                       det_rows, np.deg2rad(angles))

    sinogram_id = astra.data3d.create("-sino", proj_geom, sinogram)
    # Create a data object for the reconstruction
    rec_id = astra.data3d.create('-vol', vol_geom)

    cfg = astra.astra_dict(alg_cfg.algorithm)
    cfg['ReconstructionDataId'] = rec_id
    cfg['ProjectionDataId'] = sinogram_id

    # Create the algorithm object from the configuration structure
    alg_id = astra.algorithm.create(cfg)
    # This will have a runtime in the order of 10 seconds.
    astra.algorithm.run(alg_id, alg_cfg.num_iter)
    # This could be used to check the norm of the difference between the projection data
    # and the forward projection of the reconstruction.
    # if "CUDA" in cfg_alg.algorithm and "FBP" not cfg_alg.algorithm:
    # self.norm_diff += astra.algorithm.get_res_norm(alg_id)**2
    # print math.sqrt(self.norm_diff)

    # Get the result
    rec = astra.data3d.get(rec_id)

    astra.algorithm.delete(alg_id)
    astra.data3d.delete(rec_id)
    astra.data3d.delete(sinogram_id)

    return rec


def run_reconstruct_3d_astra(self, data, proj_angles, alg_cfg):
    """
    Run a reconstruction with astra, approach based on swpapping axes

    @param data :: projection images
    @param proj_angles :: angles corresponding to the projection images
    @param alg_cfg :: tool/algorithm configuration
    """

    def get_max_frames(algorithm):
        frames = 8 if "3D" in algorithm else 1
        return frames

    nSinos = get_max_frames(alg_cfg.algorithm)
    iterations = alg_cfg.num_iter
    print(" astra recon - doing {0} iterations".format(iterations))

# swaps outermost dimensions so it is sinogram layout
# sinogram = data
# sinogram = np.swapaxes(sinogram, 0, 1)

# Needs to be figured out better
# ctr = cor
# width = sinogram.shape[1]
# pad = 50

# sino = np.nan_to_num(1. / sinogram)

# pad the array so that the centre of rotation is in the middle
# alen = ctr
# blen = width - ctr
# mid = width / 2.0

# if ctr > mid:
# plow = pad
# phigh = (alen - blen) + pad
# else:
# plow = (blen - alen) + pad
# phigh = pad

# logdata = np.log(sino + 1)

# sinogram = np.tile(sinogram.reshape((1, ) + sinogram.shape), (8, 1, 1))

# rec = self.astra_reconstruct3d(
#     sinogram, proj_angles, depth=nSinos, alg_cfg=alg_cfg)

# return rec
