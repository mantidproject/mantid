# Copyright &copy; 2014,2015 ISIS Rutherford Appleton Laboratory, NScD
# Oak Ridge National Laboratory & European Spallation Source
#
# This file is part of Mantid.
# Mantid is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# Mantid is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# File change history is stored at: <https://github.com/mantidproject/mantid>.
# Code Documentation is available at: <http://doxygen.mantidproject.org>
"""
Do a tomographic reconstruction, including:
- Pre-processing of input raw images,
- 3d volume reconstruction using a third party tomographic reconstruction tool
- Post-processing of reconstructed volume
- Saving reconstruction results (and pre-processing results, and self-save this script and subpackages)

Example command lines:

ipython -- tomo_reconstruct.py --help

ipython -- tomo_reconstruct.py\
 --input-path=stack_larmor_metals_summed_all_bands/ --output-path=recon_out_larmor_metals_fbp500\
 --tool tomopy --algorithm sirt --cor 129

ipython -- tomo_reconstruct.py\
 --input-path=CannonTomo/SampleA/ --output-path=recon_out_canon_a\
 --tool tomopy --algorithm gridrec --cor 504


# more up-to-date:

ipython -- /home/fedemp/mantid-repos/mantid-tomo/mantid/scripts/Imaging/IMAT/tomo_reconstruct.py\
 --input-path=../tomography-tests/stack_larmor_metals_summed_all_bands/ --output-path=REMOVE_ME\
 --tool tomopy --algorithm gridrec  --cor 123 --max-angle 360 --in-img-format=tiff\
 --region-of-interest='[5, 252, 507, 507]'

"""
import os
import sys
import time

import numpy as np


# Median filter
try:
    import scipy
except ImportError:
    raise ImportError("Could not find the package scipy which is required for image pre-/post-processing")

try:
    from scipy import ndimage
except ImportError:
    raise ImportError("Could not find the subpackage scipy.ndimage, required for image pre-/post-processing")



import tomorec.io as tomoio




# run_reconstruct_3d( ... tool='astra' ...)

# TODO: air-region (normalize proton charge) <- address

# TODO: The readers must take the prefix from the group of files with the largest number of files
# DC_00...03, vs. tomo_00...tomo_600

# formats converters (individual files, whole stacks), tiff->fits; fits->tiff

# defaults for MCP: median_filter=3, rotate=-1, crop=[0,  252, 0, 512], MCP correction: on

# MCP correction - horizontal line!!!
#  normalize proton charge
#
#  /work/imat/scripts/tomopy-0.1.8 (test.nxs)
#  /work/imat/scripts/tomopy-0.1.8
#   ***angs = np.radians(theta)
#     try with 0...180 degree projections!!!
#     phantoms from tomopy
#     line integral!!!
# TODO: fill in the MCP gap before median_filter
# add self-copy of script(s) into output reconstruction (with README file giving the command, paths, etc.)
# TODO: change to command pattern
# TODO: add save functionality

