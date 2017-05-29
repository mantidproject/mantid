def execute(data_vol, method='wavelet-fourier'):
    """
    Removal of stripes in sinograms / ring artifacts in reconstructed
    volume.

    This is an unimplemented stub at the moment.
    As first step it should implement one methods: the combined wavelet-Fourier method
    (Muench et al. 2009, Opt Express, 17(10), 8567-91), as implemented also in tomopy.

    @param data_vol :: stack of projection images as 3d data (dimensions z, y, x), with
    z different projections angles, and y and x the rows and columns of individual images.

    @param method :: 'wf': Wavelet-Fourier based method

    Returns :: filtered data hopefully without stripes which should dramatically decrease
    ring artifacts after reconstruction and the effect of these on post-processing tasks
    such as segmentation of the reconstructed 3d data volume.
    """
    supported_methods = ['wavelet-fourier']

    if not isinstance(data_vol, np.ndarray) or 3 != len(data_vol.shape):
        raise ValueError(
            "Wrong data volume when trying to filter stripes/ring artifacts: {0}".
            format(data_vol))

    if method.lower() not in supported_methods:
        raise ValueError(
            "The method to remove stripes and ring artifacts must be one of {0}. "
            "Got unknown value: {1}".format(supported_methods, method))

    try:
        import tomopy
        stripped_vol = tomopy.prep.stripe.remove_stripe_fw(data_vol)
    except ImportError:
        stripped_vol = remove_sino_stripes_rings_wf(data_vol)

    return stripped_vol
