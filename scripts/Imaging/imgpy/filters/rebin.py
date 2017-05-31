from __future__ import (absolute_import, division, print_function)
import helper as h


def cli_register(parser):
    parser.add_argument(
        "--rebin",
        required=False,
        type=float,
        help="Rebin factor by which the images will be rebinned. This could be any positive float number.\n"
        "If not specified no scaling will be done.")

    parser.add_argument(
        "--rebin-mode",
        required=False,
        type=str,
        default=modes()[0],
        choices=modes(),
        help="Default: %(default)s\n"
        "Specify which interpolation mode will be used for the scaling of the image."
    )

    return parser


def gui_register(par):
    raise NotImplementedError("GUI doesn't exist yet")


def modes():
    return ['nearest', 'lanczos', 'bilinear', 'bicubic', 'cubic']


def execute(data, rebin_param, mode, cores=None, chunksize=None):
    """
    Execute the Rebin/imresize filter.

    :param data: The sample image data as a 3D numpy.ndarray
    :param rebin_param: int, float or tuple
                        int - Percentage of current size.
                        float - Fraction of current size.
                        tuple - Size of the output image.

    :param mode: Interpolation to use for re-sizing ('nearest', 'lanczos', 'bilinear', 'bicubic' or 'cubic').

    :return: the data after being processed with the filter

    Full reference:
    https://docs.scipy.org/doc/scipy-0.18.1/reference/generated/scipy.misc.imresize.html

    """
    h.check_data_stack(data)

    if rebin_param and 0 < rebin_param:
        from parallel import utility as pu
        if pu.multiprocessing_available():
            data = _execute_par(data, rebin_param, mode, cores, chunksize)
        else:
            data = _execute_seq(data, rebin_param, mode)

    h.check_data_stack(data)
    return data


def _execute_par(data, rebin_param, mode, cores=None, chunksize=None):
    import scipy.misc
    from parallel import exclusive_mem as pem

    resized_data = _create_reshaped_array(data.shape, rebin_param)

    h.pstart("Starting PARALLEL image rebinning.")

    f = pem.create_partial(scipy.misc.imresize, size=rebin_param, interp=mode)

    resized_data = pem.execute(
        data, f, cores, chunksize, "Rebinning", output_data=resized_data)

    h.pstop("Finished PARALLEL image rebinning. New shape: {0}".format(
        resized_data.shape))

    return resized_data


def _execute_seq(data, rebin_param, mode):
    import scipy.misc
    h.pstart("Starting image rebinning.")
    resized_data = _create_reshaped_array(data.shape, rebin_param)
    num_images = resized_data.shape[0]
    h.prog_init(num_images, "Rebinning")
    for idx in range(num_images):
        resized_data[idx] = scipy.misc.imresize(
            data[idx], rebin_param, interp=mode)

        h.prog_update()

    h.prog_close()

    h.pstop(
        "Finished image rebinning. New shape: {0}".format(resized_data.shape))

    return resized_data


def _create_reshaped_array(old_shape, rebin_param):
    from parallel import utility as pu

    num_images = old_shape[0]

    # use SciPy's calculation to find the expected dimensions
    # int to avoid visible deprecation warning
    expected_dimy = int(rebin_param * old_shape[1])
    expected_dimx = int(rebin_param * old_shape[2])

    # allocate memory for images with new dimensions
    return pu.create_shared_array((num_images, expected_dimy, expected_dimx))


def _execute_custom(data, config):
    """
    Downscale to for example shrink 1Kx1K images to 512x512

    @param data_vol :: 3d volume to downscale
    @param block_size :: make block_size X block_size blocks to downscale
    @param method :: either 'average' (default) or 'sum' to calculate average or sum of blocks

    Returns :: downscaled volume, with the dimensions implied by block_size, and the
    same data type as the input data volume.
    """
    raise NotImplementedError("This function's implementation is disabled.")

    if 0 != np.mod(data_vol.shape[1], block_size) or 0 != np.mod(
            data_vol.shape[2], block_size):
        raise ValueError(
            "The block size ({0}) must be an exact integer divisor of the sizes of the "
            "x and y dimensions ({1} and {2} of the input data volume".format(
                data_vol.shape[2], data_vol.shape[1], block_size))

    supported_methods = ['average', 'sum']
    if method.lower() not in supported_methods:
        raise ValueError(
            "The method to combine pixels in blocks must be one of {0}. Got unknown "
            "value: {1}".format(supported_methods, method))

    rescaled_vol = np.zeros(
        (data_vol.shape[0], data_vol.shape[1] // block_size,
         data_vol.shape[2] // block_size),
        dtype=data_vol.dtype)
    # for block averages in every slice/image along the vertical/z axis
    tmp_shape = rescaled_vol.shape[1], block_size, rescaled_vol.shape[
        2], block_size
    for vert_slice in range(len(rescaled_vol)):
        vsl = data_vol[vert_slice, :, :]
        if 'average' == method:
            rescaled_vol[vert_slice, :, :] = vsl.reshape(tmp_shape).mean(
                -1).mean(1)
        elif 'sum' == method:
            rescaled_vol[vert_slice, :, :] = vsl.reshape(tmp_shape).mean(
                -1).mean(1)

    return rescaled_vol
