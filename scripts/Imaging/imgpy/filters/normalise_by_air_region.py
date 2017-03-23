from __future__ import (absolute_import, division, print_function)
from filters.crop_coords import _crop_coords_sanity_checks
import numpy as np
import helper as h


def cli_register(parser):
    parser.add_argument(
        "-A",
        "--air-region",
        required=False,
        nargs='*',
        type=str,
        help="Air region /region for normalisation.\n"
        "For best results it should avoid being blocked by any object.\n"
        "Example: --air-region='[150,234,23,22]'")

    return parser


def gui_register(par):
    raise NotImplementedError("GUI doesn't exist yet")


def execute(data,
            air_region,
            region_of_interest,
            crop_before_normalise,
            cores=None,
            chunksize=None):
    h.check_data_stack(data)

    if air_region is not None:
        _crop_coords_sanity_checks(air_region, data)
        if region_of_interest is not None:
            _crop_coords_sanity_checks(region_of_interest, data)

        from parallel import utility as pu
        if pu.multiprocessing_available():
            data = _execute_par(data, air_region, region_of_interest,
                                crop_before_normalise, cores, chunksize)
        else:
            data = _execute_seq(data, air_region, region_of_interest,
                                crop_before_normalise)

    h.check_data_stack(data)
    return data


def _calc_sum(data,
              roi_sums,
              roi_left=None,
              roi_top=None,
              roi_right=None,
              roi_bottom=None):
    # here we can use sum or mean, but mean makes the values with a nice int16 range
    # while sum makes them in a low range of 0-1.5
    return data[roi_top:roi_bottom, roi_left:roi_right].mean()


def _divide_by_air_sum(data=None, air_sums=None):
    data[:] = np.true_divide(data, air_sums)


def _execute_par(data,
                 air_region,
                 region_of_interest,
                 crop_before_normalise,
                 cores=None,
                 chunksize=None):
    """
    normalise by beam intensity. This is not directly about proton
    charg - not using the proton charge field as usually found in
    experiment/nexus files. This uses an area of normalization, if
    provided in the pre-processing configuration. TODO: much
    of this method should be moved into filters.

    :param data :: stack of images as a 3d numpy array
    :param region_of_interest: Region of interest to ensure that the Air Region is in bounds
    :param crop_before_normalise: A switch to signify that the image has been cropped.
            This means the Air Region coordinates will have to be translated onto the cropped image
    :param air_region: The air region from which sums will be calculated and all images will be normalised

    :returns :: filtered data (stack of images)

    """
    air_right, air_top, air_left, air_bottom = translate_coords_onto_cropped_picture(
        region_of_interest, air_region, crop_before_normalise)

    h.pstart("Starting normalization by air region...")

    # initialise same number of air sums
    from parallel import two_shared_mem as ptsm
    from parallel import utility as pu

    img_num = data.shape[0]
    air_sums = pu.create_shared_array((img_num, 1, 1))

    # turn into a 1D array, from the 3D that is returned
    air_sums = air_sums.reshape(img_num)

    calc_sums_partial = ptsm.create_partial(
        _calc_sum,
        fwd_function=ptsm.fwd_func_return_to_second,
        roi_left=air_left,
        roi_top=air_top,
        roi_right=air_right,
        roi_bottom=air_bottom)

    data, air_sums = ptsm.execute(data, air_sums, calc_sums_partial, cores,
                                  chunksize, "Calculating air sums")

    air_sums_partial = ptsm.create_partial(
        _divide_by_air_sum, fwd_function=ptsm.inplace_fwd_func)

    data, air_sums = ptsm.execute(data, air_sums, air_sums_partial, cores,
                                  chunksize, "Norm by Air Sums")

    avg = np.average(air_sums)
    max_avg = np.max(air_sums) / avg
    min_avg = np.min(air_sums) / avg

    h.pstop(
        "Finished normalization by air region. Average: {0}, max ratio: {1}, min ratio: {2}.".
        format(avg, max_avg, min_avg))

    return data


def _execute_seq(data, air_region, region_of_interest, crop_before_normalise):
    """
    Normalise by beam intensity. This is not directly about proton
    charge - not using the proton charge field as usually found in
    experiment/nexus files. This uses an area of normalization, if
    provided in the pre-processing configuration.

    :param data :: stack of images as a 3d numpy array
    :param region_of_interest: Region of interest to ensure that the Air Region is in bounds
    :param crop_before_normalise: A switch to signify that the image has been cropped.
            This means the Air Region coordinates will have to be translated onto the cropped image
    :param air_region: The air region from which sums will be calculated and all images will be normalised

    :returns :: filtered data (stack of images)

    """
    import numpy as np

    air_right, air_top, air_left, air_bottom = translate_coords_onto_cropped_picture(
        region_of_interest, air_region, crop_before_normalise)

    h.pstart("Starting normalization by air region...")
    h.prog_init(data.shape[0], "Calculating air sums")
    air_sums = []
    for idx in range(0, data.shape[0]):
        air_data_sum = data[idx, air_top:air_bottom, air_left:air_right].sum()
        air_sums.append(air_data_sum)
        h.prog_update()

    h.prog_close()

    h.prog_init(data.shape[0], desc="Norm by Air Sums")
    air_sums = np.true_divide(air_sums, np.amax(air_sums))
    for idx in range(0, data.shape[0]):
        data[idx, :, :] = np.true_divide(data[idx, :, :], air_sums[idx])
        h.prog_update()

    h.prog_close()

    avg = np.average(air_sums)
    max_avg = np.max(air_sums) / avg
    min_avg = np.min(air_sums) / avg

    h.pstop(
        "Finished normalization by air region. Average: {0}, max ratio: {1}, min ratio: {2}.".
        format(avg, max_avg, min_avg))

    return data


def translate_coords_onto_cropped_picture(crop_coords, air_region,
                                          crop_before_normalise):
    air_right = air_region[2]
    air_top = air_region[1]
    air_left = air_region[0]
    air_bottom = air_region[3]

    if not crop_before_normalise or crop_coords is None:
        return air_right, air_top, air_left, air_bottom

    crop_right = crop_coords[2]
    crop_top = crop_coords[1]
    crop_left = crop_coords[0]
    crop_bottom = crop_coords[3]

    _check_air_region_in_bounds(air_bottom, air_left, air_right, air_top,
                                crop_bottom, crop_left, crop_right, crop_top)
    # Translate the air region coordinates to the crop.
    air_right -= crop_left
    air_top -= crop_top
    air_left -= crop_left
    air_bottom -= crop_top

    _check_air_region_in_bounds(air_bottom, air_left, air_right, air_top,
                                crop_bottom, crop_left, crop_right, crop_top)

    return air_right, air_top, air_left, air_bottom


def _check_air_region_in_bounds(air_bottom, air_left, air_right, air_top,
                                crop_bottom, crop_left, crop_right, crop_top):
    # sanity check just in case
    if air_top < crop_top or \
            air_bottom > crop_bottom or \
            air_left < crop_left or \
            air_right > crop_right:
        raise ValueError(
            "Selected air region is outside of the cropped data range.")
