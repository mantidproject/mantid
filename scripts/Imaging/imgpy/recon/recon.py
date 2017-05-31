from __future__ import (absolute_import, division, print_function)
import helper as h


def execute(config, cmd_line):
    """
    Run the whole reconstruction. The steps in the process are:
        - load the data
        - do pre_processing on the data
        - (optional) save out pre_processing images
        - do the reconstruction with the appropriate tool
        - save out reconstruction images

    The configuration for pre_processing and reconstruction are read from the config parameter.

    :param config: A ReconstructionConfig with all the necessary parameters to run a reconstruction.
    :param cmd_line: The full command line text if running from the CLI.
    """

    from imgdata.saver import Saver
    saver = Saver(config)

    h.initialise(config, saver)
    h.total_execution_timer()
    h.run_import_checks(config)
    h.check_config_integrity(config)

    # create directory, or throw if not empty and no --overwrite-all
    # we get the output path from the saver, as that expands variables and gets absolute path
    saver.make_dirs_if_needed(saver.get_output_path(), saver._overwrite_all)

    from readme import Readme
    readme = Readme(config, saver)
    readme.begin(cmd_line, config)
    h.set_readme(readme)

    # import early to check if tool is available
    from tools import importer
    tool = importer.timed_import(config)

    from imgdata import loader
    sample, flat, dark = loader.load_data(config)

    sample, flat, dark = pre_processing(config, sample, flat, dark)

    # Save pre-proc images, print inside
    saver.save_preproc_images(sample)
    if config.func.only_preproc is True:
        h.tomo_print_note("Only pre-processing run, exiting.")
        readme.end()
        return sample

    # ----------------------------------------------------------------
    # Reconstruction, output has different shape
    if not config.func.only_postproc:
        sample = tool.run_reconstruct(sample, config)
    else:
        h.tomo_print_note("Only post-processing run, skipping reconstruction.")

    sample = post_processing(config, sample)

    # Save output from the reconstruction
    saver.save_recon_output(sample)
    h.total_execution_timer()
    readme.end()
    return sample


def pre_processing(config, sample, flat, dark):
    """
    Does the pre-processing steps specified in the configuration file.

    :param config: A ReconstructionConfig with all the necessary parameters to run a reconstruction.
    :param sample: The sample image data as a 3D numpy.ndarray
    :param flat: The flat averaged image data as a 2D numpy.array
    :param dark: The dark averaged image data as a 2D numpy.array

    """

    if config.func.reuse_preproc:
        h.tomo_print_warning(
            "Pre-processing steps have been skipped, because --reuse-preproc or --only-postproc flag has been passed."
        )
        return sample, flat, dark

    from filters import rotate_stack, crop_coords, normalise_by_flat_dark, normalise_by_air_region, outliers, \
        rebin, median_filter, gaussian, cut_off, minus_log, value_scaling, stripe_removal

    cores = config.func.cores
    chunksize = config.func.chunksize
    roi = config.pre.region_of_interest
    sample, flat, dark = rotate_stack.execute(sample, config.pre.rotation,
                                              flat, dark, cores, chunksize)

    air = config.pre.normalise_air_region
    if (flat is not None and dark is not None) or air is not None:
        scale_factors = value_scaling.create_factors(sample, roi, cores,
                                                     chunksize)

    sample = normalise_by_flat_dark.execute(
        sample, flat, dark, config.pre.clip_min, config.pre.clip_max, roi,
        cores, chunksize)

    # removes the contrast difference between the stack of images
    sample = normalise_by_air_region.execute(sample, air, roi, crop, cores,
                                             chunksize)

    # scale up the data to a nice int16 range while keeping the effects
    # from the flat/dark and air normalisations
    if (flat is not None and dark is not None) or air is not None:
        sample = value_scaling.apply_factor(sample, scale_factors, cores,
                                            chunksize)

    sample = crop_coords.execute_volume(sample, roi)
    flat = crop_coords.execute_image(flat, roi) if flat is not None else flat
    dark = crop_coords.execute_image(dark, roi) if dark is not None else dark

    sample = rebin.execute(sample, config.pre.rebin, config.pre.rebin_mode,
                           cores, chunksize)

    sample = stripe_removal.execute(
        sample, config.pre.stripe_removal_wf, config.pre.stripe_removal_ti,
        config.pre.stripe_removal_sf, cores, chunksize)

    sample = outliers.execute(sample, config.pre.outliers_threshold,
                              config.pre.outliers_radius, cores)

    sample = median_filter.execute(sample, config.pre.median_size,
                                   config.pre.median_mode, cores, chunksize)

    sample = gaussian.execute(sample, config.pre.gaussian_size,
                              config.pre.gaussian_mode,
                              config.pre.gaussian_order, cores, chunksize)

    sample = minus_log.execute(sample, config.pre.minus_log)

    sample = cut_off.execute(sample, config.pre.cut_off)

    return sample, flat, dark


def post_processing(config, recon_data):
    """
    Does the post-processing steps specified in the configuration file.

    :param config: A ReconstructionConfig with all the necessary parameters to run a reconstruction.
    :param recon_data: The reconstructed image data as a 3D numpy.ndarray
    :return: The reconstructed data.
    """
    if config.func.no_postproc:
        h.tomo_print_warning(
            "Post-processing steps have been skipped, because --no-postproc flag has been passed."
        )
        return recon_data

    from filters import circular_mask, gaussian, median_filter, outliers, ring_removal

    cores = config.func.cores

    recon_data = outliers.execute(recon_data, config.post.outliers_threshold,
                                  config.post.outliers_radius, cores)
    recon_data = ring_removal.execute(
        recon_data, config.post.ring_removal,
        config.post.ring_removal_center_x, config.post.ring_removal_center_y,
        config.post.ring_removal_thresh, config.post.ring_removal_thresh_max,
        config.post.ring_removal_thresh_min,
        config.post.ring_removal_theta_min, config.post.ring_removal_rwidth,
        cores, config.func.chunksize)

    recon_data = median_filter.execute(recon_data, config.post.median_size,
                                       config.post.median_mode, cores,
                                       config.func.chunksize)

    recon_data = gaussian.execute(
        recon_data, config.post.gaussian_size, config.post.gaussian_mode,
        config.post.gaussian_order, cores, config.func.chunksize)

    recon_data = circular_mask.execute(recon_data, config.post.circular_mask,
                                       config.post.circular_mask_val, cores)

    return recon_data
