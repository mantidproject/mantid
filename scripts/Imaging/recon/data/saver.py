def save_recon_output(recon_data, cfg, save_horiz_slices=False):
    """
    Save output reconstructed volume in different forms.

    @param recon_data :: reconstructed data volume. A sequence of images will be saved from this
    @param cfg :: configuration of the reconstruction, including output paths and formats
    @param save_horiz_slices :: Save images along the horizontal axis in addition to the vertical
    slices saved by defult. Useful for testing
    some tools
    """
    # slices along the vertical (z) axis
    # output_dir = 'output_recon_tomopy'
    output_dir = cfg.postproc_cfg.output_dir
    out_recon_dir = os.path.join(output_dir, 'reconstructed')
    self.tomo_print_timed_start(
        " * Starting saving slices of the reconstructed volume in: {0}...".
        format(out_recon_dir))
    tomoio.save_recon_as_vertical_slices(
        recon_data,
        out_recon_dir,
        name_prefix=self._OUT_SLICES_FILENAME_PREFIX,
        img_format=cfg.pre.out_img_format)

    # Sideways slices:
    save_horiz_slices = False
    if save_horiz_slices:
        out_horiz_dir = os.path.join(output_dir, 'horiz_slices')
        print("* Saving horizontal slices in: {0}".format(out_horiz_dir))
        tomoio.save_recon_as_horizontal_slices(
            recon_data,
            out_horiz_dir,
            name_prefix=self._OUT_HORIZ_SLICES_SUBDIR,
            img_format=cfg.pre.out_img_format)

    self.tomo_print_timed_stop(
        " * Finished saving slices of the reconstructed volume in: {0}".
        format(out_recon_dir))


def save_preproc_images(data, config):
    """
    Save (pre-processed) images from a data array to image files.

    @param data :: The pre-processed data that will be saved
    @param config :: The full reconstruction config
    @param out_dtype :: dtype used for the pixel type/depth in the output image files
    """
    import os
    import numpy as np
    from recon.helper import Helper
    h = Helper(config)

    min_pix = np.amin(data)
    max_pix = np.amax(data)

    preproc_dir = os.path.join(
        config.func.output_dir, config.func.preproc_images_subdir)

    h.pstart(
        " * Saving all pre-processed images (data) into {0} dtype: {1}".format(preproc_dir, data.dtype))

    make_dirs_if_needed(preproc_dir)

    for idx in range(0, data.shape[0]):
        # rescale_intensity has issues with float64=>int16
        write_image(data[idx, :, :], os.path.join(
            preproc_dir, 'out_preproc_proj_image' + str(idx).zfill(6)))
        # write_image(data[idx, :, :], min_pix, max_pix,
        # os.path.join(preproc_dir, 'out_preproc_proj_image' +
        # str(idx).zfill(6)))

    h.pstop(" * Saving pre-processed images finished.")


def write_image(img_data, filename):
    """
    Output image data, given as a numpy array, to a file, in a given image format.
    Assumes that the output directory exists (must be checked before). The pixel
    values are rescaled in the range [min_pix, max_pix] which would normally be set
    to the minimum/maximum values found in a stack of images.

    @param img_data :: image data in the usual numpy representation

    @param min_pix :: minimum reference value to rescale data (may be local to an
    image or global for a stack of images)
    @param max_pix :: maximum reference value to rescale data (may be local to an
    image or global for a stack of images)

    @param filename :: file name, including directory and extension
    @param img_format :: image file format
    @param dtype :: can be used to force a pixel type, otherwise the type
    of the input data is used

    Returns:: name of the file saved
    """
    import loader
    fits = loader.import_pyfits()
    hdu = fits.PrimaryHDU(img_data[:, :])
    hdulist = fits.HDUList([hdu])
    hdulist.writeto(filename + ".fits")

    return filename


def gen_readme_summary_begin(filename, cfg, cmd_line):
    """
    To write configuration, settings, etc. early on. As early as possible, before any failure
    can happen.

    @param filename :: name of the readme/final report file
    @param cfg :: full reconstruction configuration
    @param cmd_line :: command line originally used to run this reconstruction, when running
    from the command line

    Returns :: time now (begin of run) in number of seconds since epoch (time() time)
    """

    import time
    tstart = time.time()

    # generate file with dos/windows line end for windows users convenience
    with open(filename, 'w') as oreadme:
        file_hdr = (
            'Tomographic reconstruction. Summary of inputs, settings and outputs.\n'
            'Time now (run begin): ' + time.ctime(tstart) + '\n')
        oreadme.write(file_hdr)

        alg_hdr = ("\n"
                   "--------------------------\n"
                   "Tool/Algorithm\n"
                   "--------------------------\n")
        oreadme.write(alg_hdr)
        oreadme.write(str(cfg.alg_cfg))
        oreadme.write("\n")

        preproc_hdr = ("\n"
                       "--------------------------\n"
                       "Pre-processing parameters\n"
                       "--------------------------\n")
        oreadme.write(preproc_hdr)
        oreadme.write(str(cfg.pre))
        oreadme.write("\n")

        postproc_hdr = ("\n"
                        "--------------------------\n"
                        "Post-processing parameters\n"
                        "--------------------------\n")
        oreadme.write(postproc_hdr)
        oreadme.write(str(cfg.postproc_cfg))
        oreadme.write("\n")

        cmd_hdr = ("\n"
                   "--------------------------\n"
                   "Command line\n"
                   "--------------------------\n")
        oreadme.write(cmd_hdr)
        oreadme.write(cmd_line)
        oreadme.write("\n")


def make_dirs_if_needed(dirname):
    """
    Makes sure that the directory needed (for example to save a file)
    exists, otherwise creates it.

    @param dirname :: (output) directory to check

    """
    import os
    absname = os.path.abspath(dirname)
    if not os.path.exists(absname):
        os.makedirs(absname)


def gen_readme_summary_end(filename, data_stages, tstart,
                           t_recon_elapsed):
    """
    Write last part of report in the output readme/report file. This should be used whenever a
    reconstruction runs correctly.

    @param filename :: name of the readme/final report file
    @param data_stages :: tuple with data in three stages (raw, pre-processed, reconstructed)
    @param tstart :: time at the beginning of the job/reconstruction, when the first part of the
    readme file was written
    @param t_recon_elapsed :: reconstruction time
    """
    # append to a readme/report that should have been pre-filled with the
    # initial configuration
    with open(filename, 'a') as oreadme:
        import time
        run_hdr = ("\n"
                   "--------------------------\n"
                   "Run/job details:\n"
                   "--------------------------\n")
        oreadme.write(run_hdr)
        (raw_data, preproc_data, recon_data) = data_stages

        oreadme.write("Dimensions of raw input sample data: {0}\n".format(
            raw_data.shape))
        oreadme.write("Dimensions of pre-processed sample data: {0}\n".
                      format(preproc_data.shape))
        oreadme.write("Dimensions of reconstructed volume: {0}\n".format(
            recon_data.shape))

        oreadme.write("Raw input pixel type: {0}\n".format(raw_data.dtype))
        oreadme.write("Output pixel type: {0}\n".format('uint16'))
        oreadme.write("Time elapsed in reconstruction: {0:.3f}s\r\n".
                      format(t_recon_elapsed))
        tend = time.time()
        oreadme.write('Time now (run end): ' + time.ctime(tend))