def apply_prep_filters(data, median_filter_size=3, rotate=None, crop=None, air_region=None,
                       norm_flat_img=None, norm_dark_img=None, cut_off=None):
    """
    @param crop :: crop to this rectangle (region of interest) - as [x1, y1, x2, y2]
    @param median_filter_size :: width/neighborhood of the median filter as an integer, 0 or none to disable
    """
    print " * Beginning pre-processing with pixel data type:", data.dtype
    if 'float64' == data.dtype:
        data = data.astype(dtype='float32')
        print " * Note: pixel data type changed to:", data.dtype

    too_verbose = False
    # quick test of a (wrong) transmission=>absorption conversion
    if False:
        for idx in range(0, data.shape[0]):
            dmax = np.amax(data[idx])

            data[idx, :, :] = dmax*dmax - data[idx, :, :]*data[idx, :, :]

    if air_region:
        air_sums = []
        for idx in range(0, data.shape[0]):
            air_data_sum = data[idx, air_region[1]:air_region[3], air_region[0]:air_region[2]].sum()
            air_sums.append(air_data_sum)
            #print "Shape of air data: ", air_data.shape
            #sum_air = air_data.sum()
            #print "Sum air, idx {0}: {1}".format(idx, sum_air)
        if too_verbose:
            print "air sums: ", air_sums
        avg = np.average(air_sums)
        if too_verbose:
            print("Air avg: {0}, max ratio: {1}, min ratio: {2}".format(
                avg, np.max(air_sums)/avg, np.min(air_sums)/avg))

    if norm_flat_img:
        norm_divide = None
        if norm_dark_img:
            norm_divide = norm_flat_img - norm_dark_img
        # prevent divide-by-zero issues
        norm_divide[norm_divide==0] = 1e-6

        for idx in range(0, data.shape[0]):
            data[idx, :, :] = np.true_divide(data[idx, :, :] - norm_dark_img, norm_divide)
        # true_divide produces float64, we assume that precision not needed (definitely not
        # for 16-bit depth images as we usually have.
        print " * Finished normalization by flat/dark images with pixel data type: {0}.".format(data.dtype)
    else:
        print " * Note: not applying normalization by flat/dark images."

    # Apply cut-off for the normalization?
    if False and cut_off:
        print "* Applying cut-off with level: {0}".format(cut_off)
        dmin = np.amin(data)
        dmax = np.amax(data)
        rel_cut_off = dmin + cut_off * (dmax - dmin)
        data[data < rel_cut_off] = dmin
        print " * Finished cut-off stepa, with pixel data type: {0}".format(data.dtype)
    else:
        print " * Note: not applying cut-off."

    # list with first-x, first-y, second-x, second-y
    if crop:
        if  isinstance(crop, list) and 4 == len(crop):
            data = data[:, crop[1]:crop[3], crop[0]:crop[2]]
        else:
            print "Error in crop parameter (expecting a list with four integers. Got: {0}.".format(crop)

        print " * Finished crop step, with pixel data type: {0}.".format(data.dtype)
    else:
        print " * Note: not applying crop."


    apply_median = True
    if apply_median:
        for idx in range(0, data.shape[0]):
            if median_filter_size:
                data[idx] = scipy.ndimage.median_filter(data[idx], median_filter_size, mode='mirror')#, mode='nearest')
                #data[idx] = tomopy.misc.corr.median_filter(data[idx], median_filter_size)

        print " * Finished noise filter / median, with pixel data type: {0}.".format(data.dtype)
    else:
        print " * Note: not applying noise filter /median."

    # rotate = None
    rotate = -1
    data_rotated = np.zeros((data.shape[0], data.shape[2], data.shape[1]), dtype=data.dtype)
    if rotate:
        for idx in range(0, data.shape[0]):
        # Rotation 90 degrees counterclockwise (or -90 degrees)
            data_rotated[idx] = np.rot90(data[idx,:,:], rotate)
        print " * Finished rotation step, with pixel data type: {0}".format(data_rotated.dtype)

    return data_rotated

def astra_reconstruct3d(sinogram, angles, shape, depth, alg_name, iterations):
    # Some of these have issues depending on the GPU setup
    algs_avail = "[FP3D_CUDA], [BP3D_CUDA]], [FDK_CUDA], [SIRT3D_CUDA], [CGLS3D_CUDA]"

    if not alg_name.upper() in algs_avail:
        raise ValueError("Invalid algorithm requested for the Astra package: {0}. "
                         "Supported algorithms: {1}".format(alg_name, algs_avail))
    det_rows = sinogram.shape[0]
    det_cols = sinogram.shape[2]

    vol_geom = astra.create_vol_geom(shape[0], depth, shape[1])
    proj_geom = astra.create_proj_geom('parallel3d', 1.0, 1.0, det_cols,
                                       det_rows, np.deg2rad(angles))

    sinogram_id = astra.data3d.create("-sino", proj_geom, sinogram)
    # Create a data object for the reconstruction
    rec_id = astra.data3d.create('-vol', vol_geom)

    cfg = astra.astra_dict(alg_name)
    cfg['ReconstructionDataId'] = rec_id
    cfg['ProjectionDataId'] = sinogram_id

    # Create the algorithm object from the configuration structure
    alg_id = astra.algorithm.create(cfg)
    # This will have a runtime in the order of 10 seconds.
    astra.algorithm.run(alg_id, iterations)
    #if "CUDA" in params[0] and "FBP" not in params[0]:
    #self.res += astra.algorithm.get_res_norm(alg_id)**2
    #print math.sqrt(self.res)

    # Get the result
    rec = astra.data3d.get(rec_id)

    astra.algorithm.delete(alg_id)
    astra.data3d.delete(rec_id)
    astra.data3d.delete(sinogram_id)

    rec = rec[:160, :160, 1]

    return rec

def get_max_frames(algorithm):
    frames = 8 if "3D" in algorithm else 1
    return frames

