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

    tool = load_tool(config, h)

    data, flat, dark = load_data(config, h)

    # todo new class for readme
    _readme_fullpath = os.path.join(config.func.output_dir, config.func.readme_file_name)
    generate_readme_begin(_readme_fullpath, cmd_line, config, h)

    preprocessing(config, data, flat, dark)

    # Save pre-proc images, print inside
    import recon.data.saver as saver
    saver.save_preproc_images(data, config)

    # ----------------------------------------------------------------

    # Reconstruction
    data = tool.run_reconstruct(data, config)

    postprocessing()

    # Save output from the reconstruction
    saver.save_recon_output(data, config)

    save_netcdf_volume()

    # todo new class for readme
    generate_readme_end(_readme_fullpath, data, config)

    pass


def preprocessing(config, data, flat, dark):
    from recon.filters import rotate_stack, crop_coords, normalise_by_flat_dark, normalise_by_air_region, cut_off, \
        mcp_corrections, scale_down, median_filter

    data, flat, dark = rotate_stack.execute(data, config, flat, dark)
    crop_before_normaliz = False  # DEBUG
    if crop_before_normaliz:
        data = crop_coords.execute(data, config)
    data = normalise_by_flat_dark.execute(data, config, flat, dark)
    data = normalise_by_air_region.execute(data, config)
    if not crop_before_normaliz:
        data = crop_coords.execute(data, config)

    # cut_off
    data = cut_off.execute(data, config)
    # mcp_corrections
    data = mcp_corrections.execute(data, config)
    # scale_down, not implemented
    # data = scale_down.execute(data, config)
    # median filter
    # h.pstart(
    #     " * Starting noise filter / median, with pixel data type: {0}, filter size/width: {1}.".
    #         format(data.dtype, config.median_filter_size))
    data = median_filter.execute(data, config)
    # h.pstop(
    #     " * Finished noise filter / median, with pixel data type: {0}, filter size/width: {1}.".
    #         format(data.dtype, config.median_filter_size))

    # ----------------------------------------------------------------


def postprocessing():
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
    #     _readme_fullpath,(data, preproc_data, recon_data), tstart, t_recon_end - t_recon_start)


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

    data, white, dark = loader.read_in_stack(config)

    h.pstop(" * Data loaded. Shape of raw data: {0}, dtype: {1}.".format(
        data.shape, data.dtype))

    h.check_data_stack(data)

    return dark, data, white
