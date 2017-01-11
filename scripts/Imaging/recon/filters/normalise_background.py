# Experimental options, disabled and not present in the config objects for now
# These and related algorithms needs more evaluation/benchmarking
def apply_filter(tool, preproc_data, cfg):
    if False:
        self.tomo_print_timed_start(
            " * Starting background normalisation...")

        preproc_data = tomopy.prep.normalize.normalize_bg(
            preproc_data, air=5)
        self.tomo_print_timed_stop(" * Finished background normalisation.")