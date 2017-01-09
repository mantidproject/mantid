from __future__ import (absolute_import, division, print_function)
# Copyright &copy; 2014,2015 ISIS Rutherford Appleton Laboratory, NScD
# Oak Ridge National Laboratory & European Spallation Source
#
# This file is part of Mantid.
# Mantid is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# Mantid is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# File change history is stored at: <https://github.com/mantidproject/mantid>.
# Code Documentation is available at: <http://doxygen.mantidproject.org>

import os
import time
import numpy as np

# For 3d median filter
try:
    import scipy
except ImportError:
    raise ImportError(
        "Could not find the package scipy which is required for image pre-/post-processing"
    )

try:
    import scipy.ndimage
except ImportError:
    raise ImportError(
        "Could not find the subpackage scipy.ndimage, required for image pre-/post-processing"
    )

from . import io as tomoio
from . import configs as tomocfg


class ReconstructionCommand(object):
    """
    Run a tomographic reconstruction command, which can be a local process or a job on a
    remote machine/scheduler. This class provides functionality to pre-process a raw dataset,
    run a reconstruction (from pre-processed data to reconstructed volume), and post-process
    a reconstructed volume.
    """

    def __init__(self):
        self._PREPROC_IMGS_SUBDIR_NAME = 'pre_processed'
        self._OUT_README_FNAME = '0.README_reconstruction.txt'
        self._OUT_SLICES_FILENAME_PREFIX = 'out_recon_slice'
        self._OUT_HORIZ_SLICES_SUBDIR = 'out_recon_horiz_slice'

        self.preproc_cfg = tomocfg.PreProcConfig()
        self.alg_cfg = tomocfg.ToolAlgorithmConfig
        self.postproc_cfg = tomocfg.PostProcConfig()

        self._timer_running = False
        self._whole_exec_timer = False

        # whether to crop before applying normalization steps. If True, the air region cannot be
        # outside of the region of interest. Leaving as False by default, and
        # not exposing this option.
        self.__class__.crop_before_normaliz = False

    def _debug_print_memory_usage_linux(self, message=""):
        try:
            # Windows doesn't seem to have resouce package, so this will
            # silently fail
            import resource
            print(" >> Memory usage " +
                  str(resource.getrusage(resource.RUSAGE_SELF).ru_maxrss) +
                  " KB, " + str(
                      int(resource.getrusage(resource.RUSAGE_SELF).ru_maxrss) /
                      1024) + " MB", message)
        except Exception:
            pass

    def tomo_print(self, message, priority=1):
        """
        TODO currently the priority parameter is ignored
        Verbosity levels:
        0 -> debug, print everything
        1 -> information, print information about progress
        2 -> print only major progress information, i.e data loaded, recon started, recon finished

        Print only messages that have priority >= config verbosity level

        :param message: Message to be printed
        :param priority: Importance level depending on which messages will be printed
        :return:
        """

        #  should be moved in the configs somewhere
        temp_verbosity = 0
        if priority >= temp_verbosity:
            print(message)

    def tomo_print_timed_start(self, message, priority=1):
        """
        On every second call this will terminate and print the timer
        TODO currently the priority parameter is ignored

        Verbosity levels:
        0 -> debug, print everything
        1 -> information, print information about progress
        2 -> print only major progress information, i.e data loaded, recon started, recon finished

        Print only messages that have priority >= config verbosity level

        :param message: Message to be printed
        :param priority: Importance level depending on which messages will be printed
        :return:
        """

        import time

        #  should be moved in the configs somewhere
        temp_verbosity = 1
        print_string = ""

        if not self._timer_running:
            self._timer_running = True
            self._timer_start = time.time()
            print_string = message

        if priority >= temp_verbosity:
            print(print_string)

    def tomo_print_timed_stop(self, message, priority=1):
        """
        On every second call this will terminate and print the timer. This will append ". " to the string
        TODO currently the priority parameter is ignored

        Verbosity levels:
        0 -> debug, print everything
        1 -> information, print information about progress
        2 -> print only major progress information, i.e data loaded, recon started, recon finished

        Print only messages that have priority >= config verbosity level

        :param message: Message to be printed
        :param priority: Importance level depending on which messages will be printed
        :return:
        """

        import time

        #  should be moved in the configs somewhere
        temp_verbosity = 1
        print_string = ""

        if self._timer_running:
            self._timer_running = False
            timer_string = str(time.time() - self._timer_start)
            print_string = message + " Elapsed time: " + timer_string + " sec"

        if priority >= temp_verbosity:
            print(print_string)

    def tomo_total_timer(self, message="Total execution time was "):
        """
        This will ONLY be used to time the WHOLE execution time.
        The first call to this will be in tomo_reconstruct.py and it will start it.abs
        The last call will be at the end of find_center or do_recon.
        """
        import time

        if not self._whole_exec_timer:
            # change type from bool to timer
            self._whole_exec_timer = time.time()
        else:
            # change from timer to string
            self._whole_exec_timer = str(time.time() - self._whole_exec_timer)
            print(message + self._whole_exec_timer + " sec")

    def _check_paths_integrity(self, cfg):
        if not cfg or not isinstance(cfg, tomocfg.ReconstructionConfig):
            raise ValueError(
                "Cannot run a reconstruction without a valid configuration")

        if not cfg.preproc_cfg.input_dir:
            raise ValueError(
                "Cannot run a reconstruction without setting the input path")

        if not cfg.postproc_cfg.output_dir:
            raise ValueError(
                "Cannot run a reconstruction without setting the output path")

    def do_recon(self, cfg, cmd_line=None):
        """
        Run a reconstruction using a particular tool, algorithm and setup

        :param preproc_cfg :: configuration (pre-processing + tool+algorithm + post-processing)

        :param cmd_line :: command line text if running from the CLI. When provided it will
        be written in the output readme file(s) for reference.
        """
        self._check_paths_integrity(cfg)

        if not cfg or not isinstance(cfg, tomocfg.ReconstructionConfig):
            raise ValueError(
                "Cannot run a reconstruction without a valid configuration")

        if not cfg.preproc_cfg.input_dir:
            raise ValueError(
                "Cannot run a reconstruction without setting the input path")

        if not cfg.postproc_cfg.output_dir:
            raise ValueError(
                "Cannot run a reconstruction without setting the output path")

        # First step import the tool
        self.tomo_print_timed_start(" * Importing tool " + cfg.alg_cfg.tool)
        # import the tool
        import tomorec.tool_imports as tti
        reconstruction_tool = tti.import_tomo_tool(cfg.alg_cfg.tool)

        self.tomo_print_timed_stop(" * Tool loaded.")

        # ----------------------------------------------------------------

        self.tomo_print_timed_start(" * Generating reconstruction script...")
        readme_fullpath = os.path.join(cfg.postproc_cfg.output_dir,
                                       self._OUT_README_FNAME)
        tstart = self.gen_readme_summary_begin(readme_fullpath, cfg, cmd_line)
        self.tomo_print_timed_stop(" * Finished generating script.")

        # ----------------------------------------------------------------

        self.tomo_print_timed_start(" * Loading data...")
        data, flat, dark = self.read_in_stack(
            cfg.preproc_cfg.input_dir, cfg.preproc_cfg.in_img_format,
            cfg.preproc_cfg.input_dir_flat, cfg.preproc_cfg.input_dir_dark)
        self.tomo_print_timed_stop(
            " * Data loaded. Shape of raw data: {0}, dtype: {1}.".format(
                data.shape, data.dtype))

        # ----------------------------------------------------------------

        preproc_data = self.apply_all_preproc(data, cfg, flat, dark)

        # ----------------------------------------------------------------

        # Save pre-proc images, print inside
        self.save_preproc_images(preproc_data, cfg.postproc_cfg.output_dir,
                                 cfg.preproc_cfg)

        # ----------------------------------------------------------------

        return
        # Reconstruction
        # for file readme summary
        t_recon_start = time.time()
        recon_data = self.run_reconstruct_3d(preproc_data, cfg.preproc_cfg,
                                             cfg.alg_cfg, reconstruction_tool)
        t_recon_end = time.time()

        # ----------------------------------------------------------------

        # Post-processing
        recon_data = self.apply_postproc_filters(recon_data, cfg.postproc_cfg)

        # Save output from the reconstruction
        self.save_recon_output(recon_data, cfg)

        # turned off for now, as it can't be opened from ParaView so it's a
        # waste
        save_netcdf_vol = False
        if save_netcdf_vol:
            self.tomo_print_timed_start(
                " * Saving reconstructed volume as NetCDF...")
            tomoio.save_recon_netcdf(recon_data, cfg.postproc_cfg.output_dir)
            self.tomo_print_timed_stop(
                " * Finished saving reconstructed volume as NetCDF.")

        self.gen_readme_summary_end(readme_fullpath,
                                    (data, preproc_data, recon_data), tstart,
                                    t_recon_end - t_recon_start)

    def gen_readme_summary_begin(self, filename, cfg, cmd_line):
        """
        To write configuration, settings, etc. early on. As early as possible, before any failure
        can happen.

        :param filename :: name of the readme/final report file
        :param cfg :: full reconstruction configuration
        :param cmd_line :: command line originally used to run this reconstruction, when running
        from the command line

        Returns :: time now (begin of run) in number of seconds since epoch (time() time)
        """
        tstart = time.time()

        # generate file with dos/windows line end for windoze users'
        # convenience
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
            oreadme.write(str(cfg.preproc_cfg))
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

        return tstart

    def gen_readme_summary_end(self, filename, data_stages, tstart,
                               t_recon_elapsed):
        """
        Write last part of report in the output readme/report file. This should be used whenever a
        reconstruction runs correctly.

        :param filename :: name of the readme/final report file
        :param data_stages :: tuple with data in three stages (raw, pre-processed, reconstructed)
        :param tstart :: time at the beginning of the job/reconstruction, when the first part of the
        readme file was written
        :param t_recon_elapsed :: reconstruction time
        """
        # append to a readme/report that should have been pre-filled with the
        # initial configuration
        with open(filename, 'a') as oreadme:
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
            oreadme.write("Total time elapsed: {0:.3f}s\r\n".format(tend -
                                                                    tstart))
            oreadme.write('Time now (run end): ' + time.ctime(tend))

    def apply_all_preproc(self, data, cfg, flat, dark):
        """
        Do all the pre-processing. This does all that is needed between a)
        loading the input data, and b) starting a reconstruction
        run/job. From raw inputs to pre-proc data that is ready to go for
        reconstruction.

        :param data :: raw data (sample projection images)
        :param cfg :: the whole filter configuration
        :param flat :: flat / flat / open-beam image for normalization in some of the first
        pre-processing steps
        :param dark :: dark image for normalization

        Returns :: pre-processed data.

        """
        self._check_data_stack(data)

        self.tomo_print(" * Beginning pre-processing with pixel data type: " +
                        str(data.dtype))
        preproc_data = self.apply_prep_filters(data, cfg.preproc_cfg, flat,
                                               dark)
        # preproc_data = self.apply_line_projection(
        #     preproc_data, cfg.preproc_cfg)

        # pass the whole config because we need to tool's name
        # preproc_data = self.apply_final_preproc_corrections(preproc_data,
        #                                                     cfg)

        return preproc_data

    def apply_prep_filters(self, data, preproc_cfg, flat, dark):
        """
        Apply the normal initial pre-processing filters, including simple
        operations as selecting/cropping to the region-of-interest,
        normalization, etc. If the images need to be rotated this is
        done as a first step (so all intermediate pre-processed
        results will be rotated as required).

        :param data :: projection images data, as 3d numpy array, with images along outermost (z)
        dimension

        :param preproc_cfg :: pre-processing configuration
        :param flat :: white/flat/open-beam image for normalization
        :param dark :: dark image for normalization

        Returns :: process/filtered data (sizes can change (cropped) and data can be rotated). 
        The flat and dark images are discarded after the pre-processing

        """
        self._check_data_stack(data)

        if 'float64' == data.dtype:
            # this is done because tomoio.write has problems with float64 to
            # int16
            data = data.astype(dtype='float32')
            # print with top priority
            self.tomo_print(
                " * Note: pixel data type changed to: " + data.dtype,
                priority=2)

        data, flat, dark = self.rotate_stack(data, preproc_cfg, flat, dark)

        # if preproc_cfg.crop_before_normalize:
        #     data, flat, dark = self.crop_coords(data, preproc_cfg, flat, dark)

        # NOTE flats and darks are cropped inside normalize_flat_dark
        data = self.normalize_flat_dark(data, preproc_cfg, flat, dark)
        data = self.normalize_air_region(data, preproc_cfg)

        # if not preproc_cfg.crop_before_normalize:
        #     data, flat, dark = self.crop_coords(data, preproc_cfg, flat, dark)

        data = self.apply_cut_off_and_others(data, preproc_cfg)
        data = self.median_filter(data, preproc_cfg)

        # discarding the flat and dark data here
        return data

    def apply_line_projection(self, imgs_angles, preproc_cfg):
        """
        Transform pixel values as $- ln (Is/I0)$, where $Is$ is the pixel (intensity) value and $I0$ is a
        reference value (pixel/intensity value for open beam, or maximum in the stack or the image).

        This produces a projection image, $ p(s) = -ln\\frac{I(s)}{I(0)} $,
        with $ I(s) = I(0) e^{-\\int_0^s \\mu(x)dx} $
        where:
        $p(s)$ represents the sum of the density of objects along a line (pixel) of the beam
        I(0) initital intensity of netron beam (white images)
        I(s) neutron count measured by detector/camera

        The integral is the density along the path through objects.
        This is required for example when pixels have neutron count values.

        :param imgs_angles :: stack of images (angular projections) as 3d numpy array. Every image will be
        processed independently, using as reference intensity the maximum pixel value found across all the
        images.

        :param preproc_cfg :: pre-processing configuration set up for a reconstruction

        Returns :: projected data volume (image stack)
        """
        self._check_data_stack(imgs_angles)

        if not preproc_cfg.line_projection:
            self.tomo_print(
                " * Note: NOT applying line projection.", priority=2)
            return imgs_angles

        self.tomo_print_timed_start(
            " * Starting to apply line projection on {0} images...".format(
                imgs_angles.shape[0]))
        imgs_angles = imgs_angles.astype('float32')
        for idx in range(0, imgs_angles.shape[0]):
            max_img = np.amax(imgs_angles[idx, :, :])
            to_log = np.true_divide(imgs_angles[idx, :, :], max_img)
            if False:
                print(
                    "   Initial image max: {0}. Transformed to log scale, min: {1}, max: {2}.".
                    format(max_img, np.amin(to_log), np.amax(to_log)))
            imgs_angles[idx, :, :] = -np.log(to_log + 1e-6)

        self.tomo_print_timed_start(
            " * Finished applying line projection on {0} images. ".format(
                imgs_angles.shape[0]))

        return imgs_angles

    def apply_final_preproc_corrections(self, preproc_data, cfg):
        """
        Apply additional, optional, pre-processing steps/filters.

        :param preproc_data :: input data as a 3d volume (projection images)
        :param cfg :: pre-processing configuration

        Returns :: filtered data (stack of images)
        """
        self._check_data_stack(preproc_data)

        import tomorec.tool_imports as tti
        tomopy = tti.import_tomo_tool(cfg.alg_cfg.tool)

        # Remove stripes in sinograms / ring artefacts in reconstructed volume
        preproc_cfg = cfg.preproc_cfg
        if preproc_cfg.stripe_removal_method:
            import prep as iprep
            if 'wavelet-fourier' == preproc_cfg.stripe_removal_method.lower():
                self.tomo_print_timed_start(
                    " * Starting removal of stripes/ring artifacts using the method '{0}'...".
                    format(preproc_cfg.stripe_removal_method))

                # preproc_data = tomopy.prep.stripe.remove_stripe_fw(preproc_data)
                preproc_data = iprep.filters.remove_stripes_ring_artifacts(
                    preproc_data, 'wavelet-fourier')

                self.tomo_print_timed_start(
                    " * Finished removal of stripes/ring artifacts.")

            elif 'titarenko' == preproc_cfg.stripe_removal_method.lower():
                self.tomo_print_timed_start(
                    " * Starting removal of stripes/ring artifacts, using the method '{0}'...".
                    format(preproc_cfg.stripe_removal_method))

                preproc_data = tomopy.prep.stripe.remove_stripe_ti(
                    preproc_data)

                self.tomo_print_timed_stop(
                    " * Finished removal of stripes/ring artifacts.")
            else:
                self.tomo_print(
                    " * WARNING: stripe removal method '{0}' is unknown. Not applying it.".
                    format(preproc_cfg.stripe_removal_method),
                    priority=2)
        else:
            self.tomo_print(
                " * Note: NOT applying stripe removal.", priority=2)

        # Experimental options, disabled and not present in the config objects for now
        # These and related algorithms needs more evaluation/benchmarking
        if False:
            self.tomo_print_timed_start(" * Starting adjust range...")
            preproc_data = tomopy.misc.corr.adjust_range(preproc_data)
            self.tomo_print_timed_stop(" * Finished adjusting range.")

        if False:
            self.tomo_print_timed_start(
                " * Starting background normalisation...")

            preproc_data = tomopy.prep.normalize.normalize_bg(
                preproc_data, air=5)
            self.tomo_print_timed_stop(" * Finished background normalisation.")

        return preproc_data

    def crop_coords(self, sample, preproc_cfg, flat=None, dark=None):
        """
        Crop stack of images to a region (region of interest or similar), image by image

        :param sample :: stack of images as a 3d numpy array
        :param preproc_cfg :: pre-processing configuration
        :param flat :: the average flat image
        :param dark :: the average dark image

        Returns :: filtered data (stack of images)
        """
        self._check_data_stack(sample)

        # list with [left, top, right, bottom]
        if preproc_cfg.crop_coords:
            try:
                self.tomo_print_timed_start(
                    " * Starting image cropping step, with pixel data type: {0}, coordinates: {1}. ...".
                    format(sample.dtype, preproc_cfg.crop_coords))

                import prep as iprep
                sample = iprep.filters.crop_vol(sample,
                                                preproc_cfg.crop_coords)

                left = preproc_cfg.crop_coords[0]
                top = preproc_cfg.crop_coords[1]
                right = preproc_cfg.crop_coords[2]
                bottom = preproc_cfg.crop_coords[3]

                if isinstance(flat, np.ndarray):
                    flat = flat[top:bottom, left:right]
                    self.save_single_image(
                        flat,
                        preproc_cfg,
                        scale_factor=1,
                        image_name='cropped_flat')

                if isinstance(dark, np.ndarray):
                    dark = dark[top:bottom, left:right]
                    self.save_single_image(
                        dark,
                        preproc_cfg,
                        scale_factor=1,
                        image_name='cropped_dark')

                self.tomo_print_timed_stop(
                    " * Finished image cropping step, with pixel data type: {0}, coordinates: {1}. "
                    "Resulting shape: {2}.".format(
                        sample.dtype, preproc_cfg.crop_coords, sample.shape))

            except ValueError as exc:
                print(
                    "Error in crop (region of interest) parameter (expecting a list with four integers. "
                    "Got: {0}. Error details: ".format(
                        preproc_cfg.crop_coords), exc)
        else:
            self.tomo_print(
                " * Note: NOT applying cropping to region of interest.",
                priority=2)

        return sample, flat, dark

    def normalize_flat_dark(self,
                            sample,
                            preproc_cfg,
                            norm_flat_img,
                            norm_dark_img=0):
        """
        Normalize by flat and dark images

        :param sample :: image stack as a 3d numpy array
        :param preproc_cfg :: pre-processing configuration
        :param norm_flat_img :: flat (open beam) image to use in normalization
        :param norm_dark_img :: dark image to use in normalization

        Returns :: filtered data (stack of images)
        """
        self._check_data_stack(sample)

        if not preproc_cfg or not isinstance(preproc_cfg,
                                             tomocfg.PreProcConfig):
            raise ValueError(
                "Cannot normalize by flat/dark images without a valid pre-processing "
                "configuration")

        if not preproc_cfg.normalize_flat_dark:
            self.tomo_print(
                " * Note: NOT applying normalization by flat/dark images.",
                priority=2)
            return sample

        if isinstance(norm_flat_img, np.ndarray):

            if 2 != len(norm_flat_img.
                        shape) or norm_flat_img.shape != sample.shape[1:]:
                raise ValueError(
                    "Incorrect shape of the flat image ({0}) which should match the "
                    "shape of the sample images ({1})".format(
                        norm_flat_img.shape, sample[0].shape))

            self.tomo_print_timed_start(
                " * Starting normalization by flat/dark images with pixel data type: {0}...".
                format(sample.dtype))

            norm_divide = None

            if isinstance(norm_dark_img, np.ndarray):
                # normalise the flats by subtracting the dark images background
                norm_divide = norm_flat_img - norm_dark_img
            else:
                norm_divide = norm_flat_img
                # set to 0 to not subtract anything as background

            # prevent divide-by-zero issues by setting to a very small number
            norm_divide[norm_divide == 0] = 1e-6

            self.save_single_image(
                norm_dark_img,
                preproc_cfg,
                scale_factor=1,
                image_name='dark_bg')
            self.save_single_image(
                norm_divide,
                preproc_cfg,
                scale_factor=1,
                image_name='flat_with_subtracted_bg')

            # normalise the sample images by subtracting the dark images background
            # and then dividing by the background normalised flat images
            # true_divide produces float64, we assume that precision,
            # hence why we're not casting the input images to floats

            max_val = np.finfo(sample.dtype).max
            for idx in range(0, sample.shape[0]):
                sample[idx, :, :] = np.true_divide(
                    sample[idx, :, :] - norm_dark_img, norm_divide)

            self.tomo_print_timed_stop(
                " * Finished normalization by flat/dark images with pixel data type: {0}.".
                format(sample.dtype))
        else:
            self.tomo_print(
                " * Note: cannot apply normalization by flat/dark images because no valid flat image has been "
                "provided in the inputs. Flat image given: {0}".format(
                    norm_flat_img),
                priority=2)

        return sample

    def normalize_air_region(self, data, pre_cfg):
        """
        TODO move to filters.py
        Normalize by beam intensity. This is not directly about proton
        charg - not using the proton charge field as usually found in
        experiment/nexus files. This uses an area of normalization, if
        provided in the pre-processing configuration. TODO: much
        of this method should be moved into filters.

        :param data :: stack of images as a 3d numpy array
        :param pre_cfg :: pre-processing configuration

        Returns :: filtered data (stack of images)

        """
        self._check_data_stack(data)

        if not pre_cfg or not isinstance(pre_cfg, tomocfg.PreProcConfig):
            raise ValueError(
                "Cannot normalize by air region without a valid pre-processing configuration"
            )

        if pre_cfg.normalize_air_region:
            if not isinstance(pre_cfg.normalize_air_region, list) or \
                            4 != len(pre_cfg.normalize_air_region):
                raise ValueError(
                    "Wrong air region coordinates when trying to use them to normalize images: {0}".
                    format(pre_cfg.normalize_air_region))

            if not all(
                    isinstance(crd, int)
                    for crd in pre_cfg.normalize_air_region):
                raise ValueError(
                    "Cannot use non-integer coordinates to use the normalization region "
                    "(air region). Got these coordinates: {0}".format(
                        pre_cfg.normalize_air_region))

            left = pre_cfg.normalize_air_region[0]
            top = pre_cfg.normalize_air_region[1]
            right = pre_cfg.normalize_air_region[2]
            bottom = pre_cfg.normalize_air_region[3]

            # skip if for example: 0, 0, 0, 0 (empty selection)
            if top >= bottom or left >= right:
                self.tomo_print(
                    " * NOTE: NOT applying Normalise by Air Region. Reason: Empty Selection"
                )
                return data

            self.tomo_print_timed_start(
                " * Starting normalization by air region...")
            air_sums = []
            for idx in range(0, data.shape[0]):
                air_data_sum = data[idx, top:bottom, left:right].sum()
                air_sums.append(air_data_sum)

            air_sums = np.true_divide(air_sums, np.amax(air_sums))

            for idx in range(0, data.shape[0]):
                data[idx, :, :] = np.true_divide(data[idx, :, :],
                                                 air_sums[idx])

            avg = np.average(air_sums)
            self.tomo_print_timed_stop(
                " * Finished normalization by air region. Statistics of values in the air region, "
                "average: {0}, max ratio: {1}, min ratio: {2}.".format(
                    avg, np.max(air_sums) / avg, np.min(air_sums) / avg))

        else:
            self.tomo_print(
                " * Note: NOT normalizing by air region", priority=2)

        return data

    def apply_cut_off_and_others(self, data, cfg):
        """
        Applies several pre-processing steps meant to be applied before
        the line projection transformation and final pre-processing on
        it. This includes at the moment: cut-off, and median
        filter. TODO: MCP corrections, scaling down, others.

        Returns :: filtered data (stack of images)

        """
        self._check_data_stack(data)

        if not cfg or not isinstance(cfg, tomocfg.PreProcConfig):
            raise ValueError(
                "Cannot apply cut-off without a valid pre-processing configuration"
            )

        # Apply cut-off for the normalization?
        if cfg.cut_off_level and cfg.cut_off_level:
            self.tomo_print_timed_start(
                "* Applying cut-off with level: {0}".format(cfg.cut_off_level))
            dmin = np.amin(data)
            dmax = np.amax(data)
            rel_cut_off = dmin + cfg.cut_off_level * (dmax - dmin)
            data[data < rel_cut_off] = dmin
            self.tomo_print_timed_stop(
                " * Finished cut-off step, with pixel data type: {0}.".format(
                    data.dtype))
        else:
            self.tomo_print(" * Note: NOT applying cut-off.", priority=2)

        if cfg.mcp_corrections:
            self.tomo_print(
                " * MCP corrections not implemented in this version",
                priority=2)

        if cfg.scale_down:
            self.tomo_print(
                " * Scale down not implemented in this version", priority=2)

        else:
            self.tomo_print(
                " * Note: NOT applying noise filter /median.", priority=2)

        return data

    def median_filter(self, data, preproc_cfg):

        self._check_data_stack(data)

        if preproc_cfg.median_filter_size and preproc_cfg.median_filter_size > 1:
            self.tomo_print_timed_start(
                " * Starting noise filter / median, with pixel data type: {0}, filter size/width: {1}.".
                format(data.dtype, preproc_cfg.median_filter_size))

            for idx in range(0, data.shape[0]):
                data[idx] = scipy.ndimage.median_filter(
                    data[idx], preproc_cfg.median_filter_size,
                    mode='mirror')  #, mode='nearest')
            # data = scipy.ndimage.median_filter(data, preproc_cfg.median_filter_size, mode='mirror')
            # , mode='nearest')

            self.tomo_print_timed_stop(
                " * Finished noise filter / median, with pixel data type: {0}, filter size/width: {1}.".
                format(data.dtype, preproc_cfg.median_filter_size))

        return data

    def rotate_stack(self, sample, cfg, flat=None, dark=None):
        """
        Rotates a stack (sample, white and dark images).
        This funciton is usually used on the whole picture, which is a square.
        If the picture is cropped first, the ROI coordinates
        have to be adjusted separately to be pointing at the NON ROTATED image!

        :param sample :: stack of sample images
        :param cfg :: pre-processing configuration
        :param white :: stack of white images
        :param dark :: stack of dark images

        :Return :: the rotated sample images, and the rotated averages for flat and dark
        """
        if not cfg or not isinstance(cfg, tomocfg.PreProcConfig):
            raise ValueError(
                "Cannot rotate images without a valid pre-processing configuration"
            )

        if not cfg.rotation or cfg.rotation < 0:
            self.tomo_print(
                " * Note: NOT rotating the input images.", priority=2)
            return sample, flat, dark

        self.tomo_print_timed_start(
            " * Starting rotation step ({0} degrees clockwise), with pixel data type: {1}...".
            format(cfg.rotation * 90, sample.dtype))

        # sanity check on sample data
        self._check_data_stack(sample)
        sample = self._rotate_imgs(sample, cfg.rotation)

        #
        if isinstance(flat, np.ndarray):
            flat = self._rotate_averages(flat, cfg.rotation)

        if isinstance(dark, np.ndarray):
            dark = self._rotate_averages(dark, cfg.rotation)

        self.tomo_print_timed_stop(
            " * Finished rotation step ({0} degrees clockwise), with pixel data type: {1}.".
            format(cfg.rotation * 90, sample.dtype))

        return sample, flat, dark

    def _rotate_imgs(self, data, rotation):
        """
        Rotate every image of a stack

        :param data :: image stack as a 3d numpy array
        :param cfg :: pre-processing configuration

        Returns :: rotated data (stack of images)
        """

        self._debug_print_memory_usage_linux("before rotation.")

        for idx in range(0, data.shape[0]):
            # rot90 rotates counterclockwise; cfg.rotation rotates clockwise
            counterclock_rotations = 4 - rotation
            data[idx, :, :] = np.rot90(data[idx, :, :], counterclock_rotations)

        self._debug_print_memory_usage_linux("after rotation.")

        return data

    def _rotate_averages(self, data, rotation):
        """
        Rotate every image of a stack

        :param data :: image stack as a 3d numpy array
        :param cfg :: pre-processing configuration

        Returns :: rotated data (stack of images)
        """

        self._debug_print_memory_usage_linux("before rotation.")

        # rot90 rotates counterclockwise; cfg.rotation rotates clockwise
        counterclock_rotations = 4 - rotation
        data[:, :] = np.rot90(data[:, :], counterclock_rotations)

        self._debug_print_memory_usage_linux("after rotation.")

        return data

    def run_reconstruct_3d(self,
                           proj_data,
                           preproc_cfg,
                           alg_cfg,
                           reconstruction_tool=None):
        """
        A 3D reconstruction

        :param proj_data :: Input projected images
        :param tool :: reconstruction tool to call/use

        Returns :: reconstructed volume
        """
        self._check_data_stack(proj_data)
        self._debug_print_memory_usage_linux(", before reconstruction.")
        num_proj = proj_data.shape[0]
        inc = float(preproc_cfg.max_angle) / (num_proj - 1)

        proj_angles = np.arange(0, num_proj * inc, inc)
        # For tomopy
        proj_angles = np.radians(proj_angles)

        if 'astra' == alg_cfg.tool:
            # run_reconstruct_3d_astra(proj_data, algorithm, cor, proj_angles=proj_angles)
            return self.run_reconstruct_3d_astra_simple(
                proj_data, proj_angles, alg_cfg, preproc_cfg.cor,
                reconstruction_tool)

        self.tomo_print(" * Using center of rotation: {0}".format(
            preproc_cfg.cor))
        if 'tomopy' == alg_cfg.tool and 'gridrec' != alg_cfg.algorithm and 'fbp' != alg_cfg.algorithm:
            if not alg_cfg.num_iter:
                reconstruction_tool.cfg_num_iter = tomocfg.PreProcConfig.DEF_NUM_ITER
            # For ref, some typical run times with 4 cores:
            # 'bart' with num_iter=20 => 467.640s ~= 7.8m
            # 'sirt' with num_iter=30 => 698.119 ~= 11.63
            self.tomo_print_timed_start(
                " * Starting iterative method with TomoPy. Algorithm: {0}, "
                "number of iterations: {1}...".format(alg_cfg.algorithm,
                                                      alg_cfg.num_iter))
            rec = reconstruction_tool.recon(
                tomo=proj_data,
                theta=proj_angles,
                center=preproc_cfg.cor,
                algorithm=alg_cfg.algorithm,
                num_iter=alg_cfg.num_iter)  # , filter_name='parzen')

        else:
            self.tomo_print_timed_start(
                " * Starting non-iterative reconstruction algorithm with TomoPy. "
                "Algorithm: {0}...".format(alg_cfg.algorithm))
            rec = reconstruction_tool.recon(
                tomo=proj_data,
                theta=proj_angles,
                center=preproc_cfg.cor,
                algorithm=alg_cfg.algorithm)

        self.tomo_print_timed_stop(
            " * Reconstructed 3D volume. Shape: {0}, and pixel data type: {1}.".
            format(rec.shape, rec.dtype))
        self._debug_print_memory_usage_linux(", after reconstruction.")
        return rec

    # def astra_reconstruct3d(self, sinogram, angles, depth, alg_cfg):
    #     """
    #     Run a reconstruction with astra

    #     :param sinogram :: sinogram data
    #     :param angles :: angles of the image projections
    #     :param depth :: number of rows in images/sinograms
    #     :param alg_cfg :: tool/algorithm configuration
    #     """
    #     # Some of these have issues depending on the GPU setup
    #     algs_avail = "[FP3D_CUDA], [BP3D_CUDA]], [FDK_CUDA], [SIRT3D_CUDA], [CGLS3D_CUDA]"

    #     if alg_cfg.algorithm.upper() not in algs_avail:
    #         raise ValueError(
    #             "Invalid algorithm requested for the Astra package: {0}. "
    #             "Supported algorithms: {1}".format(alg_cfg.algorithm,
    #                                                algs_avail))
    #     det_rows = sinogram.shape[0]
    #     det_cols = sinogram.shape[2]

    #     vol_geom = astra.create_vol_geom(sinograms.shape[0], depth,
    #                                      sinogram.shape[2])
    #     proj_geom = astra.create_proj_geom('parallel3d', 1.0, 1.0, det_cols,
    #                                        det_rows, np.deg2rad(angles))

    #     sinogram_id = astra.data3d.create("-sino", proj_geom, sinogram)
    #     # Create a data object for the reconstruction
    #     rec_id = astra.data3d.create('-vol', vol_geom)

    #     cfg = astra.astra_dict(alg_cfg.algorithm)
    #     cfg['ReconstructionDataId'] = rec_id
    #     cfg['ProjectionDataId'] = sinogram_id

    #     # Create the algorithm object from the configuration structure
    #     alg_id = astra.algorithm.create(cfg)
    #     # This will have a runtime in the order of 10 seconds.
    #     astra.algorithm.run(alg_id, alg_cfg.num_iter)
    #     # This could be used to check the norm of the difference between the projection data
    #     # and the forward projection of the reconstruction.
    #     # if "CUDA" in cfg_alg.algorithm and "FBP" not cfg_alg.algorithm:
    #     # self.norm_diff += astra.algorithm.get_res_norm(alg_id)**2
    #     # print math.sqrt(self.norm_diff)

    #     # Get the result
    #     rec = astra.data3d.get(rec_id)

    #     astra.algorithm.delete(alg_id)
    #     astra.data3d.delete(rec_id)
    #     astra.data3d.delete(sinogram_id)

    #     return rec

    # def run_reconstruct_3d_astra(self, proj_data, proj_angles, alg_cfg):
    #     """
    #     Run a reconstruction with astra, approach based on swpapping axes

    #     :param proj_data :: projection images
    #     :param proj_angles :: angles corresponding to the projection images
    #     :param alg_cfg :: tool/algorithm configuration
    #     """

    #     def get_max_frames(algorithm):
    #         frames = 8 if "3D" in algorithm else 1
    #         return frames

    #     nSinos = get_max_frames(alg_cfg.algorithm)
    #     iterations = alg_cfg.num_iter
    #     print(" astra recon - doing {0} iterations".format(iterations))

    # swaps outermost dimensions so it is sinogram layout
    # sinogram = proj_data
    # sinogram = np.swapaxes(sinogram, 0, 1)

    # Needs to be figured out better
    # ctr = cor
    # width = sinogram.shape[1]
    # pad = 50

    # sino = np.nan_to_num(1./sinogram)

    # pad the array so that the centre of rotation is in the middle
    # alen = ctr
    # blen = width - ctr
    # mid = width / 2.0

    # if ctr > mid:
    # plow = pad
    # phigh = (alen - blen) + pad
    # else:
    # plow = (blen - alen) + pad
    # phigh = pad

    # logdata = np.log(sino+1)

    # sinogram = np.tile(sinogram.reshape((1, ) + sinogram.shape), (8, 1, 1))

    # rec = self.astra_reconstruct3d(
    #     sinogram, proj_angles, depth=nSinos, alg_cfg=alg_cfg)

    # return rec

    def run_reconstruct_3d_astra_simple(self,
                                        proj_data,
                                        proj_angles,
                                        alg_cfg,
                                        cor=None,
                                        reconstruction_tool=None):
        """
        Run a reconstruction with astra, simple handling of projected data/images

        :param proj_data :: projection images
        :param alg_cfg :: tool/algorithm configuration
        :param cor :: center of rotation
        :param proj_angles :: angles corresponding to the projection images
        """
        import tomorec.tool_imports as tti
        astra = tti.import_tomo_tool('astra')
        sinograms = proj_data

        sinograms = np.swapaxes(sinograms, 0, 1)

        plow = (proj_data.shape[2] - cor * 2)
        phigh = 0

        # minval = np.amin(sinograms)
        sinograms = np.pad(sinograms, ((0, 0), (0, 0), (plow, phigh)),
                           mode='reflect')

        proj_geom = astra.create_proj_geom('parallel3d', .0, 1.0,
                                           proj_data.shape[1],
                                           sinograms.shape[2], proj_angles)
        sinogram_id = astra.data3d.create('-sino', proj_geom, sinograms)

        vol_geom = astra.create_vol_geom(
            proj_data.shape[1], sinograms.shape[2], proj_data.shape[1])
        recon_id = astra.data3d.create('-vol', vol_geom)
        alg_cfg = astra.astra_dict(alg_cfg.algorithm)
        alg_cfg['ReconstructionDataId'] = recon_id
        alg_cfg['ProjectionDataId'] = sinogram_id
        alg_id = astra.algorithm.create(alg_cfg)

        number_of_iters = 100
        astra.algorithm.run(alg_id, number_of_iters)
        recon = astra.data3d.get(recon_id)

        astra.algorithm.delete(alg_id)
        astra.data3d.delete(recon_id)
        astra.data3d.delete(sinogram_id)

        return recon

    def apply_postproc_filters(self, recon_data, cfg):
        """
        Apply all post-processing steps/filters/transformations on a reconstructed volume

        TODO :: move into a class

        :param recon_data :: the reconstructed images
        :param cfg :: post-processing configuration

        Returns :: filtered data (reconstructed 3d volume)
        """
        import prep as iprep

        # if cfg.circular_mask:
        #     self.tomo_print_timed_start(
        #         " * Applying circular mask on reconstructed volume...")
        #     recon_data = iprep.filters.circular_mask(
        #         recon_data, ratio=cfg.circular_mask)
        #     self.tomo_print_timed_stop(
        #         " * Finished applying circular mask on reconstructed volume")
        # else:
        #     self.tomo_print(
        #         " * Note: NOT applied circular mask on reconstructed volume",
        #         priority=2)

        if cfg.cut_off_level and cfg.cut_off_level > 0.0:
            self.tomo_print_timed_start(
                " * Starting to apply cut-off: {0}".format(cfg.cut_off))
            dmin = np.amin(recon_data)
            dmax = np.amax(recon_data)
            rel_cut_off = dmin + cfg.cut_off * (dmax - dmin)
            recon_data[recon_data < rel_cut_off] = dmin
            self.tomo_print_timed_stop(" * Finished applying cut-off.")

        if cfg.gaussian_filter_par:
            self.tomo_print(" * Gaussian filter not implemented", priority=2)

        if cfg.median_filter_size and cfg.median_filter_size > 1:
            self.tomo_print_timed_start(
                " * Applying median_filter on reconstructed volume, with filtersize: {0}".
                format(cfg.median_filter_size))

            recon_data = scipy.ndimage.median_filter(recon_data,
                                                     cfg.median_filter_size)

            self.tomo_print_timed_stop(
                " * Finished applying median_filter on reconstructed volume, with filtersize: {0}".
                format(cfg.median_filter_size))
        else:
            self.tomo_print(
                " * Note: NOT applied median_filter on reconstructed volume",
                priority=2)

        if cfg.median_filter3d_size and cfg.median_filter3d_size > 1:
            kernel_size = 3

            self.tomo_print_timed_start(
                " * Applying N-dimensional median filter on reconstructed volume, with filter size: {0} ".
                format(kernel_size))

            # Note this can be extremely slow
            recon_data = scipy.signal.medfilt(
                recon_data, kernel_size=kernel_size)

            self.tomo_print_timed_stop(
                " * Finished applying N-dimensional median filter on reconstructed volume, with filter size: {0} ".
                format(kernel_size))
        else:
            self.tomo_print(
                " * Note: NOT applied N-dimensional median filter on reconstructed volume"
            )

        return recon_data

    def read_in_stack(self,
                      sample_path,
                      img_format,
                      flat_field_path=None,
                      dark_field_path=None):
        """
        Loads a stack, including sample, white and dark images.

        :param sample_path :: path to sample images

        :param img_format :: image format to expect/use (as a filename extension)

        :param flat_field_path :: (optional) path to open beam / white image(s).
        Can be a file or directory

        :param dark_field_path :: (optional) path to dark field image(s).
        Can be a file or directory

        Returns :: stack of images as a 3-elements tuple: numpy array with sample images, white image,
        and dark image.
        """
        # Note, not giving prefix. It will load all the files found.
        # Example prefixes are prefix = 'tomo_', prefix = 'LARMOR00', prefix =
        # 'angle_agg'

        sample, white, dark = tomoio.read_stack_of_images(
            sample_path,
            flat_field_path=flat_field_path,
            dark_field_path=dark_field_path,
            file_extension=img_format)

        if not isinstance(sample, np.ndarray) or not sample.shape \
                or not isinstance(sample.shape, tuple) or 3 != len(sample.shape):
            raise RuntimeError(
                "Error reading sample images. Could not produce a 3-dimensional array "
                "of data from the sample images. Got: {0}".format(sample))

        return (sample, white, dark)

    def save_recon_output(self, recon_data, cfg, save_horiz_slices=False):
        """
        Save output reconstructed volume in different forms.

        :param recon_data :: reconstructed data volume. A sequence of images will be saved from this
        :param cfg :: configuration of the reconstruction, including output paths and formats
        :param save_horiz_slices :: Save images along the horizontal axis in addition to the vertical
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
            img_format=cfg.preproc_cfg.out_img_format)

        # Sideways slices:
        save_horiz_slices = False
        if save_horiz_slices:
            out_horiz_dir = os.path.join(output_dir, 'horiz_slices')
            print("* Saving horizontal slices in: {0}".format(out_horiz_dir))
            tomoio.save_recon_as_horizontal_slices(
                recon_data,
                out_horiz_dir,
                name_prefix=self._OUT_HORIZ_SLICES_SUBDIR,
                img_format=cfg.preproc_cfg.out_img_format)

        self.tomo_print_timed_stop(
            " * Finished saving slices of the reconstructed volume in: {0}".
            format(out_recon_dir))

    def save_preproc_images(self,
                            preproc_data,
                            output_dir,
                            preproc_cfg,
                            out_dtype='uint16',
                            constant_factor=True):
        """
        Save (pre-processed) images from a data array to image files.

        :param constant_factor :: default True, multiply all output images by the same factor,
                                in order to try and keep them with the same brightness
        :param output_dir :: where results are being saved, including the pre-proc images/slices
        :param preproc_data :: data volume with pre-processed images
        :param preproc_cfg :: pre-processing configuration set up for a reconstruction
        :param out_dtype :: dtype used for the pixel type/depth in the output image files
        """

        if constant_factor:
            min_pix = np.amin(preproc_data[0, :, :])
            max_pix = np.amax(preproc_data[0, :, :])

            # replace float infinities to one, which will just be scaled up
            preproc_data[preproc_data == np.inf] = 1

            # from bigger to smaller type, example: float32 => uint16
            pix_range = max_pix - min_pix

            scale_factor = (
                np.iinfo(out_dtype).max - np.iinfo(out_dtype).min) / pix_range
        else:
            # scale factor will be generated for each image
            scale_factor = None

        if preproc_cfg.save_preproc_imgs:
            preproc_dir = os.path.join(output_dir,
                                       self._PREPROC_IMGS_SUBDIR_NAME)
            self.tomo_print_timed_start(
                " * Saving all pre-processed images (preproc_data) into {0} dtype: {1}".
                format(preproc_dir, preproc_data.dtype))
            tomoio.make_dirs_if_needed(preproc_dir)
            for idx in range(0, preproc_data.shape[0]):
                # rescale_intensity has issues with float64=>int16

                tomoio.write_image(
                    preproc_data[idx, :, :],
                    os.path.join(preproc_dir,
                                 'out_preproc_proj_image' + str(idx).zfill(6)),
                    img_format=preproc_cfg.out_img_format,
                    dtype=out_dtype,
                    scale_factor=scale_factor)

            self.tomo_print_timed_stop(
                " * Saving pre-processed images finished.")
        else:
            self.tomo_print(" * NOTE: NOT saving pre-processed images...")

    def save_single_image(self,
                          data,
                          preproc_cfg,
                          output_dir=None,
                          out_dtype='uint16',
                          image_name='saved_image',
                          image_index=0,
                          scale_factor=None):
        """
        Save (pre-processed) images from a data array to image files.

        :param output_dir :: where results are being saved, including the pre-proc images/slices
        :param data :: data volume with pre-processed images
        :param preproc_cfg :: pre-processing configuration set up for a reconstruction
        :param out_dtype :: dtype used for the pixel type/depth in the output image files
        :param image_name: image name to be appended
        :param image_index: image index to be appended
        :param scale_factor :: Default None, if left None, the scale factor will be calculated for the image
        """

        # DEBUG message
        # print("   with min_pix: {0}, max_pix: {1}".format(min_pix, max_pix))
        if output_dir is not None:
            preproc_dir = os.path.join(output_dir,
                                       self._PREPROC_IMGS_SUBDIR_NAME)
        else:
            preproc_dir = preproc_cfg.output_dir

        self.tomo_print_timed_start(" * Saving single image {0} dtype: {1}".
                                    format(preproc_dir, data.dtype))

        tomoio.make_dirs_if_needed(preproc_dir)

        # rescale_intensity has issues with float64=>int16
        tomoio.write_image(
            data[:, :],
            os.path.join(preproc_dir, image_name + str(image_index).zfill(6)),
            img_format=preproc_cfg.out_img_format,
            dtype=out_dtype,
            scale_factor=scale_factor)

        self.tomo_print_timed_stop(" * Finished saving single image.")

    def _check_data_stack(self, data):
        if not isinstance(data, np.ndarray):
            raise ValueError(
                "Invalid stack of images data. It is not a numpy array: {0}".
                format(data))

        if 3 != len(data.shape):
            raise ValueError(
                "Invalid stack of images data. It does not have 3 dimensions. Shape: {0}".
                format(data.shape))

    def find_center(self, cfg):
        self._check_paths_integrity(cfg)

        self.tomo_print_timed_start(" * Importing tool " + cfg.alg_cfg.tool)
        # import tool
        import tomorec.tool_imports as tti
        tomopy = tti.import_tomo_tool(cfg.alg_cfg.tool)

        self.tomo_print_timed_stop(" * Tool loaded.")

        self.tomo_print_timed_start(" * Loading data...")
        # load in data
        sample, flat, dark = self.read_in_stack(
            cfg.preproc_cfg.input_dir, cfg.preproc_cfg.in_img_format,
            cfg.preproc_cfg.input_dir_flat, cfg.preproc_cfg.input_dir_dark)
        self.tomo_print_timed_stop(
            " * Data loaded. Shape of raw data: {0}, dtype: {1}.".format(
                sample.shape, sample.dtype))

        # rotate
        sample, flat, dark = self.rotate_stack(sample, cfg.preproc_cfg)

        # crop the ROI, this is done first, so beware of what the correct ROI
        # coordinates are
        sample, flat, dark = self.crop_coords(sample, cfg.preproc_cfg)

        # sanity check
        self.tomo_print(" * Sanity check on data", 0)
        self._check_data_stack(sample)

        num_projections = sample.shape[0]
        inc = float(cfg.preproc_cfg.max_angle) / (num_projections - 1)

        self.tomo_print(" * Calculating projection angles")
        proj_angles = np.arange(0, num_projections * inc, inc)
        # For tomopy
        self.tomo_print(" * Calculating radians for TomoPy")
        proj_angles = np.radians(proj_angles)

        size = int(num_projections)

        # depending on the number of COR projections it will select different
        # slice indices
        cor_num_checked_projections = 6
        cor_proj_slice_indices = []
        cor_slice_index = 0

        if cor_num_checked_projections < 2:
            # this will give us the middle slice
            cor_slice_index = int(size / 2)
            cor_proj_slice_indices.append(cor_slice_index)
        else:
            for c in range(cor_num_checked_projections):
                cor_slice_index += int(size / cor_num_checked_projections)
                cor_proj_slice_indices.append(cor_slice_index)

        calculated_cors = []

        self.tomo_print_timed_start(
            " * Starting COR calculation for " +
            str(cor_num_checked_projections) + " out of " +
            str(sample.shape[0]) + " projections", 2)

        cropCoords = cfg.preproc_cfg.crop_coords[0]
        imageWidth = sample.shape[2]

        # if crop corrds match with the image width then the full image was
        # selected
        pixelsFromLeftSide = cropCoords if cropCoords - imageWidth <= 1 else 0

        for slice_idx in cor_proj_slice_indices:
            tomopy_cor = tomopy.find_center(
                tomo=sample, theta=proj_angles, ind=slice_idx, emission=False)
            print(" ** COR for slice", str(slice_idx), ".. REL to CROP ",
                  str(tomopy_cor), ".. REL to FULL ",
                  str(tomopy_cor + pixelsFromLeftSide))
            calculated_cors.append(tomopy_cor)

        self.tomo_print_timed_stop(" * Finished COR calculation.", 2)

        averageCORrelativeToCrop = sum(calculated_cors) / len(calculated_cors)
        averageCORrelativeToFullImage = sum(calculated_cors) / len(
            calculated_cors) + pixelsFromLeftSide

        # we add the pixels cut off from the left, to reflect the full image in
        # Mantid
        self.tomo_print(" * Printing average COR in relation to cropped image "
                        + str(cfg.preproc_cfg.crop_coords) + ":", 2)
        print(str(round(averageCORrelativeToCrop)))
        self.tomo_print(" * Printing average COR in relation to FULL image:",
                        2)
        print(str(round(averageCORrelativeToFullImage)))
