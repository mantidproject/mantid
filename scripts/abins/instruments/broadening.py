# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np
from scipy.special import erf
from scipy.signal import convolve

import abins.parameters

prebin_required_schemes = ["interpolate", "interpolate_coarse"]


def broaden_spectrum(frequencies, bins, s_dft, sigma, scheme="gaussian_truncated"):
    """Convert frequency/S data to broadened spectrum on a regular grid

    Several algorithms are implemented, for purposes of
    performance/accuracy optimisation and demonstration of legacy
    behaviour.

    :param frequencies: input frequency series; these may be irregularly spaced
    :type frequencies: 1D array-like
    :param bins:
        Evenly-spaced frequency bin values for the output spectrum. If *frequencies* is None, s_dft is assumed to
        correspond to mid-point frequencies on this mesh.
    :type bins: 1D array-like
    :param s_dft: scattering values corresponding to *frequencies*
    :type s_dft: 1D array-like
    :param sigma:
        width of broadening function. This should be a scalar used over the whole spectrum, or a series of values
        corresponding to *frequencies*.
    :type sigma: float or 1D array-like
    :param scheme:
        Name of broadening method used. Options:

        - none: Return the input data as a histogram, ignoring the value of sigma
        - gaussian: Evaluate a Gaussian on the output grid for every input point and sum them. Simple but slow, and
              recommended only for benchmarking and reference calculations.
        - normal: Generate histograms with appropriately-located normal distributions for every input point. In
              principle this is more accurate than 'Gaussian' but in practice results are very similar and just as slow.
        - gaussian_truncated: Gaussians are cut off at a range of 3 * max(sigma). This leads to small discontinuities in
              the tails of the broadened peaks, but is _much_ faster to evaluate as all the broadening functions are the
              same (limited) size. Recommended for production calculations.
        - normal_truncated: As gaussian_truncated, but with normally-distributed histograms replacing Gaussian evaluation.
              Results are marginally more accurate at a marginally increased cost.
        - interpolate: A possibly-novel approach in which the spectrum broadened with a frequency-dependent kernel is
              approximated by interpolation between a small set of spectra broadened with logarithmically-spaced
              fixed-width kernels. In principle this method introduces an incorrect asymmetry to the peaks. In practice
              the results are visually acceptable for a slowly-varying instrumental broadening function, with greatly
              reduced computation time. The default spacing factor of sqrt(2) gives error of ~1%.
        - interpolate_coarse: The approximate interpolative scheme (above) with a spacing factor of 2, which yields
              error of ~5%. This is more likely to cause artefacts in the results... but it is very fast. Not recomended
              for production calculations.

    :type scheme: str

    :returns: (freq_points, broadened_spectrum)

    The *freq_points* are the mid-bin frequency values which
    nominally correspond to *broadened_spectrum*; both of these are
    1D arrays of length (N-1) corresponding to the length-N *bins*.
    """
    if (bins is None) or (s_dft is None) or (sigma is None):
        raise ValueError("Frequency bins, S data and broadening width must be provided.")

    bins = np.asarray(bins)
    freq_points = (bins[1:] + bins[:-1]) / 2

    #  Don't bother broadening if there is nothing here to broaden: return empty spectrum
    if (len(s_dft) == 0) or not np.any(s_dft):
        return freq_points, np.zeros_like(freq_points)

    #  Allow histogram data to be input as bins + s_dft; use the mid-bin values
    elif frequencies is None:
        if len(s_dft) == len(bins) - 1:
            frequencies = freq_points
        else:
            raise ValueError("Cannot determine frequency values for s_dft before broadening")

    if scheme == "none":
        # Null broadening; resample input data and return
        hist, _ = np.histogram(frequencies, bins=bins, weights=s_dft, density=False)
        return freq_points, hist

    elif scheme == "gaussian":
        # Gaussian broadening on the full grid (no range cutoff, sum over all peaks).
        # This is not very optimised but a useful reference for "correct" results.
        bin_width = bins[1] - bins[0]
        kernels = mesh_gaussian(sigma=sigma[:, np.newaxis], points=freq_points, center=frequencies[:, np.newaxis])
        spectrum = np.dot(kernels.transpose(), s_dft)
        return freq_points, spectrum

    elif scheme == "normal":
        # Normal distribution on the full grid (no range cutoff, sum over all peaks)
        # In principle this scheme yields slightly more accurate histograms that direct
        # Gaussian evaluation. In practice the results are very similar but there may
        # be a performance boost from avoiding the normalisation step.
        kernels = normal(sigma=sigma[:, np.newaxis], bins=bins, center=frequencies[:, np.newaxis])
        spectrum = np.dot(kernels.transpose(), s_dft)
        return freq_points, spectrum

    elif scheme == "gaussian_truncated":
        # Gaussian broadening on the full grid (sum over all peaks, Gaussian range limited to 3 sigma)
        # All peaks use the same number of points; if the center is located close to the low- or high-freq limit,
        # the frequency point set is "justified" to lie inside the range
        points, spectrum = trunc_and_sum_inplace(
            function=gaussian,
            sigma=sigma[:, np.newaxis],
            points=freq_points,
            bins=bins,
            center=frequencies[:, np.newaxis],
            limit=abins.parameters.sampling["broadening_range"],
            weights=s_dft[:, np.newaxis],
            method="auto",
        )

        # Normalize the Gaussian kernel such that sum of points matches input
        bin_width = bins[1] - bins[0]
        return points, spectrum * bin_width

    elif scheme == "normal_truncated":
        # Normal distribution on the full grid (sum over all peaks, Gaussian range limited to 3 sigma)
        # All peaks use the same number of points; if the center is located close to the low- or high-freq limit,
        # the frequency point set is "justified" to lie inside the range

        return trunc_and_sum_inplace(
            function=normal,
            function_uses="bins",
            sigma=sigma[:, np.newaxis],
            points=freq_points,
            bins=bins,
            center=frequencies[:, np.newaxis],
            limit=abins.parameters.sampling["broadening_range"],
            weights=s_dft[:, np.newaxis],
            method="auto",
        )

    elif scheme == "interpolate":
        return interpolated_broadening(
            sigma=sigma,
            points=freq_points,
            bins=bins,
            center=frequencies,
            weights=s_dft,
            is_hist=True,
            limit=abins.parameters.sampling["broadening_range"],
            function="gaussian",
            spacing="sqrt2",
        )

    elif scheme == "interpolate_coarse":
        return interpolated_broadening(
            sigma=sigma,
            points=freq_points,
            bins=bins,
            center=frequencies,
            weights=s_dft,
            is_hist=True,
            limit=abins.parameters.sampling["broadening_range"],
            function="gaussian",
            spacing="2",
        )

    else:
        raise ValueError(
            'Broadening scheme "{}" not supported for this instrument, please correct '
            'abins.parameters.sampling["broadening_scheme"]'.format(scheme)
        )


