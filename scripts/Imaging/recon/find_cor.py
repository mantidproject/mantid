from __future__ import (absolute_import, division, print_function)

import numpy as np

from recon.helper import Helper


def execute(config):
    h = Helper()
    h.check_config_integrity(config)

    # -----------------------------------------------------------------------------

    h.pstart(" * Importing tool " + config.func.tool)

    # import tool
    from recon.tools import tool_importer

    # tomopy is the only supported tool for now
    tool = tool_importer.import_tool(config.func.tool)

    h.pstop(" * Tool loaded.")

    # -----------------------------------------------------------------------------
    h.pstart(" * Loading data...")

    from recon.data import loader
    sample, flat, dark = loader.read_in_stack(config)  # and onwards we go!

    h.pstop(
        " * Data loaded. Shape of raw data: {0}, dtype: {1}.".format(
            sample.shape, sample.dtype))

    # -----------------------------------------------------------------------------

    # import all used filters
    from recon.filters import rotate_stack, crop_coords

    h.pstart(" * Rotating stack...")

    sample, flat, dark = rotate_stack.execute(sample, config, flat, dark)

    h.pstop(" * Finished rotating stack.")

    h.pstart(" * Cropping images...")
    # crop the ROI, this is done first, so beware of what the correct ROI
    # coordinates are
    sample = crop_coords.execute(sample, config)

    h.pstop("* Finished cropping images.")

    num_projections = sample.shape[0]
    projection_angle_increment = float(config.pre.max_angle) / num_projections

    h.tomo_print(" * Calculating projection angles")
    projection_angles = np.arange(
        0, num_projections * projection_angle_increment, projection_angle_increment)

    # For tomopy
    h.tomo_print(" * Calculating radians for TomoPy")
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

    h.pstart(
        " * Starting COR calculation on {0} projections.".format(checked_projections))

    left_crop_pos = config.pre.crop_coords[0]
    image_width = sample.shape[2]

    # if crop coords match with the image width then the full image was
    # selected
    pixels_from_left_side = left_crop_pos if left_crop_pos - image_width <= 1 else 0

    calculated_cors = []
    for slice_idx in slice_indices:
        cor = tool.find_center(
            tomo=sample, theta=projection_angles, ind=slice_idx, emission=False)

        h.tomo_print(" ** COR for slice" + str(slice_idx) + ".. REL to CROP " +
                     str(cor) + ".. REL to FULL " + str(cor + pixels_from_left_side), 3)

        calculated_cors.append(cor)

    h.pstop(" * Finished COR calculation.", 2)

    average_cor_relative_to_crop = sum(calculated_cors) / len(calculated_cors)
    average_cor_relative_to_full_image = sum(
        calculated_cors) / len(calculated_cors) + pixels_from_left_side

    # we add the pixels cut off from the left, to reflect the full image in
    # Mantid
    h.tomo_print(" * Printing average COR in relation to image crop {0}: {1}".format(
        config.pre.crop_coords, round(average_cor_relative_to_crop)))

    h.tomo_print(" * Printing average COR in relation to non-cropped image: {0}".format(
        round(average_cor_relative_to_full_image)))
