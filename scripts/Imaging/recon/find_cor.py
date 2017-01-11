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
    tool = tool_importer.import_tool(config.alg_cfg.tool)

    h.pstop(" * Tool loaded.")

    # -----------------------------------------------------------------------------
    h.pstart(" * Loading data...")

    from recon.data import loader
    sample, white, dark = loader.read_in_stack(
        config.preproc_cfg.input_dir, config.preproc_cfg.in_img_format,
        config.preproc_cfg.input_dir_flat, config.preproc_cfg.input_dir_dark)

    h.pstop(
        " * Data loaded. Shape of raw data: {0}, dtype: {1}.".format(
            sample.shape, sample.dtype))

    # -----------------------------------------------------------------------------

    # import all used filters
    from recon.filters import rotate_stack, crop_coords

    h.pstart(" * Rotating stack...")

    sample, white, dark = rotate_stack.execute(sample, config.preproc_cfg)

    h.pstop(" * Finished rotating stack.")

    h.pstart(" * Cropping images...")
    # crop the ROI, this is done first, so beware of what the correct ROI
    # coordinates are
    sample = crop_coords.execute(sample, config.preproc_cfg)

    h.pstop("* Finished cropping images.")

    # sanity check
    h.tomo_print(" * Sanity check on data", 0)
    h.check_data_stack(sample)

    num_projections = sample.shape[0]
    projection_angle_increment = float(
        config.preproc_cfg.max_angle) / (num_projections - 1)

    h.tomo_print(" * Calculating projection angles")
    projection_angles = np.arange(
        0, num_projections * projection_angle_increment, projection_angle_increment)

    # For tomopy
    h.tomo_print(" * Calculating radians for TomoPy")
    projection_angles = np.radians(projection_angles)

    size = int(num_projections)

    # depending on the number of COR projections it will select different
    # slice indices
    checked_projections = 6
    slice_indices = []
    current_slice_index = 0

    if checked_projections < 2:
        # this will give us the middle slice
        current_slice_index = int(size / 2)
        slice_indices.append(current_slice_index)
    else:
        for c in range(checked_projections):
            current_slice_index += int(size / checked_projections)
            slice_indices.append(current_slice_index)

    h.pstart(" * Starting COR calculation on " +
             str(checked_projections) + " projections.")

    crop_coords = config.preproc_cfg.crop_coords[0]
    image_width = sample.shape[2]

    # if crop coords match with the image width then the full image was
    # selected
    pixels_from_left_side = crop_coords if crop_coords - image_width <= 1 else 0

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
    h.tomo_print(" * Printing average COR in relation to cropped image {0}:{1}".format(
        str(config.preproc_cfg.crop_coords), str(round(average_cor_relative_to_crop))))

    h.tomo_print(" * Printing average COR in relation to FULL image:{0}".format(
        str(round(average_cor_relative_to_full_image))))