def run_reconstruct_3d_astra(proj_data, algorithm, cor, proj_angles):
    nSinos = get_max_frames(algorithm=algorithm)
    #lparams = self.get_parameters()
    alg_name = algorithm #lparams[0]
    iterations = 100 #lparams[1]
    print " astra recon - doing {0} iterations".format(iterations)

    # swaps outermost dimensions so it is sinogram layout
    sinogram = proj_data
    sinogram = np.swapaxes(sinogram, 0, 1)


    ctr = cor
    width = sinogram.shape[1]
    pad = 50

    sino = np.nan_to_num(1./sinogram)

    # pad the array so that the centre of rotation is in the middle
    alen = ctr
    blen = width - ctr
    mid = width / 2.0

    if ctr > mid:
        plow = pad
        phigh = (alen - blen) + pad
    else:
        plow = (blen - alen) + pad
        phigh = pad

    logdata = np.log(sino+1)

    sinogram = np.tile(sinogram.reshape((1,)+sinogram.shape),
                       (8, 1, 1))

    vol_shape = (proj_data.shape[1], sinogram.shape[2], proj_data.shape[1]) # ?
    rec = astra_reconstruct3d(sinogram, proj_angles, shape=vol_shape, depth=nSinos,
                              alg_name=alg_name, iterations=iterations)

    return rec

def run_reconstruct_3d_astra_simple(proj_data, algorithm, cor, proj_angles):
    sinograms = proj_data

    sinograms = np.swapaxes(sinograms, 0, 1)

    plow = (proj_data.shape[2] - cor*2)
    phigh = 0

    minval = np.amin(sinograms)
    sinograms = np.pad(sinograms, ((0,0),(0,0),(plow,phigh)), mode='reflect')

    proj_geom = astra.create_proj_geom('parallel3d', .0, 1.0, proj_data.shape[1],
                                       sinograms.shape[2], proj_angles)
    sinogram_id = astra.data3d.create('-sino', proj_geom, sinograms)

    vol_geom = astra.create_vol_geom(proj_data.shape[1], sinograms.shape[2], proj_data.shape[1])
    recon_id = astra.data3d.create('-vol', vol_geom)
    alg_cfg = astra.astra_dict('SIRT3D_CUDA')
    alg_cfg['ReconstructionDataId'] = recon_id
    alg_cfg['ProjectionDataId'] = sinogram_id
    alg_id = astra.algorithm.create(alg_cfg)

    number_of_iters=100
    astra.algorithm.run(alg_id, number_of_iters)
    recon = astra.data3d.get(recon_id)

    astra.algorithm.delete(alg_id)
    astra.data3d.delete(recon_id)
    astra.data3d.delete(sinogram_id)

    return recon

def run_reconstruct_3d(proj_data, tool, algorithm, cor=None, max_angle=360, num_iter=None):
    """
    A 3D reconstruction

    @param proj_data :: Input projected images
    @param tool :: reconstruction tool to call/use

    Returns :: reconstructed volume
    """
    num_proj = proj_data.shape[0]
    inc = float(max_angle)/(num_proj-1)

    proj_angles=np.arange(0, num_proj*inc, inc)
    # For tomopy
    proj_angles = np.radians(proj_angles)

    verbosity = 1
    if 'astra' == tool:
        #run_reconstruct_3d_astra(proj_data, algorithm, cor, proj_angles=proj_angles)
        return run_reconstruct_3d_astra_simple(proj_data, algorithm, cor, proj_angles=proj_angles)

    for slice_idx in [200]: # examples to check: [30, 130, 230, 330, 430]:
        print " > Finding center with tomopy find_center, slice_idx: {0}...".format(slice_idx)
        import tomorec.tool_imports as tti
        try:
            tomopy = tti.import_tomo_tool('tomopy')
            tomopy_cor = tomopy.find_center(tomo=proj_data, theta=proj_angles, ind=slice_idx, emission=False)
            if not cor:
                cor = tomopy_cor
            print " > Center of rotation found by tomopy.find_center:  {0}".format(tomopy_cor)
        except ImportError as exc:
            print(" * WARNING: could not import tomopy so could not use the tomopy method to find the center "
                  "of rotation. Details: {0}".format(exc))


    print "Using center of rotation: {0}".format(cor)
    start = time.time()
    if 'tomopy' == tool and 'gridrec' != algorithm and 'fbp' != algorithm:
        # TODO: turn this into a default
        if not num_iter:
            num_iter=40
        # bart with num_iter=20 => 467.640s ~= 7.8m
        # sirt with num_iter=30 => 698.119 ~= 11.63
        if verbosity >= 1:
            print("Running iterative method with tomopy. Algorithm: {0}, "
                  "number of iterations: {1}".format(algorithm, num_iter))
        rec = tomopy.recon(tomo=proj_data, theta=proj_angles, center=cor,
                           algorithm=algorithm, num_iter=num_iter) #, filter_name='parzen')
    else:
        if verbosity >= 1:
            print("Running non-iterative reconstruction algorithm with tomopy. "
                  "Algorithm: {0}".format(algorithm))
        rec = tomopy.recon(tomo=proj_data, theta=proj_angles, center=cor, algorithm=algorithm)
    tnow = time.time()
    print "Reconstructed 3D volume. Time elapsed in reconstruction algorithm: {0:.3f}".format(tnow - start)

    return rec

