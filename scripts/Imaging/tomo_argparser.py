from __future__ import (absolute_import, division, print_function)

# Copyright &copy; 2017-2018 ISIS Rutherford Appleton Laboratory, NScD
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
# Author: Dimitar Tasev, Mantid Development Team
#
# File change history is stored at: <https://github.com/mantidproject/mantid>.
# Code Documentation is available at: <http://doxygen.mantidproject.org>


def grab_full_config():
    from configs.functional_config import FunctionalConfig
    from configs.postproc_config import PostProcConfig
    from configs.preproc_config import PreProcConfig
    from configs.recon_config import ReconstructionConfig

    import argparse
    from argparse import RawTextHelpFormatter

    parser = argparse.ArgumentParser(
        description='Run tomographic reconstruction via third party tools',
        formatter_class=RawTextHelpFormatter)

    # this sets up the arguments in the parser, with defaults from the Config
    # file
    functional_args = FunctionalConfig()
    parser = functional_args.setup_parser(parser)

    pre_args = PreProcConfig()
    parser = pre_args.setup_parser(parser)

    post_args = PostProcConfig()
    parser = post_args.setup_parser(parser)

    # parse the real arguments
    args = parser.parse_args()

    # update the configs
    functional_args.update(args)
    pre_args.update(args)
    post_args.update(args)
    # combine all of them together
    return ReconstructionConfig(functional_args, pre_args, post_args)
