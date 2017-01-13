from __future__ import (absolute_import, division, print_function)

# this will handle tool imports and running the correct recon runner
from recon.helper import Helper


def execute(config, cmd_line=None):
    """
    Run a reconstruction using a particular tool, algorithm and setup

    :param config :: The full configuration, after being read from the parameters

    :param cmd_line :: command line text if running from the CLI. When provided it will be written in the output
    readme file(s) for reference.
    """

    import os

    h = Helper()

    h.check_config_integrity(config)

    # import early to check if tool is available
    tool = load_tool(config, h)

    sample, flat, dark = load_data(config, h)

    # todo new class for readme
    _readme_fullpath = os.path.join(
        config.func.output_dir, config.func.readme_file_name)
    generate_readme_begin(_readme_fullpath, cmd_line, config, h)

    sample = pre_processing(config, sample, flat, dark)

    # Save pre-proc images, print inside
    import recon.data.saver as saver
    saver.save_preproc_images(sample, config)

    # ----------------------------------------------------------------
    return
    # Reconstruction
    sample = tool.run_reconstruct(sample, config)

    post_processing()

    # Save output from the reconstruction
    saver.save_recon_output(sample, config)

    save_netcdf_volume()

    # todo new class for readme
    generate_readme_end(_readme_fullpath, sample, config)

    pass


def pre_processing(config, data, flat, dark):
    from recon.filters import rotate_stack, crop_coords, normalise_by_flat_dark, normalise_by_air_region, cut_off, \
        mcp_corrections, scale_down, median_filter
    # _debug_save_out_data(data, config, flat, dark)

    d = True if config.func.debug else False

    data, flat, dark = rotate_stack.execute(data, config, flat, dark)
    if d:
        _debug_save_out_data(data, config, flat, dark, "_rotated")

    # the air region coordinates must be within the ROI if this is selected
    # TODO reflect change in GUI
    if config.pre.crop_before_normalize:
        data = crop_coords.execute_volume(data, config)

        flat = crop_coords.execute_image(flat, config)
        dark = crop_coords.execute_image(dark, config)

    # removes background using images taken when exposed to fully open beam and no beam
    data = normalise_by_flat_dark.execute(data, config, flat, dark)
    if d:
        _debug_save_out_data(data, config, flat, dark, "_normalised_by_flat_dark")

    # removes the contrast difference between the stack of images
    data = normalise_by_air_region.execute(data, config)
    if d:
        _debug_save_out_data(data, config, flat, dark, "_normalised_by_air")

    if not config.pre.crop_before_normalize:
        # in this case we don't care about cropping the flat and dark
        data = crop_coords.execute_volume(data, config)

    # cut_off
    # data = cut_off.execute(data, config)
    # mcp_corrections
    # data = mcp_corrections.execute(data, config)
    # scale_down, not implemented
    # data = scale_down.execute(data, config)

    data = median_filter.execute(data, config)
    if d:
        _debug_save_out_data(data, config, flat, dark, "_median_filtered")

    return data


def _debug_save_out_data(data, config, flat, dark, image_append):
    from recon.data import saver

    import time

    append_index = str(time.time())[-2:]

    saver.save_single_image(
        data, config, 'sample', image_name='sample'+image_append, image_index=append_index)
    saver.save_single_image(
        flat, config, 'flat', image_name='flat'+image_append, image_index=append_index)
    saver.save_single_image(
        dark, config, 'dark', image_name='dark'+image_append, image_index=append_index)


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
    # saver.save_recon_netcdf(recon_data, config.post.output_dir)
    # h.pstop(
    #     " * Finished saving reconstructed volume as NetCDF.")


def generate_readme_begin(_readme_fullpath, cmd_line, config, h):
    # TODO move this functionality into a new readme writer class!
    import recon.data.saver as saver

    h.pstart(" * Generating reconstruction script...")
    saver.gen_readme_summary_begin(_readme_fullpath, config, cmd_line)
    h.pstop(" * Finished generating script.")


def generate_readme_end(_readme_fullpath, data, config):
    pass
    # import recon.data.saver as saver
    # TODO move this functionality into a new readme writer class!

    # saver.gen_readme_summary_end(
    # _readme_fullpath,(data, preproc_data, recon_data), tstart, t_recon_end -
    # t_recon_start)


def load_tool(config, h):
    # First step import the tool
    h.pstart(" * Importing tool " + config.func.tool)
    # import tool
    from recon.tools import tool_importer
    # tomopy is the only supported tool for now
    tool = tool_importer.import_tool(config.func.tool)
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
