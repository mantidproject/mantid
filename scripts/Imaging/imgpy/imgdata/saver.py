from __future__ import (absolute_import, division, print_function)
import os
import helper as h


def write_fits(data, filename, overwrite=False):
    from imgdata.loader import import_pyfits
    fits = import_pyfits()
    hdu = fits.PrimaryHDU(data)
    hdulist = fits.HDUList([hdu])
    hdulist.writeto(filename, clobber=overwrite)


def write_img(data, filename, overwrite=False):
    from imgdata.loader import import_skimage_io
    skio = import_skimage_io()
    skio.imsave(filename, data)


def write_nxs(data, filename, projection_angles=None, overwrite=False):
    """
    Write a h5 NXS file.

    :param data: the sample data that will be saved
    :param filename: The full path of the output file, plus the name.
    :param flat: An averaged flat image MUST BE PROVIDED
    :param dark: An averaged flat image MUST BE PROVIDED
    :param projection_angles: Pre-calculated projection angles for the stack.
    :param overwrite: Overwrite an existing file.
    """

    import numpy as np
    import h5py
    nxs = h5py.File(filename, 'w')

    # appending flat and dark images is disabled for now
    # new shape to account for appending flat and dark images
    # correct_shape = (data.shape[0] + 2, data.shape[1], data.shape[2])

    dset = nxs.create_dataset("tomography/sample_data", data.shape)
    dset[:data.shape[0]] = data[:]
    # dset[-2] = flat[:]
    # dset[-1] = dark[:]

    if projection_angles is not None:
        rangle = nxs.create_dataset(
            "tomography/rotation_angle", data=projection_angles)
        rangle[...] = projection_angles


