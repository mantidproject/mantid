from __future__ import (absolute_import, division, print_function)

# Experimental options, disabled and not present in the config objects for now
# These and related algorithms needs more evaluation/benchmarking


def apply_filter(tool, preproc_data, cfg):
    if False:
        self.tomo_print_timed_start("Starting adjust range...")
        preproc_data = tomopy.misc.corr.adjust_range(preproc_data)
        self.tomo_print_timed_stop("Finished adjusting range.")
