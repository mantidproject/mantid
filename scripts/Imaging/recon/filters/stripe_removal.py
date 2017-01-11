# Remove stripes in sinograms / ring artefacts in reconstructed volume

def apply_filter(tool, preproc_data, cfg):
    if cfg.stripe_removal_method:
        import prep as iprep
        if 'wavelet-fourier' == cfg.stripe_removal_method.lower():
            self.tomo_print_timed_start(
                " * Starting removal of stripes/ring artifacts using the method '{0}'...".
                    format(cfg.stripe_removal_method))

            # preproc_data = tomopy.prep.stripe.remove_stripe_fw(preproc_data)
            preproc_data = iprep.filters.remove_stripes_ring_artifacts(
                preproc_data, 'wavelet-fourier')

            self.tomo_print_timed_start(
                " * Finished removal of stripes/ring artifacts.")

        elif 'titarenko' == cfg.stripe_removal_method.lower():
            self.tomo_print_timed_start(
                " * Starting removal of stripes/ring artifacts, using the method '{0}'...".
                    format(cfg.stripe_removal_method))

            preproc_data = tomopy.prep.stripe.remove_stripe_ti(
                preproc_data)

            self.tomo_print_timed_stop(
                " * Finished removal of stripes/ring artifacts.")
        else:
            self.tomo_print(
                " * WARNING: stripe removal method '{0}' is unknown. Not applying it.".
                    format(cfg.stripe_removal_method),
                verbosity=2)
    else:
        self.tomo_print(
            " * Note: NOT applying stripe removal.", verbosity=2)