from __future__ import (absolute_import, division, print_function)
import os


class Saver(object):

    def __init__(self, config):
        from recon.helper import Helper
        self._h = Helper(config)

        self._output_path = os.path.abspath(os.path.expanduser(config.func.output_path))

        self._overwrite_all = config.func.overwrite_all
        self._data_as_stack = config.func.data_as_stack

        self._readme_fullpath = os.path.join(
            self._output_path, config.func.readme_file_name)

        self._preproc_dir = config.func.preproc_subdir
        self._save_preproc = config.func.save_preproc

        self._out_slices_file_name_prefix = config.func.out_slices_file_name_prefix
        self._out_horiz_slices_subdir = config.func.out_horiz_slices_subdir
        self._save_horiz_slices = config.func.save_horiz_slices

    def save_single_image(self, data,
                          subdir=None,
                          image_name='saved_image',
                          image_index=0):
        """
        Save (pre-processed) images from a data array to image files.
        :param subdir :: additional output directory, currently used for debugging
        :param data :: data volume with pre-processed images
        :param image_name: image name to be appended
        :param image_index: image index to be appended
        """

        # using the config's output dir
        preproc_dir = os.path.join(self._output_path, self._preproc_dir)

        if subdir is not None:
            # using the provided subdir
            preproc_dir = os.path.join(preproc_dir, subdir)
            preproc_dir = os.path.abspath(preproc_dir)

        self._h.pstart(
            "Saving single image {0} dtype: {1}".format(preproc_dir, data.dtype))

        self.make_dirs_if_needed(preproc_dir)

        self.write_image(data, os.path.join(
            preproc_dir, image_name + str(image_index).zfill(6)))

        self._h.pstop("Finished saving single image.")

    def save_recon_output(self, data):
        """
        Save output reconstructed volume in different forms.

        :param data :: reconstructed data volume. A sequence of images will be saved from this
        :param config :: configuration of the reconstruction, including output paths and formats
        slices saved by default. Useful for testing some tools
        """
        # slices along the vertical (z) axis
        # output_path = 'output_recon_tomopy'

        out_recon_dir = os.path.join(self._output_path, 'reconstructed')

        self._h.pstart(
            "Starting saving slices of the reconstructed volume in: {0}...".format(out_recon_dir))

        self.save_recon_as_vertical_slices(
            data, out_recon_dir, self._out_slices_file_name_prefix, self._data_as_stack)

        # Sideways slices:
        if self._save_horiz_slices:
            out_horiz_dir = os.path.join(out_recon_dir, 'horiz_slices')
            self._h.tomo_print(
                "Saving horizontal slices in: {0}".format(out_horiz_dir))
            self.save_recon_as_horizontal_slices(
                data, out_horiz_dir, self._out_horiz_slices_subdir)

        self._h.pstop(
            "Finished saving slices of the reconstructed volume in: {0}".
            format(out_recon_dir))

    def save_recon_as_vertical_slices(self, data, output_dir, name_prefix='out_recon_slice', data_as_stack=False):
        """
        Save reconstructed volume (3d) into a series of slices along the Z axis (outermost numpy dimension)
        :param data_as_stack:
        :param data :: data as images/slices stores in numpy array
        :param output_dir :: where to save the files
        :param name_prefix :: prefix for the names of the images - an index is appended to this prefix
        :param overwrite_all: Overwrite any existing images with conflicting names

        """

        self.make_dirs_if_needed(output_dir)
        if not data_as_stack:
            for idx in range(0, data.shape[0]):
                self.write_image(data[idx, :, :], os.path.join(
                    output_dir, name_prefix + str(idx).zfill(6)))
        else:
            self.write_image(data, os.path.join(
                output_dir, name_prefix + "_stack".zfill(6)))

    def save_recon_as_horizontal_slices(self, data, out_horiz_dir, name_prefix='out_recon_horiz_slice'):
        """
        Save reconstructed volume (3d) into a series of slices along the Y axis (second numpy dimension)
        :param data :: data as images/slices stores in numpy array
        :param out_horiz_dir :: where to save the files
        :param name_prefix :: prefix for the names of the images throughout the horizontal axis
        :param overwrite_all: Overwrite any existing images with conflicting names
        the prefix
        """

        self.make_dirs_if_needed(out_horiz_dir)

        # TODO find out how to flip with numpy
        # if not config.func.data_as_stack:
        for idx in range(0, data.shape[1]):
            self.write_image(data[:, idx, :], os.path.join(
                out_horiz_dir, name_prefix + str(idx).zfill(6)))
            # else:
            # write_image(data[:, idx, :], os.path.join(
            # out_horiz_dir, name_prefix + str(idx).zfill(6)))

    def save_preproc_images(self, data):
        """
        Save (pre-processed) images from a data array to image files.

        :param data :: The pre-processed data that will be saved
        :param config :: The full reconstruction config
        """

        if not self._save_preproc:
            self._h.tomo_print_warning(
                "NOT saving out pre-processed images, because no -s/--save-preproc was specified.")
            return

        preproc_dir = os.path.join(self._output_path, self._preproc_dir)

        self._h.pstart(
            "Saving all pre-processed images into {0} dtype: {1}".format(preproc_dir, data.dtype))

        self.make_dirs_if_needed(preproc_dir)

        if not self._data_as_stack:
            for idx in range(0, data.shape[0]):
                self.write_image(data[idx, :, :], os.path.join(
                    preproc_dir, 'out_preproc_proj_image' + str(idx).zfill(6)))
        else:
            self.write_image(data, os.path.join(
                preproc_dir, 'out_preproc_proj_images_stack'))

        self._h.pstop("Saving pre-processed images finished.")

    def write_image(self, img_data, filename):
        """
        Output image data, given as a numpy array, to a file, in a given image format.
        Assumes that the output directory exists (must be checked before). The pixel
        values are rescaled in the range [min_pix, max_pix] which would normally be set
        to the minimum/maximum values found in a stack of images.

        :param img_data :: image data in the usual numpy representation
        :param filename :: file name, including directory and extension
        of the input data is used
        :returns:: name of the file saved
        """

        from recon.data import loader

        # from skimage import exposure
        # img_data = exposure.rescale_intensity(img_data[:, :], out_range='uint16')

        fits = loader.import_pyfits()
        # to flip vertically img_data[::-1] ps. saved images are still flipped
        # vertically
        hdu = fits.PrimaryHDU(img_data)
        hdulist = fits.HDUList([hdu])
        hdulist.writeto(filename + ".fits", clobber=self._overwrite_all)

        return filename

    def gen_readme_summary_begin(self, cmd_line, config):
        """
        To write configuration, settings, etc. early on. As early as possible, before any failure
        can happen.

        :param cmd_line :: command line originally used to run this reconstruction, when running
        from the command line
        :param config :: the full reconstruction configuration so it can be dumped into the file

        Returns :: time now (begin of run) in number of seconds since epoch (time() time)
        """

        import time
        tstart = time.time()

        self.make_dirs_if_needed(self._output_path)

        self._h.pstart("Generating reconstruction script beginning...")

        # generate file with dos/windows line end for windows users convenience
        with open(self._readme_fullpath, 'w') as oreadme:
            file_hdr = (
                'Tomographic reconstruction. Summary of inputs, settings and outputs.\n'
                'Time now (run begin): ' + time.ctime(tstart) + '\n')
            oreadme.write(file_hdr)

            alg_hdr = ("\n"
                       "--------------------------\n"
                       "Tool/Algorithm\n"
                       "--------------------------\n")
            oreadme.write(alg_hdr)
            oreadme.write(str(config.func))
            oreadme.write("\n")

            preproc_hdr = ("\n"
                           "--------------------------\n"
                           "Pre-processing parameters\n"
                           "--------------------------\n")
            oreadme.write(preproc_hdr)
            oreadme.write(str(config.pre))
            oreadme.write("\n")

            postproc_hdr = ("\n"
                            "--------------------------\n"
                            "Post-processing parameters\n"
                            "--------------------------\n")
            oreadme.write(postproc_hdr)
            oreadme.write(str(config.post))
            oreadme.write("\n")

            cmd_hdr = ("\n"
                       "--------------------------\n"
                       "Command line\n"
                       "--------------------------\n")
            oreadme.write(cmd_hdr)
            oreadme.write(cmd_line)
            oreadme.write("\n")

        self._h.pstop("Finished generating script beginning.")

    def make_dirs_if_needed(self, dirname):
        """
        Makes sure that the directory needed (for example to save a file)
        exists, otherwise creates it.

        :param dirname :: (output) directory to check
        """

        absname = os.path.abspath(dirname)
        if not os.path.exists(absname):
            os.makedirs(absname)
        elif os.listdir(absname) and not self._overwrite_all:
            raise RuntimeError(
                "The output directory is NOT empty! This can be overridden with -w/--overwrite-all")

    def gen_readme_summary_end(self, data_stages, tstart,
                               t_recon_elapsed):
        """
        Write last part of report in the output readme/report file. This should be used whenever a
        reconstruction runs correctly.

        :param data_stages :: tuple with data in three stages (raw, pre-processed, reconstructed)
        :param tstart :: time at the beginning of the job/reconstruction, when the first part of the
        readme file was written
        :param t_recon_elapsed :: reconstruction time
        """

        pass

        # append to a readme/report that should have been pre-filled with the
        # initial configuration
        with open(self._readme_fullpath, 'a') as oreadme:
            import time
            run_hdr = ("\n"
                       "--------------------------\n"
                       "Run/job details:\n"
                       "--------------------------\n")
            oreadme.write(run_hdr)
            (raw_data, preproc_data, data) = data_stages

            oreadme.write("Dimensions of raw input sample data: {0}\n".format(
                raw_data.shape))
            oreadme.write("Dimensions of pre-processed sample data: {0}\n".
                          format(preproc_data.shape))
            oreadme.write("Dimensions of reconstructed volume: {0}\n".format(
                data.shape))

            oreadme.write("Raw input pixel type: {0}\n".format(raw_data.dtype))
            oreadme.write("Output pixel type: {0}\n".format('uint16'))
            oreadme.write("Time elapsed in reconstruction: {0:.3f}s\r\n".
                          format(t_recon_elapsed))
            tend = time.time()
            oreadme.write('Time now (run end): ' + time.ctime(tend))
