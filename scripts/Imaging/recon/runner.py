from __future__ import (absolute_import, division, print_function)

# this will handle tool imports and running the correct recon runner


def execute(config, cmd_line=None):
    """
    Run a reconstruction using a particular tool, algorithm and setup

    :param config :: The full configuration, after being read from the parameters

    :param cmd_line :: command line text if running from the CLI. When provided it will be written in the output
    readme file(s) for reference.
    """

    import os

    from recon.helper import Helper
    h = Helper(config)

    from recon.data.saver import Saver
    saver = Saver(config)

    h.check_config_integrity(config)

    # import early to check if tool is available
    tool = load_tool(config, h)

    sample, flat, dark = load_data(config, h)

    # todo new class for readme

    saver.gen_readme_summary_begin(cmd_line, config)

    sample = pre_processing(config, sample, flat, dark)

    # Save pre-proc images, print inside
    saver.save_preproc_images(sample)

    # ----------------------------------------------------------------
    # Reconstruction
    sample = tool.run_reconstruct(sample, config)

    post_processing()

    # Save output from the reconstruction
    saver.save_recon_output(sample)

    save_netcdf_volume()

    # todo new class for readme
    # saver.gen_readme_summary_end(sample)


def pre_processing(config, data, flat, dark):
    from recon.filters import rotate_stack, crop_coords, normalise_by_flat_dark, normalise_by_air_region, cut_off, \
        mcp_corrections, scale_down, median_filter

    save_preproc = config.func.save_preproc
    debug = True if config.func.debug else False

    data, flat, dark = rotate_stack.execute(data, config, flat, dark)
    if debug and save_preproc:
        _debug_save_out_data(data, config, flat, dark, "rotated", "_rotated")

    # the air region coordinates must be within the ROI if this is selected
    if config.pre.crop_before_normalise:
        data = crop_coords.execute_volume(data, config)

        flat = crop_coords.execute_image(flat, config)
        dark = crop_coords.execute_image(dark, config)

        if debug and save_preproc:
            _debug_save_out_data(data, config, flat, dark,
                                 "cropped", "_cropped")

    # removes background using images taken when exposed to fully open beam
    # and no beam
    data = normalise_by_flat_dark.execute(data, config, flat, dark)
    if debug and save_preproc:
        _debug_save_out_data(data, config, flat, dark,
                             "norm_by_flat_dark", "_normalised_by_flat_dark")

    # removes the contrast difference between the stack of images
    data = normalise_by_air_region.execute(data, config)
    if debug and save_preproc:
        _debug_save_out_data(data, config, flat, dark,
                             "norm_by_air",  "_normalised_by_air")

    if not config.pre.crop_before_normalise:
        # in this case we don't care about cropping the flat and dark
        data = crop_coords.execute_volume(data, config)

        if debug and save_preproc:
            _debug_save_out_data(data, config, flat, dark,
                                 "cropped", "_cropped")

    # cut_off
    # data = cut_off.execute(data, config)
    # mcp_corrections
    # data = mcp_corrections.execute(data, config)
    # scale_down, not implemented
    # data = scale_down.execute(data, config)

    data = median_filter.execute(data, config)
    if debug and save_preproc:
        _debug_save_out_data(data, config, flat, dark,
                             "median_filtered", "_median_filtered")

    return data


def _debug_save_out_data(data, config, flat, dark, out_path_append, image_append):
    from recon.data import saver

    import time

    saver.save_single_image(
        data, config, output_path=out_path_append, image_name='sample' + image_append)
    saver.save_single_image(
        flat, config, output_path=out_path_append, image_name='flat' + image_append)
    saver.save_single_image(
        dark, config, output_path=out_path_append, image_name='dark' + image_append)


def post_processing():
    pass
    # TODO Post-processing
    # ----------------------------------------------------------------
    # from recon.filters import circular_mask, gaussian, median_filter, cut_off
    # circular_mask.execute(recon_data, config)
    # cut_off.execute(data, config)
    # gaussian.execute(data, config)
    # median_filter.execute(data, config)


def save_netcdf_volume():
    pass
    # turned off for now, as it can't be opened from ParaView so it's a
    # waste
    # save_netcdf_vol = False
    # if save_netcdf_vol:
    #     h.pstart(
    #         " * Saving reconstructed volume as NetCDF...")
    # saver.save_recon_netcdf(recon_data, config.post.output_path)
    # h.pstop(
    #     " * Finished saving reconstructed volume as NetCDF.")


def load_tool(config, h):
    # First step import the tool
    h.pstart(" * Importing tool " + config.func.tool)
    # import tool
    from recon.tools import tool_importer
    # tomopy is the only supported tool for now
    tool = tool_importer.do_importing(config.func.tool)

    h.pstop(" * Tool loaded.")
    return tool


def load_data(config, h):
    h.pstart(" * Loading data...")

    from recon.data import loader

    sample, flat, dark = loader.read_in_stack(config)

    h.pstop(" * Data loaded. Shape of raw data: {0}, dtype: {1}.".format(
        sample.shape, sample.dtype))

    h.check_data_stack(sample)

    return sample, flat, dark
