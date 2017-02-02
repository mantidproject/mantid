from __future__ import (absolute_import, division, print_function)


def execute(config, cmd_line):
    """
    Run a reconstruction using a particular tool, algorithm and setup

    :param config :: The full configuration, after being read from the parameters
    :param cmd_line :: command line text if running from the CLI. When provided it will be written in the output
    :param h:
    readme file(s) for reference.
    """

    from recon.helper import Helper
    h = Helper(config)
    h.total_execution_timer()
    config.helper = h
    h.check_config_integrity(config)

    from recon.data.saver import Saver
    saver = Saver(config, h)
    # create directory, or throw if not empty and no --overwrite-all
    saver.make_dirs_if_needed()

    from recon.data.readme import Readme
    readme = Readme(config, saver, h)
    readme.begin(cmd_line, config)

    h.set_readme(readme)
    h.run_import_checks()

    # import early to check if tool is available
    tool = load_tool(config, h)

    from recon.data import loader
    sample, flat, dark = loader.load_data(config, h)

    sample, flat, dark = pre_processing(config, sample, flat, dark)

    # Save pre-proc images, print inside
    saver.save_preproc_images(sample, flat, dark)
    if config.func.only_preproc is True:
        h.tomo_print_note("Only pre-processing run, exiting.")
        readme.end()
        return 14

    # ----------------------------------------------------------------
    # Reconstruction
    sample = tool.run_reconstruct(sample, config, h)

    sample = post_processing(sample, config)

    # Save output from the reconstruction
    saver.save_recon_output(sample)
    h.total_execution_timer()
    readme.end()
    return 0


def pre_processing(config, sample, flat, dark):
    h = config.helper
    if config.func.reuse_preproc is True:
        h.tomo_print_warning(
            "Pre-processing steps have been skipped, because --reuse-preproc flag has been passed.")
        return sample

    from recon.filters import rotate_stack, crop_coords, normalise_by_flat_dark, normalise_by_air_region, outliers, \
        mcp_corrections, rebin, median_filter, gaussian

    cores = config.func.cores
    chunksize = config.func.chunksize
    save_preproc = config.func.save_preproc
    debug = True if config.func.debug else False

    sample, flat, dark = rotate_stack.execute(
        sample, config.pre.rotation, flat, dark, cores=cores, chunksize=chunksize, h=h)
    h.save_debug(sample, config, flat, dark, "1rotated",
                 debug, save_preproc, config.pre.rotation)

    # the air region coordinates must be within the ROI if this is selected
    if config.pre.crop_before_normalise:
        sample = crop_coords.execute_volume(
            sample, config.pre.region_of_interest, h)

        flat = crop_coords.execute_image(
            flat, config.pre.region_of_interest, h)
        dark = crop_coords.execute_image(
            dark, config.pre.region_of_interest, h)

        h.save_debug(sample, config, flat, dark, "2cropped",
                     config.pre.crop_before_normalise)

    # removes background using images taken when exposed to fully open beam
    # and no beam
    sample = normalise_by_flat_dark.execute(
        sample, flat, dark, config.pre.clip_min, config.pre.clip_max, cores=cores, chunksize=chunksize, h=h)
    h.save_debug(sample, config, flat, dark,
                 "3norm_by_flat_dark", debug, save_preproc, flat, dark)

    # removes the contrast difference between the stack of images
    air = config.pre.normalise_air_region
    roi = config.pre.region_of_interest
    crop = config.pre.crop_before_normalise

    sample = normalise_by_air_region.execute(
        sample, air, roi, crop, cores=cores, chunksize=chunksize, h=h)
    h.save_debug(sample, config, flat, dark, "4norm_by_air",
                 debug, save_preproc, air, roi, crop)

    if not config.pre.crop_before_normalise:
        # in this case we don't care about cropping the flat and dark
        sample = crop_coords.execute_volume(
            sample, config.pre.region_of_interest, h)

        flat = crop_coords.execute_image(
            flat, config.pre.region_of_interest, h)
        dark = crop_coords.execute_image(
            dark, config.pre.region_of_interest, h)

        h.save_debug(sample, config, flat, dark, "5cropped",
                     debug, save_preproc, crop)

    sample = outliers.execute(
        sample, config.pre.outliers_threshold, config.pre.outliers_mode, h)
    h.save_debug(sample, config, flat, dark, "6outliers",
                 debug, save_preproc, config.pre.outliers_threshold)

    # mcp_corrections
    # data = mcp_corrections.execute(data, config)

    sample = rebin.execute(sample, config.pre.rebin,
                           config.pre.rebin_mode, cores=cores, chunksize=chunksize, h=h)
    h.save_debug(sample, config, flat, dark, "7scaled",
                 debug, save_preproc, config.pre.rebin)

    sample = median_filter.execute(
        sample, config.pre.median_size, config.pre.median_mode, cores=cores, chunksize=chunksize, h=h)
    h.save_debug(sample, config, flat, dark, "8median_filtered",
                 debug, save_preproc, config.pre.median_size)

    sample = gaussian.execute(sample, config.pre.gaussian_size, config.pre.gaussian_mode,
                              config.pre.gaussian_order, cores=cores, chunksize=chunksize, h=h)
    h.save_debug(sample, config, flat, dark, "9gaussian",
                 debug, save_preproc, config.pre.gaussian_size)

    return sample, flat, dark