def _make_dirs_if_needed(dirname):
    """
    Makes sure that the directory needed to save the file exists, or creates it

    @param dirname :: (output) directory to check
    """
    absname = os.path.abspath(dirname)
    if not os.path.exists(absname):
        os.makedirs(absname)


def read_in_stack(sample_path, img_format):
    # Note, not giving prefix. It will load all the files found.
    # Example prefixes are prefix = 'tomo_', prefix = 'LARMOR00', prefix = 'angle_agg'

    sample, white, dark = tomoio.read_stack_of_images(sample_path, file_extension=img_format)

    if not isinstance(sample, np.ndarray) or not sample.shape \
       or not isinstance(sample.shape, tuple) or 3 != len(sample.shape):
        raise RuntimeError("Error reading sample images. Could not produce a 3-dimensional array "
                           "of data from the sample images. Got: {0}".format(sample))

    return (sample, white, dark)

def apply_line_projection(data_vol):
    """
    """
    # Line integral, yeah!  exp(integral) => - ln (Is/I0)
    # produce projection image, $ p(s) = -ln\frac{I(s)}{I(0)} $,
    # with $ I(s) = I(0) e^{-\int_0^s \mu(x)dx} $
    # where $p(s)$ represents the sum of the density of objects along a line (pixel) of the beam
    # I(0) initital intensity of netron beam (white images)
    # I(s) neutron count measured by detector/camera
    # the integral is the density along the path through objects
    data_vol = data_vol.astype('float32')
    for idx in range(0, data_vol.shape[0]):
        max_img = np.amax(data_vol[idx, :, :])
        to_log = np.true_divide(data_vol[idx, :, :], max_img)
        if False:
            print("initial img max: {0}. transformed to log scale,  min: {1}, max: {2}".
                  format(max_img, np.amin(to_log), np.amax(to_log)))
        data_vol[idx, :, :] = - np.log(to_log) # +0.001

    return data_vol

def apply_final_preproc_corrections(preproc_data, remove_stripes='wavelet-fourier'):
    """
    """
    # Remove stripes in sinograms / ring artefacts in reconstructed volume
    if remove_stripes:
        import prep as iprep
        if 'wavelet-fourier' == remove_stripes.lower():
            time1 = time.time()
            print " * Removing stripes/ring artifacts using the method '{0}'".format(remove_stripes)
            #preproc_data = tomopy.prep.stripe.remove_stripe_fw(preproc_data)
            preproc_data = iprep.filters.remove_stripes_ring_artifacts(preproc_data, 'wavelet-fourier')
            time2 = time.time()
            print " * Removed stripes/ring artifacts. Time elapsed: {0:.3f}".format(time2 - time1)
        elif 'titarenko' == remove_stripes.lower():
            time1 = time.time()
            print " * Removing stripes/ring artifacts, using the method '{0}'".format(remove_stripes)
            preproc_data = tomopy.prep.stripe.remove_stripe_ti(preproc_data)
            time2 = time.time()
            print " * Removed stripes/ring artifacts, Time elapsed: {0:.3f}".format(time2 - time1)
        else:
            print " * WARNING: stripe removal method '{0}' is unknown. Not applying it.".format(remove_stripes)
    else:
        print " * Note: not applying stripe removal."

    if False:
        # preproc_data = tomopy.misc.corr.remove_outlier(preproc_data ...
        preproc_data = tomopy.misc.corr.adjust_range(preproc_data)

    if False:
        preproc_data = tomopy.prep.normalize.normalize_bg(preproc_data, air=5)

    return preproc_data

