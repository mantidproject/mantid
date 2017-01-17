# Remove stripes in sinograms / ring artefacts in reconstructed volume
def execute(data, config):
    from recon.helper import Helper
    h = Helper(config)

    if config.stripe_removal_method:

        if 'wavelet-fourier' == config.post.stripe_removal_method.lower():
            h.pstart(
                "Starting removal of stripes/ring artifacts using the method '{0}'...".format(config.stripe_removal_method))

            # data = tomopy.prep.stripe.remove_stripe_fw(data)
            data = iprep.filters.remove_stripes_ring_artifacts(
                data, 'wavelet-fourier')

            h.pstop("Finished removal of stripes/ring artifacts.")

        elif 'titarenko' == config.stripe_removal_method.lower():
            h.pstart(
                "Starting removal of stripes/ring artifacts, using the method '{0}'...".format(config.stripe_removal_method))

            data = tomopy.prep.stripe.remove_stripe_ti(
                data)

            h.pstop("Finished removal of stripes/ring artifacts.")
        else:
            h.tomo_print_warning(
                "WARNING: stripe removal method '{0}' is unknown. Not applying it.".
                format(config.stripe_removal_method))
    else:
        h.tomo_print_note("NOT applying stripe removal.")
