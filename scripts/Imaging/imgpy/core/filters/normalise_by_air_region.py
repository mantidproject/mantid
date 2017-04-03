from __future__ import (absolute_import, division, print_function)
import numpy as np
import helper as h
from core.parallel import two_shared_mem as ptsm
from core.parallel import utility as pu


def cli_register(parser):
    parser.add_argument(
        "-A",
        "--air-region",
        required=False,
        nargs='*',
        type=str,
        help="Air region /region for normalisation. The selection is a rectangle and expected order is - Left Top Right Bottom.\n"
        "For best results the region selected should not be blocked by any object in the Tomography.\n"
        "Example: --air-region 150 234 23 22")

    return parser


def gui_register(par):
    raise NotImplementedError("GUI doesn't exist yet")


def execute(data, air_region, cores=None, chunksize=None):
    """
    Normalise by beam intensity. This is not directly about proton
    charge - not using the proton charge field as usually found in
    experiment/nexus files. This uses an area of normalization, if
    provided in the pre-processing configuration.
    
    This does NOT do any checks if the Air Region is out of bounds!
    If the Air Region is out of bounds, the crop will fail at runtime.
    If the Air Region is in bounds, but has overlapping coordinates 
    the crop give back a 0 shape of the coordinates that were wrong.

    :param data: stack of images as a 3d numpy array
    :param air_region: The air region from which sums will be calculated and all images will be normalised. 
                       The selection is a rectangle and expected order is - Left Top Right Bottom.
    :param cores: The number of cores that will be used to process the data.
    :param chunksize: The number of chunks that each worker will receive.

    :returns: filtered data (stack of images)

    Example command line:
    python main.py -i /some/data -A 134 203 170 250
    python main.py -i /some/data --air-region 134 203 170 250
    """
    h.check_data_stack(data)

    if air_region:
        if pu.multiprocessing_available():
            data = _execute_par(data, air_region, cores, chunksize)
        else:
            data = _execute_seq(data, air_region)

    h.check_data_stack(data)
    return data


def _calc_sum(data,
              air_sums,
              air_left=None,
              air_top=None,
              air_right=None,
              air_bottom=None):
    # here we can use ndarray.sum or ndarray.mean
    # ndarray.mean makes the values with a nice int16 range 
    # (0-65535, BUT NOT int16 TYPE! They remain floats!)
    # while ndarray.sum makes them in a low range of 0-1.5.
    # There are little differences in the results both visually and in the histograms
    return data[air_top:air_bottom, air_left:air_right].mean()


def _divide_by_air_sum(data=None, air_sums=None):
    data[:] = np.true_divide(data, air_sums)


def _execute_par(data, air_region, cores=None, chunksize=None):
    left = air_region[0]
    top = air_region[1]
    right = air_region[2]
    bottom = air_region[3]
    h.pstart("Starting normalization by air region...")

    # initialise same number of air sums
    img_num = data.shape[0]
    air_sums = pu.create_shared_array((img_num, 1, 1))

    # turn into a 1D array, from the 3D that is returned
    air_sums = air_sums.reshape(img_num)

    calc_sums_partial = ptsm.create_partial(
        _calc_sum,
        fwd_function=ptsm.fwd_func_return_to_second,
        air_left=left,
        air_top=top,
        air_right=right,
        air_bottom=bottom)

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


def _execute_seq(data, air_region):
    left = air_region[0]
    top = air_region[1]
    right = air_region[2]
    bottom = air_region[3]

    h.pstart("Starting normalization by air region...")
    h.prog_init(data.shape[0], "Calculating air sums")
    air_sums = []
    for idx in range(0, data.shape[0]):
        air_data_sum = data[idx, top:bottom, left:right].sum()
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