def save_preproc_images(output_dir, preproc_data,
                        subdir_name='preproc_images', out_dtype='uint16'):
    """
    """
    print " * Pre-processed images (preproc_data) dtype:", preproc_data.dtype
    min_pix = np.amin(preproc_data)
    max_pix = np.amax(preproc_data)
    print "   with min_pix: {0}, max_pix: {1}".format(min_pix, max_pix)
    if True:
        preproc_dir = os.path.join(output_dir, subdir_name)
        print "* Saving pre-processed images into: {0}".format(preproc_dir)
        _make_dirs_if_needed(preproc_dir)
        for idx in range(0, preproc_data.shape[0]):
            # rescale_intensity has issues with float64=>int16
            tomoio._write_image(preproc_data[idx, :, :], min_pix, max_pix,
                                os.path.join(preproc_dir, 'out_preproc_proj_image' + str(idx).zfill(6)),
                                dtype=out_dtype)#, rescale_intensity=False)
    else:
        print "* NOTE: not saving pre-processed images..."

def apply_postproc_filters(recon_data, cut_off=0.0, circular_mask=True, median_filter_size=None):
    """
    @param cut_off
    @param circular_mask :: ratio as a floating point number (for example 0.94 or 0.95)
    """
    if cut_off:
        print "=== applying cut-off: {0}".format(cut_off)
        dmin = np.amin(recon_data)
        dmax = np.amax(recon_data)
        rel_cut_off = dmin + cut_off * (dmax - dmin)
        recon_data[recon_data < rel_cut_off] = dmin

    import prep as iprep

    if circular_mask:
        recon_data = iprep.filters.circular_mask(recon_data, ratio=circular_mask)
        print " * Applied circular mask on reconstructed volume"
    else:
        print " * Note: not applied circular mask on reconstructed volume"

    if False and median_filter_size:
        recon_data = scipy.ndimage.median_filter(recon_data, median_filter_size)
        print (" * Applied median_filter on reconstructed volume, with filtersize: {0}".
               format(median_filter_size))
    else:
        print " * Note: not applied median_filter on reconstructed volume"

    if False:
        import scipy.signal
        kernel_size=3
        # Note this can be extremely slow
        recon_data = scipy.signal.medfilt(recon_data, kernel_size=kernel_size)
        print(" * Applied N-dimensional median filter on reconstructed volume, with filter size: {0} ".
              format(kernel_size))
    else:
        print " * Note: not applied N-dimensional median filter on reconstructed volume"


