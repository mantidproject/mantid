from __future__ import (absolute_import, division, print_function)
import importlib


def register_into(parser):
    """
    This function will import all the filters, and then call cli_register(parser) on them.
    """
    MODULES = [
        'circular_mask', 'crop_coords', 'cut_off', 'gaussian', 'median_filter',
        'minus_log', 'normalise_by_air_region', 'normalise_by_flat_dark',
        'outliers', 'rebin', 'rotate_stack', 'ring_removal', 'stripe_removal',
        'value_scaling'
    ]

    for m in MODULES:
        # we specify the full absolute path and append the package name
        # the code underneath does something like import core.filters.package_name
        m = importlib.import_module('core.filters.' + m)
        m.cli_register(parser)

    return parser
