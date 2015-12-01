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


#pylint: disable=too-few-public-methods
class ToolAlgorithmConfig(object):
    """
    Reconstruction algorithm specific configuration. Required for any reconstruction:
    a tool that implements the method/algorithm,
    the algorithm name,
    (only for the iterative algorithms) the number of iterations
    (only for some algorithms) a regularization/smoothing parameter
    """
    DEF_TOOL = 'tomopy'
    DEF_ALGORITHM = 'gridrec'

    def __init__(self):
        self.tool = self.DEF_TOOL
        self.algorithm = self.DEF_ALGORITHM
        self.num_iter = None
        self.regularization = None


#pylint: disable=too-many-instance-attributes
class PreProcConfig(object):
    """
    All pre-processing options required to run a tomographic reconstruction.

    Options like the stripe removal, MCP correction, or even the median filter would
    better be handled as plugins. For the time being we just have a panel with a fixed
    set of options to enable/disable/configure
    """

    DEF_NUM_ITER = 5

    def __init__(self):
        # defaults that look sensible for the MCP detector:
        # median_filter=3, rotate=-1, crop=[0,  252, 0, 512], MCP correction: on
        self.input_dir = None
        self.in_img_format = 'tiff'
        # Center of rotation
        self.cor = None
        self.normalize_proton_charge = False
        self.normalize_flat_dark = True
        self.cut_off_level = 0
        self.scale_down = 0
        self.mcp_correction = True
        self.stripe_removal_method = 'wavelet-fourier'
        # region of interest
        self.crop_coords = None
        self.median_filter_size = 3
        self.rotation = -1
        self.max_angle = 360
        self.save_preproc_imgs = True

class PostProcConfig(object):
    """
    All pre-processing options required to run a tomographic reconstruction
    """

    def __init__(self):
        """
        Builds a default post-processing configuration with a sensible choice of parameters
        """
        self.output_dir = None
        self.circular_mask = 0.94
        self.cut_off_level = 0
        self.median_filter_size = 0
        self.median_filter3d_size = 0


class ReconstructionCommand(object):
    """
    Run a tomographic reconstruction command, which can be a local process or a job on a
    remote machine/scheduler.
    """

    def __init__(self):
        self.preproc_cfg = PreProcConfig()
        self.alg_cfg = ToolAlgorithmConfig
        self.postproc_cfg = PostProcConfig()

    def do_recon(self, preproc_cfg, alg_cfg, postproc_cfg, cmd_line=None):
        """
        Run a reconstruction using a particular tool, algorithm and setup

        """

        pass