def mesh_gaussian(sigma=None, points=None, center=0):
    """Evaluate a Gaussian function over a regular (given) mesh

    The Gaussian is normalized by multiplying by the bin-width. This approach will give correct results even when the
    function extends (or originates) beyond the sampling region; however, it cannot be applied to an irreglar mesh.
    Note that this normalisation gives a consistent _sum_ rather than a consistent _area_; it is a suitable kernel for
    convolution of a histogram.

    If 'points' contains less than two values, an empty or zero array is returned as the spacing cannot be inferred.

    :param sigma: sigma defines width of Gaussian
    :param points: points for which Gaussian should be evaluated
    :type points: 1-D array
    :param center: center of Gaussian
    :type center: float or array
    :returns:
        numpy array with calculated Gaussian values. Dimensions are broadcast from inputs:
        to obtain an (M x N) 2D array of gaussians at M centers and
        corresponding sigma values, evaluated over N points, let *sigma* and
        *center* be arrays of shape (M, 1) (column vectors) and *points* have
         shape (N,) or (1, N) (row vector).
    """

    if len(points) < 2:
        return np.zeros_like(points * sigma)

    bin_width = points[1] - points[0]
    return gaussian(sigma=sigma, points=points, center=center) * bin_width


def gaussian(sigma=None, points=None, center=0):
    """Evaluate a Gaussian function over a given mesh

    :param sigma: sigma defines width of Gaussian
    :param points: points for which Gaussian should be evaluated
    :type points: 1-D array
    :param center: center of Gaussian
    :type center: float or array
    :param normalized:
    If True, scale the output so that the sum of all values
    equals 1 (i.e. make a suitable kernel for convolution of a histogram.)
    This will not be reliable if the function extends beyond the provided set of points.
    :type normalized: bool
    :returns:
        numpy array with calculated Gaussian values. Dimensions are broadcast from inputs:
        to obtain an (M x N) 2D array of gaussians at M centers and
        corresponding sigma values, evaluated over N points, let *sigma* and
        *center* be arrays of shape (M, 1) (column vectors) and *points* have
         shape (N,) or (1, N) (row vector).
    """
    two_sigma = 2.0 * sigma
    sigma_factor = two_sigma * sigma
    kernel = np.exp(-((points - center) ** 2) / sigma_factor) / (np.sqrt(sigma_factor * np.pi))

    return kernel


