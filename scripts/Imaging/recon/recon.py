from __future__ import (absolute_import, division, print_function)
from helper import Helper


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

    h = Helper(config)
    config.helper = h
    h.total_execution_timer()
    h.run_import_checks()
    h.check_config_integrity(config)

    from imgdata.saver import Saver
    saver = Saver(config, h)
    # create directory, or throw if not empty and no --overwrite-all
    saver.make_dirs_if_needed()

    from imgdata.readme import Readme
    readme = Readme(config, saver, h)
    readme.begin(cmd_line, config)
    h.set_readme(readme)

    # import early to check if tool is available
    from recon.tools import importer
    tool = importer.timed_import(config, h)

    from imgdata import loader
    sample, flat, dark = loader.load_data(config, h)

    sample, flat, dark = pre_processing(config, sample, flat, dark)

    # Save pre-proc images, print inside
    saver.save_preproc_images(sample, flat, dark)
    if config.func.only_preproc is True:
        h.tomo_print_note("Only pre-processing run, exiting.")
        readme.end()
        return sample

    # ----------------------------------------------------------------
    # Reconstruction, output has different shape
    recon = tool.run_reconstruct(sample, config, h)

    recon = post_processing(config, recon)

    # Save output from the reconstruction
    saver.save_recon_output(recon)
    h.total_execution_timer()
    readme.end()
    return recon


def pre_processing(config, sample, flat, dark, h=None):
    """
    Does the pre-processing steps specified in the configuration file.

    :param config: A ReconstructionConfig with all the necessary parameters to run a reconstruction.
    :param sample: The sample image data as a 3D numpy.ndarray
    :param flat: The flat averaged image data as a 2D numpy.array
    :param dark: The dark averaged image data as a 2D numpy.array
    :param h: Helper class, if not provided will be initialised with the config

    """
    h = Helper(config) if h is None else h

    if config.func.reuse_preproc is True:
        h.tomo_print_warning(
            "Pre-processing steps have been skipped, because --reuse-preproc flag has been passed."
        )
        return sample, flat, dark

    from filters import rotate_stack, crop_coords, normalise_by_flat_dark, normalise_by_air_region, outliers, \
        rebin, median_filter, gaussian

    cores = config.func.cores
    chunksize = config.func.chunksize

    sample, flat, dark = rotate_stack.execute(
        sample,
        config.pre.rotation,
        flat,
        dark,
        cores=cores,
        chunksize=chunksize,
        h=h)

    # the air region coordinates must be within the ROI if this is selected
    if config.pre.crop_before_normalise:
        sample = crop_coords.execute_volume(sample,
                                            config.pre.region_of_interest, h)

        if flat is not None:
            flat = crop_coords.execute_image(flat,
                                             config.pre.region_of_interest, h)

        if dark is not None:
            dark = crop_coords.execute_image(dark,
                                             config.pre.region_of_interest, h)

    # removes background using images taken when exposed to fully open beam
    # and no beam
    sample = normalise_by_flat_dark.execute(
        sample,
        flat,
        dark,
        config.pre.clip_min,
        config.pre.clip_max,
        cores=cores,
        chunksize=chunksize,
        h=h)

    # removes the contrast difference between the stack of images
    air = config.pre.normalise_air_region
    roi = config.pre.region_of_interest
    crop = config.pre.crop_before_normalise

    sample = normalise_by_air_region.execute(
        sample, air, roi, crop, cores=cores, chunksize=chunksize, h=h)

    if not config.pre.crop_before_normalise:
        # in this case we don't care about cropping the flat and dark
        sample = crop_coords.execute_volume(sample,
                                            config.pre.region_of_interest, h)

        if flat is not None:
            flat = crop_coords.execute_image(flat,
                                             config.pre.region_of_interest, h)

        if dark is not None:
            dark = crop_coords.execute_image(dark,
                                             config.pre.region_of_interest, h)

    sample = outliers.execute(sample, config.pre.outliers_threshold,
                              config.pre.outliers_mode, h)

    # mcp_corrections, they have to be included as well
    # data = mcp_corrections.execute(data, config)

    sample = rebin.execute(
        sample,
        config.pre.rebin,
        config.pre.rebin_mode,
        cores=cores,
        chunksize=chunksize,
        h=h)

    sample = median_filter.execute(
        sample,
        config.pre.median_size,
        config.pre.median_mode,
        cores=cores,
        chunksize=chunksize,
        h=h)

    sample = gaussian.execute(
        sample,
        config.pre.gaussian_size,
        config.pre.gaussian_mode,
        config.pre.gaussian_order,
        cores=cores,
        chunksize=chunksize,
        h=h)

    return sample, flat, dark


def post_processing(config, recon_data, h=None):
    """
    Does the post-processing steps specified in the configuration file.

    :param config: A ReconstructionConfig with all the necessary parameters to run a reconstruction.
    :param recon_data: The reconstructed image data as a 3D numpy.ndarray
    :param h: Helper class, if not provided will be initialised with the config
    :return: The reconstructed data.
    """
    from filters import circular_mask, gaussian, median_filter, outliers

    h = Helper(config) if h is None else h

    recon_data = circular_mask.execute(recon_data, config.post.circular_mask,
                                       config.post.circular_mask_val, h)

    recon_data = outliers.execute(recon_data, config.post.outliers_threshold,
                                  config.post.outliers_mode, h)

    recon_data = gaussian.execute(recon_data, config.post.gaussian_size,
                                  config.post.gaussian_mode,
                                  config.post.gaussian_order,
                                  config.func.cores, config.func.chunksize, h)

    recon_data = median_filter.execute(
        recon_data, config.post.median_size, config.post.median_mode,
        config.func.cores, config.func.chunksize, h)

    return recon_data
