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
    print(
        scipy.__file__,
        "< this is the directory of where scipy was imported from. If this is a Mantid "
        "directory, then the scipy that fails to import here is older and/or missing ndimage than "
        "the one I have locally")
except ImportError:
    raise ImportError(
        "Could not find the package scipy which is required for image pre-/post-processing"
    )

try:
    # for some reason ndimage is not found here. reason unknown?
    import scipy.ndimage
    print(
        scipy.ndimage.__file__,
        "< this is the directory of where scipy was imported from. If this is a Mantid "
        "directory, then the scipy that fails to import here is older and/or missing ndimage than "
        "the one I have locally")
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

    def gen_readme_summary_begin(self, filename, cfg, cmd_line):
        """
        To write configuration, settings, etc. early on. As early as possible, before any failure
        can happen.

        @param filename :: name of the readme/final report file
        @param cfg :: full reconstruction configuration
        @param cmd_line :: command line originally used to run this reconstruction, when running
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

        @param filename :: name of the readme/final report file
        @param data_stages :: tuple with data in three stages (raw, pre-processed, reconstructed)
        @param tstart :: time at the beginning of the job/reconstruction, when the first part of the
        readme file was written
        @param t_recon_elapsed :: reconstruction time
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

    def apply_all_preproc(self, data, preproc_cfg, white, dark):
        """
        Do all the pre-processing. This does all that is needed between a)
        loading the input data, and b) starting a reconstruction
        run/job. From raw inputs to pre-proc data that is ready to go for
        reconstruction.

        @param data :: raw data (sample projection images)
        @param preproc_cfg :: pre-processing configuration
        @param white :: white / flat / open-beam image for normalization in some of the first
        pre-processing steps
        @param dark :: dark image for normalization

        Returns :: pre-processed data.

        """
        self._check_data_stack(data)

        self.tomo_print(" * Beginning pre-processing with pixel data type: " +
                        str(data.dtype))
        preproc_data = self.apply_prep_filters(data, preproc_cfg, white, dark)
        preproc_data = self.apply_line_projection(preproc_data, preproc_cfg)
        preproc_data = self.apply_final_preproc_corrections(preproc_data,
                                                            preproc_cfg)

        return preproc_data

    def apply_prep_filters(self, data, cfg, white, dark):
        """
        Apply the normal initial pre-processing filters, including simple
        operations as selecting/cropping to the region-of-interest,
        normalization, etc. If the images need to be rotated this is
        done as a first step (so all intermediate pre-processed
        results will be rotated as required).

        @param data :: projection images data, as 3d numpy array, with images along outermost (z)
        dimension

        @param cfg :: pre-processing configuration
        @param white :: white/flat/open-beam image for normalization
        @param dark :: dark image for normalization

        Returns :: process/filtered data (sizes can change (cropped) and data can be rotated)

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

        data, white, dark = self.rotate_stack(data, cfg, white, dark)

        if self.crop_before_normaliz:
            data = self.crop_coords(data, cfg)

        data = self.normalize_flat_dark(data, cfg, white, dark)
        data = self.normalize_air_region(data, cfg)

        if not self.crop_before_normaliz:
            data = self.crop_coords(data, cfg)

        data = self.apply_cut_off_and_others(data, cfg)

        return data

    def _check_data_stack(self, data):
        if not isinstance(data, np.ndarray):
            raise ValueError(
                "Invalid stack of images data. It is not a numpy array: {0}".
                format(data))

        if 3 != len(data.shape):
            raise ValueError(
                "Invalid stack of images data. It does not have 3 dimensions. Shape: {0}".
                format(data.shape))