def normal(bins=None, sigma=None, center=0):
    """
    Broadening function: fill histogram bins with normal distribution

    This should give identical results to evaluating the Gaussian on a very dense grid before resampling to the bins
    It is implemented using the analytic integral of the Gaussian function and should not require normalisation

    Int [exp(-0.5((x-u)/sigma)^2)]/[sigma sqrt(2 pi)] dx = -0.5 erf([u - x]/[sqrt(2) sigma]) + k

    :param bins: sample bins for function evaluation. This _must_ be evenly-spaced.
    :type bins: 1-D array
    :param sigma: width parameter; use a column vector to obtain a 2D array of peaks with different widths
    :type sigma: float or Nx1 array
    :param center: x-position of function center; use a column vector to obtain a 2D array of peaks with different centers.
    :type center: float or Nx1 array
    """

    integral_at_bin_edges = -0.5 * erf((center - bins) / (np.sqrt(2) * sigma))
    bin_contents = integral_at_bin_edges[..., 1:] - integral_at_bin_edges[..., :-1]

    return bin_contents


def trunc_function(function=None, sigma=None, points=None, center=None, limit=3):
    """
    Wrap a simple broadening function and evaluate within a limited range

                                 *
                                * *
                                * *
                               *   *
                              *     *
                            *         *
                        ..: *         * :..
                     -----------------------
                            |----|
                         limit * sigma

    A spectrum is returned corresponding to the entire input set of
    bins/points, but this is zeroed outside of a restricted range (``limit * sigma``).
    The purpose of this is performance optimisation compared to evaluating over the whole range.
    There is an artefact associated with this: a vertical step down to zero at the range cutoff.

    Note that when evaluating over multiple center/sigma rows, each row uses its own cutoff range.

    Note that this relies heavily on the _broadcasting_ capabilities of the
    functions. They will be called with long 1-D arrays of input data, expected
    to return 1-D data that is unpacked to the results matrix.

    :param function: broadening function; this should accept named arguments "center", "sigma" and either "bins" or "points".
    :type function: python function
    :param sigma: sigma defines width of Gaussian
    :type sigma: float or Nx1 array
    :param points: points for which function should be evaluated. Use this option _or_ *bins*.
    :type points: 1-D array
    :param center: center of Gaussian
    :type center: float or Nx1 array
    :param limit: range (as multiple of sigma) for cutoff
    :type limit: float

    :returns: numpy array with calculated Gaussian values
    """

    if isinstance(center, float):
        center = np.array([[center]])
    if isinstance(sigma, float):
        sigma = sigma * np.ones_like(center)

    points_matrix = points * np.ones((len(center), 1))

    center_matrix = center * np.ones(len(points))
    sigma_matrix = sigma * np.ones(len(points))

    distances = abs(points_matrix - center_matrix)
    points_close_to_peaks = distances < (limit * sigma_matrix)

    results = np.zeros((len(center), len(points)))
    results[points_close_to_peaks] = function(
        sigma=sigma_matrix[points_close_to_peaks], points=points_matrix[points_close_to_peaks], center=center_matrix[points_close_to_peaks]
    )
    return results