# cor for metals: 123
def do_recon(input_dir, output_dir, cor, command_line=None, tool=None, algorithm=None, max_angle=360,
             remove_stripes='wavelet-fourier',
             rotate=None, crop_coords=None,
             circular_mask=None, cut_off=None, num_iter=None,
             img_format='fits',
             cmd_line=None):
    """
    Run a reconstruction using a particular tool, algorithm and setup

    @param input_dir ::

    @param output_dir ::

    @param cor :: center of rotation, in pixel coordinates (x axis), generally assuming that
    the sample rotates around the y axis.

    @param remove_stripes :: sinogram stripe removal method (for ring artifacts)

    @param cmd_line :: command line text if running from the CLI. When provided it will
    be written in the output readme file(s) for reference.
    """

    data, white, dark = read_in_stack(input_dir, img_format)
    print "Shape of raw data: {0}, dtype: {1}".format(data.shape, data.dtype)

    raw_data_dtype = data.dtype
    raw_data_shape = data.shape

    air_region  = [260, 5, 502, 250]
    rotation = -1

    # For Cannon. tompoy CoR: 470.8 / oct: 504
    #air_region = None

    # These imports will raise appropriate exceptions in case of error
    import tomorec.tool_imports as tti
    if 'astra' == tool:
        astra = tti.import_tomo_tool('astra')
    elif 'tomopy' == tool:
        tomopy = tti.import_tomo_tool('tomopy')

    median_filter_size = 3

    preproc_data = apply_prep_filters(data, rotate=-1,
                                      crop=crop_coords,
                                      air_region=air_region, cut_off=cut_off) # TODO: pass median_filter_size
    #preproc_data = data
    #data = apply_prep_filters(data, rotate=-1, crop=[0, 256, 512, 512])
    print "Shape of preproc data: {0}".format(preproc_data.shape)


    preproc_data = apply_line_projection(preproc_data)

    preproc_data = apply_final_preproc_corrections(preproc_data, remove_stripes)

    #import matplotlib.pyplot as pl
    #pl.imshow(preproc_data[66], cmap='gray')
    #pl.show()

    # Save pre-proc images
    save_preproc_images(output_dir, preproc_data)

    # gridrec fbp sirt, bart, pml_hybrid
    #recon_data = run_reconstruct_3d(proj_data=preproc_data, tool='astra', algorithm='gridrec', cor=cor)# TODO cor=122.667)
    if None == algorithm:
        algorithm = 'gridrec' #fbp, gridrec


    if False:
        preproc_data = preproc_data[0:71, :, :]
        max_angle = 180


    tstart = time.time()
    recon_data = run_reconstruct_3d(proj_data=preproc_data, tool='tomopy', algorithm=algorithm,
                                    cor=cor, max_angle=max_angle, num_iter=num_iter)# TODO cor=122.667)

    tnow = time.time()

    print("Reconstructed volume. Shape: {0}, and pixel data type: {1}".
          format(recon_data.shape, recon_data.dtype))

    apply_postproc_filters(recon_data, cut_off, circular_mask=circular_mask,
                           median_filter_size=median_filter_size)

    # Save

    # slices along the vertical (z) axis
    # output_dir = 'output_recon_tomopy'
    print "* Saving slices of the reconstructed volume in: {0}".format(output_dir)
    _make_dirs_if_needed(output_dir)
    #tomopy.io.writer.write_tiff_stack(recon_data)
    min_pix = np.amin(recon_data)
    max_pix = np.amax(recon_data)
    for idx in range(0, recon_data.shape[0]):
        tomoio._write_image(recon_data[idx, :, :], min_pix, max_pix,
                            os.path.join(output_dir, 'out_recon_slice' + str(idx).zfill(6)),
                            dtype='uint16')

        #for idx in range(0, recon_data.shape[1]):
        #    _write_image(recon_data[:, idx, :], os.path.join(output_dir, 'out_recon_slice' + str(idx).zfill(6)), dtype='int16')

        #for idx in range(0, recon_data.shape[2]):
        #    _write_image(recon_data[:, :, idx], os.path.join(output_dir, 'out_recon_slice' + str(idx).zfill(6)), dtype='int16')

    # Sideways slices:
    out_horiz_dir = os.path.join(output_dir, 'horiz_slices')
    print "* Saving horizontal slices in: {0}".format(out_horiz_dir)
    _make_dirs_if_needed(out_horiz_dir)
    for idx in range(0, recon_data.shape[1]):
        tomoio._write_image(recon_data[:, idx, :], min_pix, max_pix,
                            os.path.join(out_horiz_dir, 'out_recon_horiz_slice' + str(idx).zfill(6)),
                            dtype='uint16')

    if False:
        NXdata = recon_data
        xsize = NXdata.shape[0]
        ysize = NXdata.shape[1]
        zsize = NXdata.shape[2]

        print " net cdf shape: {0}".format(NXdata.shape)
        nc_path = 'recon_nedcdf.nc'
        ncfile = netcdf_file(nc_path, 'w')
        ncfile.createDimension('x', xsize)
        ncfile.createDimension('y', ysize)
        ncfile.createDimension('z', zsize)
        print "Creating netCDF var"
        data = ncfile.createVariable('data', np.dtype('int16').char, ('x','y','z'))
        print "data.shape: ", data.shape
        print "Assigning data..."
        data[:,:,:] = NXdata[0:xsize,0:ysize,0:zsize]
        print "Closing nc file... ", nc_path
        ncfile.close()


    #run_summary_string = gen_txt_run_summary()
    out_readme_fname = '0.README_reconstruction.txt'
    # generate file with dos/windows line end for windoze users' convenience
    with open(os.path.join(output_dir, out_readme_fname), 'w') as oreadme:
        if None == tool:
            tool = 'tomopy'
        #oreadme.write(run_summary_string)
        oreadme.write('Tomographic reconstruction. Summary of inputs, settings and outputs.\r\n')
        oreadme.write(time.strftime("%c") + "\r\n")
        oreadme.write("\r\n")
        oreadme.write("Dimensions of raw input sample data: {0}\r\n".format(raw_data_shape))
        oreadme.write("Dimensions of pre-processed sample data: {0}\r\n".format(preproc_data.shape))
        oreadme.write("Dimensions of reconstructed volume: {0}\r\n".format(recon_data.shape))
        oreadme.write("Max. angle: {0}\r\n".format(max_angle))
        oreadme.write("Tool: {0}\r\n".format(tool))
        oreadme.write("Algorithm: {0}\r\n".format(algorithm))
        if num_iter:
            oreadme.write("Number of algorith iterations: {0}\r\n".format(num_iter))
        else:
            oreadme.write("(No algorithm iterations parameter)\r\n")
        oreadme.write("Raw input pixel type: {0}\r\n".format(raw_data_dtype))
        oreadme.write("Output pixel type: {0}\r\n".format('uint16'))
        oreadme.write("Reconstruction time: {0:.3f}s\r\n".format(tnow-tstart))
        oreadme.write("\r\n")
        oreadme.write("--------------------------\r\n")
        oreadme.write("Pre-processing parameters\r\n")
        oreadme.write("--------------------------\r\n")
        oreadme.write("Center of rotation: {0}\r\n".format(cor))
        oreadme.write("Normalize flat: {0}\r\n".format(1))
        oreadme.write("Normalize dark: {0}\r\n".format(1))
        oreadme.write("Normalize proton charge: {0}\r\n".format(0))
        oreadme.write("Line integral: {0}\r\n".format(1))
        oreadme.write("Median filter width: {0}\r\n".format(median_filter_size))
        oreadme.write("Rotation: {0}\r\n".format(rotation))
        oreadme.write("Crop coordinates: {0}\r\n".format(crop_coords))
        oreadme.write("Sinogram stripes removal: {0}\r\n".format('fw method'))
        oreadme.write("\r\n")
        oreadme.write("--------------------------\r\n")
        oreadme.write("Post-processing parameters\r\n")
        oreadme.write("--------------------------\r\n")
        oreadme.write("Gaussian filter: {0}\r\n".format(0))
        oreadme.write("Cut-off on reconstructed volume: {0}\r\n".format(cut_off))
        oreadme.write("Circular mask: {0}\r\n".format(1))
        oreadme.write("\r\n\r\n")
        oreadme.write("Output written into: {0}\r\n".format(os.path.abspath(output_dir)))
        oreadme.write("Write pre-proc: {0}\r\n".format(1))
        oreadme.write("\r\n\r\n")
        oreadme.write(cmd_line)