class Saver(object):
    """
    This class doesn't have any try: ... except: ... because when called
    it's usually at an end point, where there would be no point in recovering.

    However if the directory in which the output should be written out
    does not exist, it will be created on the first call of make_dirs_if_needed.
    And if the directory cannot be created/accessed/written to afterwards, it will fail on
    writing out the Readme Summary beginning, which is early in the reconstruction,
    before loading any data in, or even the tool.

    That means this class should always fail early before any
    expensive operations have been attempted.
    """

    @staticmethod
    def supported_formats():
        # reuse supported formats, they currently share them
        from imgdata.loader import supported_formats
        return supported_formats()

    def __init__(self, config):

        self._output_path = config.func.output_path
        if self._output_path is not None:
            self._output_path = os.path.abspath(
                os.path.expanduser(self._output_path))

        self._img_format = config.func.out_format
        self._overwrite_all = config.func.overwrite_all
        self._swap_axes = config.func.swap_axes

        self._preproc_dir = config.func.preproc_subdir
        self._save_preproc = config.func.save_preproc

        self._out_slices_prefix = config.func.out_slices_prefix
        self._out_horiz_slices_prefix = config.func.out_horiz_slices_prefix
        self._out_horiz_slices_subdir = config.func.out_horiz_slices_subdir
        self._save_horiz_slices = config.func.save_horiz_slices

    def get_output_path(self):
        return self._output_path

    def save_single_image(self, data, subdir=None, name='saved_image', swap_axes=False, custom_index=None, zfill_len=0,
                          name_postfix='', use_preproc_folder=True):
        """
        Save a single image to a single image file.
        THIS SHOULD NOT BE USED WITH A 3D STACK OF IMAGES.
        
        :param subdir :: additional output directory, currently used for debugging
        :param data :: data volume with pre-processed images
        :param name: image name to be appended
        :param custom_index: parameter number that will be appended to the image filename3
        :param zfill_len: When saving multiple images, how much zeros to put after the name and before the index number.
                          E.g. saving out an image with zfill_len = 6: saved_image000001,...saved_image000201 and so on
                          E.g. saving out an image with zfill_len = 3: saved_image001,...saved_image201 and so on
        :param name_postfix: String to be appended after the zero fill. This is not recommended and might confuse
                              imaging programs (including this script) as to the order of the images, and they could
                              end up not loading all of the images.
        """

        if self._output_path is None:
            h.tomo_print_note(
                "Not saving a single image, because no output path is specified."
            )
            return

        # using the config's output dir
        if use_preproc_folder:
            output_dir = os.path.join(self._output_path, self._preproc_dir)
        else:
            output_dir = self._output_path

        if subdir is not None:
            # using the provided subdir
            output_dir = os.path.join(output_dir, subdir)
            output_dir = os.path.abspath(output_dir)

        h.pstart("Saving single image {0} dtype: {1}".format(output_dir,
                                                             data.dtype))

        self.save(
            data,
            output_dir,
            name,
            swap_axes,
            img_format=self._img_format,
            overwrite_all=self._overwrite_all,
            zfill_len=zfill_len,
            name_postfix=name_postfix,
            custom_idx=custom_index)

        h.pstop("Finished saving single image.")

    def save_preproc_images(self, data):
        """
        Specialised save function to save out the pre-processed images.
        
        This will save the images out in a subdir /pre-processed/.

        :param data: The pre-processed data that will be saved
        :param flat: The averaged flat image
        :param dark: The averaged dark image
        """

        if self._save_preproc and self._output_path is not None:
            preproc_dir = os.path.join(self._output_path, self._preproc_dir)

            h.pstart("Saving all pre-processed images into {0} dtype: {1}".
                     format(preproc_dir, data.dtype))

            self.save(data, preproc_dir, 'out_preproc_image', self._swap_axes,
                      self._img_format, self._overwrite_all)

            h.pstop("Saving pre-processed images finished.")

    def save_recon_output(self, data):
        """
        Specialised method to save out the reconstructed data 
        depending on the configuration options.

        This will save out the data in a subdirectory /reconstructed/.
        If --save-horiz-slices is specified, the axis will be flipped and 
        the result will be saved out in /reconstructed/horiz.
        ^ This means that the data usage will double as numpy.swapaxes WILL COPY the data!
        However this is not usually a problem as the reconstructed data is small.

        :param data :: reconstructed data volume. A sequence of images will be saved from this
        slices saved by default. Useful for testing some tools
        """
        if self._output_path is None:
            h.tomo_print_warning(
                "Not saving reconstruction output, because no output path is specified."
            )
            return

        out_recon_dir = os.path.join(self._output_path, 'reconstructed')

        h.pstart(
            "Starting saving slices of the reconstructed volume in: {0}...".
            format(out_recon_dir))

        self.save(data, out_recon_dir, self._out_slices_prefix, self._swap_axes,
                  self._img_format, self._overwrite_all)

        # Sideways slices:
        if self._save_horiz_slices:
            # try np swapaxis and save that !
            out_horiz_dir = os.path.join(out_recon_dir,
                                         self._out_horiz_slices_subdir)

            h.tomo_print_note(
                "Saving horizontal slices in: {0}".format(out_horiz_dir))

            import numpy as np
            # save out the horizontal slices by flipping the axes
            self.save(data, out_horiz_dir, self._out_horiz_slices_prefix,
                      not self._swap_axes, self._img_format, self._overwrite_all)

        h.pstop("Finished saving slices of the reconstructed volume in: {0}".
                format(out_recon_dir))

    @staticmethod
    def save(data,
             output_dir,
             name_prefix,
             swap_axes=False,
             img_format='tiff',
             overwrite_all=False,
             custom_idx=None,
             zfill_len=6,
             name_postfix=''):
        """
        Save image volume (3d) into a series of slices along the Z axis.
        The Z axis in the script is the ndarray.shape[0].

        :param data: data as images/slices stores in numpy array
        :param output_dir: where to save the files
        :param name_prefix: prefix for the names of the images - an index is appended to this prefix
        :param overwrite_all: Overwrite any existing images with conflicting names

        """

        Saver.make_dirs_if_needed(output_dir, overwrite_all)

        import numpy as np

        if swap_axes:
            data = np.swapaxes(data, 0, 1)

        if img_format in ['nxs']:
            filename = os.path.join(output_dir, name_prefix + name_postfix)
            write_nxs(data, filename + '.nxs', overwrite=overwrite_all)
        else:
            if img_format in ['fit', 'fits']:
                write_func = write_fits
            else:
                # pass all other formats to skimage
                write_func = write_img

            h.prog_init(data.shape[0], "Saving " + img_format + " images")
            for idx in range(0, data.shape[0]):
                # use the custom index if one is provided
                index = custom_idx if custom_idx is not None else str(
                    idx).zfill(zfill_len)
                # create the file name, and use the format as extension
                name = name_prefix + index + name_postfix + "." + img_format

                write_func(data[idx, :, :],
                           os.path.join(output_dir, name), overwrite_all)
                h.prog_update()
            h.prog_close()

    @staticmethod
    def make_dirs_if_needed(dirname=None, overwrite_all=False):
        """
        Makes sure that the directory needed (for example to save a file)
        exists, otherwise creates it.

        :param dirname :: (output) directory to check
        """
        if dirname is None:
            return

        path = os.path.abspath(os.path.expanduser(dirname))

        if not os.path.exists(path):
            os.makedirs(path)
        elif os.listdir(path) and not overwrite_all:
            raise RuntimeError(
                "The output directory is NOT empty:{0}\n. This can be overridden with -w/--overwrite-all.".
                format(path))
