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

    _preproc_images_subdir_name = 'pre_processed'
    _out_readme_filename = '0.README_reconstruction.txt'
    _out_slices_filename_prefix = 'out_recon_slice'
    _out_horiz_slices_subdir = 'out_recon_horiz_slice'

    h = Helper()

    h.check_config_integrity(config)
    # ----------------------------------------------------------------

    # First step import the tool
    h.pstart(" * Importing tool " + config.func.tool)

    # import tool
    from recon.tools import tool_importer

    # tomopy is the only supported tool for now
    tool = tool_importer.import_tool(config.func.tool)

    h.pstop(" * Tool loaded.")

    # ----------------------------------------------------------------

    h.pstart(" * Loading data...")

    from recon.data import loader
    data, white, dark = loader.read_in_stack(
        config.pre.input_dir, config.pre.in_img_format,
        config.pre.input_dir_flat, config.pre.input_dir_dark)

    h.pstop(" * Data loaded. Shape of raw data: {0}, dtype: {1}.".format(
        data.shape, data.dtype))

    h.check_data_stack(data)

    # ----------------------------------------------------------------

    import os
    from recon.data import saver
    h.pstart(" * Generating reconstruction script...")

    readme_fullpath = os.path.join(
        config.func.output_dir, _out_readme_filename)

    # TODO move this timer somewhere else, or remove
    tstart = saver.gen_readme_summary_begin(readme_fullpath, config, cmd_line)

    h.pstop(" * Finished generating script.")

    # ----------------------------------------------------------------

    # TODO move into func

    if 'float64' == data.dtype:
        # this is done because tomoio.write has problems with float64 to
        # int16
        data = data.astype(dtype='float32')
        # print with top priority
        h.tomo_print(" * Note: pixel data type changed to: " + data.dtype)

    from recon.filters import rotate_stack, crop_coords, normalise_by_flat_dark, normalise_by_air_region, cut_off, \
        mcp_corrections, scale_down, median_filter
    data, white, dark = rotate_stack.execute(data, config, white, dark)

    crop_before_normaliz = False  # DEBUG
    if crop_before_normaliz:
        data = crop_coords.execute(data, config)

    data = normalise_by_flat_dark.execute(data, config, white, dark)
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

    # Save pre-proc images, print inside
    saver.save_preproc_images(config.post.output_dir, data,
                              config.pre)

    # ----------------------------------------------------------------

    # Reconstruction
    # for file readme summary

    import time
    t_recon_start = time.time()

    # TODO this needs to be changed to be dependent on the tool
    recon_data = tool.run_reconstruct()

    if 'astra' == alg_cfg.tool:
        # run_reconstruct_3d_astra(proj_data, algorithm, cor, proj_angles=proj_angles)
        return self.run_reconstruct_3d_astra_simple(
            proj_data, proj_angles, alg_cfg, preproc_cfg.cor,
            reconstruction_tool)

    t_recon_end = time.time()

    # ----------------------------------------------------------------

    # Post-processing
    self.apply_postproc_filters(recon_data, config.post)

    # Save output from the reconstruction
    self.save_recon_output(recon_data, config)

    # turned off for now, as it can't be opened from ParaView so it's a
    # waste
    save_netcdf_vol = False
    if save_netcdf_vol:
        h.pstart(
            " * Saving reconstructed volume as NetCDF...")
        tomoio.save_recon_netcdf(recon_data, config.post.output_dir)
        h.pstop(
            " * Finished saving reconstructed volume as NetCDF.")

    self.gen_readme_summary_end(readme_fullpath,
                                (data, preproc_data, recon_data), tstart,
                                t_recon_end - t_recon_start)