def trunc_and_sum_inplace(
    function=None, function_uses="points", sigma=None, points=None, bins=None, center=None, weights=1, limit=3, method="auto"
):
    """Wrap a simple broadening function, evaluate within a consistent range and sum to spectrum

                      center1                          center2
                         :                                :
                         :                                :
                         *                                :
                        * *                               :
                        * *                               **
                       * |-| sigma1      +               *  *            + ...
                      *     *                          *   |--| sigma2
                    *         *                    . *          * .
                ..: *         * :..             ..:: *          * ::..
             --------------------------     -----------------------
                    |----|                           |----|
                 limit * max(sigma)          limit * max(sigma)

    A spectrum is returned corresponding to the entire input set of
    bins/points, but this is zeroed outside of a restricted range (``limit * sigma``).
    The range is the same for all peaks, so max(sigma) is used for all.
    The purpose of this is performance optimisation compared to evaluating over the whole range.
    There is an artefact associated with this: a vertical step down to zero at the range cutoff.

    The function will be evaluated with the input points or bins
    for each set of (center, sigma) and summed to a single
    spectrum. Two technical approaches are provided: a python for-loop
    which iterates over the functions and sums/writes to the
    appropriate regions of the output array, and a np.histogram dump of
    all the frequency/intensity values. The for-loop approach has a
    more consistent speed but performance ultimately depends on the array sizes.

    :param function: broadening function; this should accept named arguments "center", "sigma" and either "bins" or "points".
    :type function: python function
    :param function_uses: 'points' or 'bins'; select which type of x data is passed to "function"
    :param sigma: widths of broadening functions (passed to "sigma" argument of function)
    :type sigma: float or Nx1 array
    :param bins: sample bins for function evaluation. This _must_ be evenly-spaced.
    :type bins: 1-D array
    :param points: regular grid of points for which function should be evaluated.
    :type points: 1-D array
    :param center: centers of broadening functions
    :type center: float or Nx1 array
    :param weights: weights of peaks for summation
    :type weights: float or array corresponding to "center"
    :param limit: range (as multiple of sigma) for cutoff
    :type limit: float
    :param method:
        'auto', 'histogram' or 'forloop'; select between implementations for summation at appropriate
        frequencies. 'auto' uses 'forloop' when there are > 1500 frequencies
    :type method: str

    :returns: (points, spectrum)
    :returntype: (1D array, 1D array)

    """
    # Select method for summation
    # Histogram seems to be faster below 1500 points; tested on a macbook pro and a Xeon workstation.
    # This threshold may change depending on updates to hardware, Python and Numpy...
    # For now we hard-code the number. It would ideally live in abins.parameters but is very specific to this function.
    if method == "auto" and points.size < 1500:
        sum_method = "histogram"
    elif method == "auto":
        sum_method = "forloop"
    else:
        sum_method = method

    bin_width = bins[1] - bins[0]
    if not np.isclose(points[1] - points[0], bin_width):
        raise ValueError("Bin spacing and point spacing are not consistent")

    freq_range = limit * max(sigma)
    nrows = len(center)
    ncols = int(np.ceil((freq_range * 2) / bin_width))
    if ncols > len(points):
        raise ValueError("Kernel is wider than evaluation region; use a smaller cutoff limit or a larger range of points/bins")

    # Work out locations of frequency blocks. As these should all be the same size,
    # blocks which would exceed array size are "justified" into the array
    #
    # Start by calculating ideal start positions (allowed to exceed bounds) and
    # corresponding frequencies
    #
    # s-|-x----      |
    #   | s---x----  |
    #   |  s---x---- |
    #   |     s---x--|-
    #
    #
    start_indices = np.asarray((center - points[0] - freq_range + (bin_width / 2)) // bin_width, int)
    # Values exceeding calculation range would have illegally large "initial guess" indices so clip those to final point
    #   |            | s---x----     ->       |           s|--x----
    start_indices[start_indices > len(points)] = -1

    start_freqs = points[start_indices]

    # Next identify points which will overshoot left: x lies to left of freq range
    # s-|-x-:--      |
    #   | s-:-x----  |
    #   |  s:--x---- |
    #   |   : s---x--|-
    left_justified = center < freq_range

    # "Left-justify" these points by setting start position to low limit
    #   |sx-------   |
    #   | s---x----  |
    #   |  s---x---- |
    #   |     s---x--|-
    start_freqs[left_justified] = points[0]
    start_indices[left_justified] = 0

    # Apply same reasoning to fix regions overshooting upper bound
    # Note that the center frequencies do not move: only the grid on which they are evaluated
    #   |sx------:   |           |sx-------   |
    #   | s---x--:-  |    --->   | s---x----  |
    #   |  s---x-:-- |           |  s---x---- |
    #   |     s--:x--|-          |   s-----x--|
    right_justified = center > (points[-1] - freq_range)
    start_freqs[right_justified] = points[-1] - (ncols * bin_width)
    start_indices[right_justified] = len(points) - ncols

    # freq_matrix is not used in (bins, forloop) mode so only generate if needed
    if (function_uses == "points") or (function_uses == "bins" and sum_method == "histogram"):
        freq_matrix = start_freqs.reshape(nrows, 1) + np.arange(0, 2 * freq_range, bin_width)

    # Dispatch kernel generation depending on x-coordinate scheme
    if function_uses == "points":
        kernels = function(sigma=sigma, points=freq_matrix, center=center)
    elif function_uses == "bins":
        bin_matrix = start_freqs.reshape(nrows, 1) + np.arange(-bin_width / 2, 2 * freq_range + bin_width / 2, bin_width)
        kernels = function(sigma=sigma, bins=bin_matrix, center=center)
    else:
        raise ValueError('x-basis "{}" for broadening function is unknown.'.format(function_uses))

    # Sum spectrum using selected method
    if sum_method == "histogram":
        spectrum, bin_edges = np.histogram(np.ravel(freq_matrix), bins, weights=np.ravel(weights * kernels), density=False)
    elif sum_method == "forloop":
        spectrum = np.zeros_like(points)
        for start, kernel, weight in zip(start_indices.flatten(), kernels, np.asarray(weights).flatten()):
            scaled_kernel = kernel * weight
            spectrum[start : start + ncols] += scaled_kernel
    else:
        raise ValueError('Summation method "{}" is unknown.', format(method))

    return points, spectrum


def interpolated_broadening(
    sigma=None, points=None, bins=None, center=None, weights=1, is_hist=False, limit=3, function="gaussian", spacing="sqrt2"
):
    """Return a fast estimate of frequency-dependent broadening

    Consider a spectrum of two peaks, in the case where (as in indirect-geometry INS) the peak width
    increases with frequency.

       |
       |        |
       |        |
    -----------------

    In the traditional scheme we broaden each peak individually and combine:

       *                      |                       *
       *        |       +     |        *       =      *        *
      * *       |             |      *   *           * *     *   *
    -----------------      -----------------       -----------------

    Instead of summing over broadening kernels evaluated at each peak, the approximate obtains
    a spectrum corresponding to the following scheme:

    - For each sigma value, the entire spectrum is convolved with an
      appropriate-width broadening function
    - At each frequency, the final spectrum value is drawn from the spectrum
      broadened with corresponding sigma.

    Compared to a summation over broadened peaks, this method introduces an
    asymmetry to the spectrum about each peak.

       *                    *                    *                         *
       *        *          * *       *          * *       *      -->       **       *
      * *       *         *   *     * *       *     *   *   *             *  *     *  *
    ----------------- ,  ----------------- ,  -----------------         -----------------

    This asymmetry should be tolerable as long as the broadening function
    varies slowly in frequency relative to its width.

    The benefit of this scheme is that we do not need to evaluate the
    convolution at every sigma value; nearby spectra can be interpolated.
    Trial-and-error finds that with optimal mixing the error of a Gaussian
    approximated by mixing a wider and narrower Gaussian is ~ 5% when the sigma
    range is factor of 2, and ~ 1% when the sigma range is a factor of sqrt(2).
    A pre-optimised transfer function can be used for a fixed ratio between the
    reference functions.

    :param sigma: widths of broadening functions (passed to "sigma" argument of function)
    :type sigma: float or Nx1 array
    :param bins: sample bins for function evaluation. This _must_ be evenly-spaced.
    :type bins: 1-D array
    :param points: regular grid of points for which function should be evaluated.
    :type points: 1-D array
    :param center: centers of broadening functions
    :type center: float or Nx1 array
    :param weights: weights of peaks for summation
    :type weights: float or array corresponding to "center"
    :param is_hist:
        If "weights" is already a histogram corresponding to evenly-spaced
        frequencies, set this to True to avoid a redundant binning operation.
    :type is_hist: bool
    :param function: broadening function; currently only 'gaussian' is accepted
    :type function: str
    :param limit: range (as multiple of sigma) for cutoff
    :type limit: float
    :param spacing:
        Spacing factor between Gaussian samples on log scale. This is not a
        free parameter as a pre-computed curve is used for interpolation.
        Allowed values: '2', 'sqrt2', with error ~5% and ~1% respectively.
    :type spacing: str

    :returns: (points, spectrum)
    :returntype: (1D array, 1D array)

    """
    mix_functions = {
        "gaussian": {
            "2": {"lower": [-0.1873, 1.464, -4.079, 3.803], "upper": [0.2638, -1.968, 5.057, -3.353]},
            "sqrt2": {"lower": [-0.6079, 4.101, -9.632, 7.139], "upper": [0.7533, -4.882, 10.87, -6.746]},
        }
    }
    log_bases = {"2": 2, "sqrt2": np.sqrt(2)}
    log_base = log_bases[spacing]

    # Sample on appropriate log scale: log_b(x) = log(x) / log(b)
    n_kernels = int(np.ceil(np.log(max(sigma) / min(sigma)) / np.log(log_base))) + 1

    if n_kernels == 1:  # Special case: same width everywhere, only need one kernel
        sigma_samples = np.array([min(sigma)])
    else:
        sigma_samples = log_base ** np.arange(n_kernels) * min(sigma)

    bin_width = bins[1] - bins[0]

    # Get set of convolved spectra for interpolation
    if is_hist:
        hist = weights
    else:
        hist, _ = np.histogram(center, bins=bins, weights=weights, density=False)
    freq_range = limit * max(sigma)
    kernel_npts_oneside = np.ceil(freq_range / bin_width)

    if function == "gaussian":
        kernels = mesh_gaussian(
            sigma=sigma_samples[:, np.newaxis], points=np.arange(-kernel_npts_oneside, kernel_npts_oneside + 1, 1) * bin_width, center=0
        )
    else:
        raise ValueError('"{}" kernel not supported for "interpolate" broadening method.'.format(function))

    spectra = np.array([convolve(hist, kernel, mode="same") for kernel in kernels])

    # Interpolate with parametrised relationship
    sigma_locations = np.searchsorted(sigma_samples, sigma)  # locations in sampled values of points from sigma
    spectrum = np.zeros_like(points)
    # Samples with sigma == min(sigma) are a special case: copy directly from spectrum
    spectrum[sigma_locations == 0] = spectra[0, sigma_locations == 0]

    for i in range(1, len(sigma_samples)):
        masked_block = sigma_locations == i
        sigma_factors = sigma[masked_block] / sigma_samples[i - 1]
        lower_mix = np.polyval(mix_functions[function][spacing]["lower"], sigma_factors)
        upper_mix = np.polyval(mix_functions[function][spacing]["upper"], sigma_factors)

        spectrum[masked_block] = lower_mix * spectra[i - 1, masked_block] + upper_mix * spectra[i, masked_block]

    return points, spectrum