# To save this script (and dependencies) into the output reconstructions
def we_are_frozen():
    # All of the modules are built-in to the interpreter, e.g., by py2exe
    return hasattr(sys, "frozen")

def module_path():
    encoding = sys.getfilesystemencoding()
    if we_are_frozen():
        return os.path.dirname(unicode(sys.executable, encoding))
    else:
        return os.path.dirname(unicode(__file__, encoding))

def self_save_zipped_scripts(output_path):

    def _zipdir(path, ziph):
        # ziph is zipfile handle
        for root, dirs, files in os.walk(path):
            for file in files:
                ziph.write(os.path.join(root, file))

    import inspect
    this_path = os.path.abspath(inspect.getsourcefile(lambda:0))

    scripts_path = os.path.dirname(this_path)

    _make_dirs_if_needed(output_path)
    print ("Saving myself (reconstruction scripts) from: {0} in: {1}".
           format(scripts_path, os.path.abspath(output_path)))
    import zipfile
    # os.path.join(output_path, ... )
    RECON_SCRIPTS_PKG_NAME = '0.reconstruction_scripts.zip'
    with zipfile.ZipFile(os.path.join(output_path, RECON_SCRIPTS_PKG_NAME), 'w', zipfile.ZIP_DEFLATED) as zip_scripts:
        # To write just this file: zipscr.write(this_path)
        _zipdir(scripts_path, zip_scripts)