def _debug_save_out_data(data, config, flat=None, dark=None, out_path_append='', image_append=''):
    from recon.data.saver import Saver

    saver = Saver(config)

    saver.save_single_image(
        data, subdir=out_path_append, image_name='sample' + image_append)

    if flat is not None:
        saver.save_single_image(
            flat, subdir=out_path_append, image_name='flat' + image_append)

    if dark is not None:
        saver.save_single_image(
            dark, subdir=out_path_append, image_name='dark' + image_append)


def post_processing(recon_data, config):
    from recon.filters import circular_mask, gaussian, median_filter, outliers

    h = config.helper
    debug = True if config.func.debug else False

    recon_data = circular_mask.execute(
        recon_data, config.post.circular_mask, h)
    if debug and config.post.circular_mask is not None:
        _debug_save_out_data(recon_data, config, out_path_append='../post_processed/1circular_masked',
                             image_append='_circular_masked')

    recon_data = outliers.execute(
        recon_data, config.post.outliers_threshold, config.post.outliers_mode, h)
    if debug and config.post.outliers_threshold is not None:
        _debug_save_out_data(
            recon_data, config, out_path_append='../post_processed/2outliers', image_append='_outliers')

    recon_data = gaussian.execute(recon_data, config.post.gaussian_size, config.post.gaussian_mode,
                                  config.post.gaussian_order, config.func.cores, config.func.chunksize, h)
    if debug and config.post.gaussian_size is not None:
        _debug_save_out_data(
            recon_data, config, out_path_append='../post_processed/3gaussian', image_append='_gaussian')

    recon_data = median_filter.execute(
        recon_data, config.post.median_size, config.post.median_mode, config.func.cores, config.func.chunksize, h)

    if debug and config.post.median_size is not None:
        _debug_save_out_data(
            recon_data, config, out_path_append='../post_processed/4median', image_append='_median')

    return recon_data


def save_netcdf_volume():
    pass
    # turned off for now, as it can't be opened from ParaView so it's a
    # waste
    # save_netcdf_vol = False
    # if save_netcdf_vol:
    #     h.pstart(
    #         "Saving reconstructed volume as NetCDF...")
    # saver.save_recon_netcdf(recon_data, config.post.output_path)
    # h.pstop(
    #     "Finished saving reconstructed volume as NetCDF.")


def load_tool(config, h):
    # First step import the tool
    h.pstart("Importing tool " + config.func.tool)
    # import tool
    from recon.tools import tool_importer
    # tomopy is the only supported tool for now
    tool = tool_importer.do_importing(config.func.tool)
    tool.check_algorithm_compatibility(config)

    h.pstop("Tool loaded.")
    return tool
