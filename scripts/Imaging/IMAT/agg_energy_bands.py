"""
Command line tool to aggregate multiple bands from energy selective imaging experiments.
This simply provides a command line interface to prep.energy_bands_aggregator

To print usage details:
ipython agg_energy_bands.py --help

Usage example:
ipython -- agg_energy_bands.py --input-path=~/test/LARMOR/test_few_angles/ --output-path=out_stack_test

ipython -- agg_energy_bands.py --input-path=~/test/LARMOR/test_few_angles/ --output-path=out_stack_test --energy-bands=100,299 --format=png

"""
# Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD
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

import IMAT.prep.energy_bands_aggregator as eba
import argparse

def _indices_to_tuple(indices):
    """
    See if we have energy band indices. Turn something like '100,200' to a tuple (100,200)

    @param indices :: indices given by the user

    Returns:: a tuple with the min and max indices
    """
    tidx = None
    if indices:
        idxlist = indices.split(',')
        if 2 != len(indices) or not isinstance(indices[0], int) or not isinstance(indices[1], int):
            tidx = (int(idxlist[0]), int(idxlist[1]))
        else:
            raise ValueError("Wrong energy band indices given: {0}. "
                             "I expect two integers separated by a comma".format(
                                 indices))
    return tidx

def agg_cli():
    """
    Provides a command line interface to the EnergyBandsAggregator
    """
    arg_parser = argparse.ArgumentParser()
    eb_agg = eba.EnergyBandsAggregator()

    arg_parser.add_argument("-i","--input-path", required=True, help="Input directory")
    arg_parser.add_argument("-o","--output-path", help="Where to write the output stack of (aggregated) images. "
                            "If left empty a default '{0}' in the current working directory.".
                            format(eb_agg.default_out_path))
    arg_parser.add_argument("-b","--energy-bands", help="Energy band indices to aggregate into the outputs. "
                            "Must be given as two integer values separated by comma. "
                            "The indices start from 0. By default all bands are included.")
    arg_parser.add_argument("-t","--agg-type", help="Type of aggregation. Supported: {0} ".
                            format(eb_agg.supported_aggs))

    arg_parser.add_argument("-f","--format", help="Format of output files. Supported: {0}. Default: {1}.".
                            format(eb_agg.supported_out_formats,
                                   eb_agg.default_out_format))

    args = arg_parser.parse_args()
    tidx = _indices_to_tuple(args.energy_bands)

    eb_agg.agg_angles(args.input_path, args.output_path, band_indices=tidx, agg_method=args.agg_type,
                      out_format=args.format)


if __name__=='__main__':
    agg_cli()