if __name__=='__main__':
    import argparse
    import ast

    arg_parser = argparse.ArgumentParser(description='Run tomographic reconstruction via third party tools')

    grp_req = arg_parser.add_argument_group('Mandatory/required options')

    grp_req.add_argument("-i","--input-path", required=True, help="Input directory")

    grp_req.add_argument("-o","--output-path", required=True, help="Where to write the output slice images"
                         "(reconstructred volume)")

    grp_req.add_argument("-c","--cor", required=True, help="Center of rotation (in pixels). rotation around y "
                         "axis is assumed")

    grp_recon = arg_parser.add_argument_group('Reconstruction options')

    grp_recon.add_argument("-t","--tool", required=False, help="Tomographic reconstruction tool to use")

    grp_recon.add_argument("-a","--algorithm", required=False, help="Reconstruction algorithm (tool dependent)")

    grp_recon.add_argument("-n","--num-iter", required=False, help="Number of iterations (only valid for "
                           "iterative methods (example: SIRT, ART, etc.).")


    grp_recon.add_argument("--max-angle", required=False, help="Maximum angle (of the last projection), "
                           "assuming first angle=0, and uniform angle increment for every projection (note: this "
                           "is overriden by the angles found in the input FITS headers)")

    grp_pre = arg_parser.add_argument_group('Pre-processing of input raw images/projections')

    grp_pre.add_argument("--in-img-format", required=False, default='fits',
                         help="Format/file extension expected for the input images. Supported: {0}".
                         format(['tiff', 'fits', 'tif', 'fit', 'png']))

    grp_pre.add_argument("--region-of-interest", required=False, help="Region of interest (crop original "
                         "images to these coordinates, given as comma separated values: x1,y1,x2,y2. If not "
                         "given, the whole images are used.")

    grp_pre.add_argument("--air-region", required=False, help="Air region /region for normalization. "
                         "If not provided, the normalization against beam intensity fluctuations will not be "
                         "performed")

    grp_pre.add_argument("--median-filter-size", required=False, help="Size/width of the median filter "
                         "(pre-processing")

    grp_pre.add_argument("--remove-stripes", default='wf', required=False,
                         help="Methods supported: 'wf' (Wavelet-Fourier)")

    grp_pre.add_argument("--rotation", required=False, help="Rotate images by 90 degrees a number of "
                         "times. The rotation is clockwise unless a negative number is given which indicates "
                         "rotation counterclocwise")

    grp_pre.add_argument("--scale-down", required=False, help="Scale down factor, to reduce the size of "
                         "the images for faster (lower-resolution) reconstruction. For example a factor of 2 "
                         "reduces 1kx1k images to 512x512 images (combining blocks of 2x2 pixels into a single "
                         "pixel. The output pixels are calculated as the average of the input pixel blocks.")

    grp_post = arg_parser.add_argument_group('Post-processing of the reconstructed volume')

    grp_post.add_argument("--circular-mask", required=False, default=0.94,
                          help="Radius of the circular mask to apply on the reconstructed volume. "
                          "It is given in [0,1] relative to the size of the smaller dimension/edge "
                          "of the slices. Empty or zero implies no masking.")

    grp_post.add_argument("--cut-off", required=False, help="Cut off level (percentage) for reconstructed "
                          "volume. pixels below this percentage with respect to maximum intensity in the stack "
                          "will be set to the minimum value.")

    arg_parser.add_argument("-v", "--verbose", action="count", default=1, help="Verbosity level. Default: 1. "
                            "User zero to supress outputs.")

    args = arg_parser.parse_args()

    # '/home/fedemp/tomography-tests/stack_larmor_metals_summed_all_bands/data_metals/'
    if not args.cor.isdigit():
        raise RuntimeError("The center of rotation must be an integer")

    self_save_zipped_scripts(args.output_path)

    # TODO: check cut_off is a valid float, and not 0, not 1, etc.!
    cut_off = None
    if args.cut_off:
        cut_off = args.cut_off

    # TODO: is a valid int, and not <=0, etc.!
    num_iter = None
    if args.num_iter:
        num_iter = int(args.num_iter)

    cmd_line = " ".join(sys.argv)

    if args.max_angle:
        max_angle = float(args.max_angle)

    circular_mask = None
    if args.circular_mask:
        circular_mask = float(args.circular_mask)

    roi = None
    if args.region_of_interest:
        roi = ast.literal_eval(args.region_of_interest)
    else:
        border_pix = 5
        roi = [0+border_pix, 252, 512-border_pix, 512-border_pix]

    remove_stripes = None
    if 'wf' == args.remove_stripes:
        remove_stripes = 'wavelet-fourier'

    do_recon(input_dir=args.input_path, output_dir=args.output_path, cor=int(args.cor),
             tool=args.tool, algorithm=args.algorithm,
             max_angle=max_angle,rotate=args.rotation, crop_coords=roi,
             remove_stripes=remove_stripes,
             circular_mask=circular_mask, cut_off=cut_off, num_iter=num_iter, img_format=args.in_img_format,
             cmd_line=cmd_line)
