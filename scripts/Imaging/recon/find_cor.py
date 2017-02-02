from __future__ import (absolute_import, division, print_function)
import numpy as np


def execute(config):
    from recon.helper import Helper
    h = Helper(config)
    h.check_config_integrity(config)

    # -----------------------------------------------------------------------------

    # tomopy is the only supported tool for now
    from recon.runner import load_tool
    tool = load_tool(config, h)

    from recon.data import loader
    sample, flat, dark = loader.load_data(config, h)  # and onwards we go!

    # -----------------------------------------------------------------------------

    # import all used filters
    from recon.filters import rotate_stack, crop_coords
    cores = config.func.cores
    chunksize = config.func.chunksize

    sample, flat, dark = rotate_stack.execute(
        sample, config.pre.rotation, flat, dark, cores=cores, chunksize=chunksize, h=h)

    sample = crop_coords.execute_volume(
        sample, config.pre.region_of_interest, h)

    num_projections = sample.shape[0]
    projection_angle_increment = float(config.func.max_angle) / num_projections

    h.tomo_print_note("Calculating projection angles", 3)
    projection_angles = np.arange(
        0, num_projections * projection_angle_increment, projection_angle_increment)

    # For tomopy
    h.tomo_print_note("Calculating radians for TomoPy", 3)
    projection_angles = np.radians(projection_angles)

    size = int(num_projections)

    # depending on the number of COR projections it will select different
    # slice indices
    checked_projections = config.func.num_iter
    slice_indices = []
    current_slice_index = 0

    # if we are checking a single projection
    # or the data we've sent in is a single image
    if checked_projections == 1 or sample.shape[0] <= 1:
        # this will give us the middle slice
        current_slice_index = int(size / 2)
        slice_indices.append(current_slice_index)
    else:
        for c in range(checked_projections):
            current_slice_index += int(size / checked_projections)
            slice_indices.append(current_slice_index)

    left_crop_pos = config.pre.region_of_interest[
        0] if config.pre.region_of_interest is not None else 0
    image_width = sample.shape[2]

    # if crop coords match with the image width then the full image was
    # selected
    pixels_from_left_side = left_crop_pos if left_crop_pos - image_width <= 1 else 0

    h.pstart(
        "Starting COR calculation on {0} projections.".format(checked_projections))

    calculated_cors = []
    h.prog_init(len(slice_indices), "Center of Rotation", unit='calculations')
    for slice_idx in slice_indices:
        cor = tool.find_center(
            tomo=sample, theta=projection_angles, ind=slice_idx)

        print_proper_message(cor, h, pixels_from_left_side, slice_idx)
        h.prog_update()

        calculated_cors.append(cor)
    h.prog_close()
    h.pstop("Finished COR calculation.")

    average_cor_relative_to_crop = sum(calculated_cors) / len(calculated_cors)
    average_cor_relative_to_full_image = sum(
        calculated_cors) / len(calculated_cors) + pixels_from_left_side

    # we add the pixels cut off from the left, to reflect the full image in
    # Mantid
    h.tomo_print_note("Printing average COR in relation to image crop {0}: {1}".format(
        config.pre.region_of_interest, round(average_cor_relative_to_crop)))

    # new line for GUI to be able to read
    h.tomo_print_note("Printing average COR in relation to non-cropped image: \n{0}".format(
        round(average_cor_relative_to_full_image)))

    return round(average_cor_relative_to_full_image)


def print_proper_message(cor, h, pixels_from_left_side, slice_idx):
    v = h.get_verbosity()
    if v <= 2:
        h.tomo_print_same_line('.')
    else:
        h.tomo_print_note("COR for slice {0} .. REL to CROP {1} .. REL to FULL {2}"
                          .format(slice_idx, cor, (cor + pixels_from_left_side)), 3)
