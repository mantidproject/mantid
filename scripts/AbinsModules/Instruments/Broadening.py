
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import AbinsModules
import numpy as np

def broaden_spectrum(frequencies=None, bins=None, s_dft=None, sigma=None, scheme='legacy', prebin='auto'):
    """Convert frequency/S data to broadened spectrum on a regular grid

    Several algorithms are implemented, for purposes of
    performance/accuracy optimisation and demonstration of legacy
    behaviour.

    :param frequencies: input frequency series; these may be irregularly spaced
    :type frequencies: 1D array-like
    :param bins: Evenly-spaced frequency bin values for the output spectrum. If *frequencies* is None, s_dft is assumed to correspond to mid-point frequencies on this mesh.
    :type bins: 1D array-like
    :param s_dft: scattering values corresponding to *frequencies*
    :type s_dft: 1D array-like
    :param sigma: width of broadening function. This may be a scalar used over the whole spectrum, or a series of values corresponding to *frequencies*.
    :type sigma: float or 1D array-like
    :param scheme:
        Name of broadening method used. Options:

        - legacy: Implementation of the legacy Abins method (sum over
              Gaussian kernels with fixed number of samples). Not
              recommended for production calculations due to aliasing on
              non-commensurate output grid. Values are slightly inconsistent
              with older versions due to change in sampling grid convention.
    :type scheme: str
    :returns: (freq_points, broadened_spectrum)

    The *freq_points* are the mid-bin frequency values which
    nominally correspond to *broadened_spectrum*; both of these are
    1D arrays of length (N-1) corresponding to the length-N *bins*.
    """
    if (bins is None) or (s_dft is None) or (sigma is None):
        raise ValueError("Frequency bins, S data and broadening width must be provided.")

    # if scheme not in ('legacy', 'gaussian', 'gaussian_trunc', 'gaussian_trunc_forloop'):
    #     raise ValueError('Broadening scheme "{}" not supported for this instrument, please correct '
    #                      'AbinsParameters.sampling["broadening_scheme"]'.format(scheme))

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

    if scheme == 'legacy':
        fwhm = AbinsModules.AbinsParameters.instruments['fwhm']
        pkt_per_peak = AbinsModules.AbinsParameters.sampling['pkt_per_peak']
        def multi_linspace(start, stop, npts):
            """Given 1D length-N start, stop and scalar npts, get (npts, N) linspace-like matrix"""
            ncols = len(start)
            step = (stop - start) / (npts - 1)
            step_matrix = step.reshape((ncols, 1)) * np.ones((1, npts))
            start_matrix = start.reshape((ncols, 1)) * np.ones((1, npts))
            return (np.arange(npts) * step_matrix) + start_matrix
        broad_freq = multi_linspace(frequencies - fwhm * sigma,
                                    frequencies + fwhm * sigma,
                                    pkt_per_peak)
        ncols = len(frequencies)
        if np.asarray(sigma).shape == ():
            sigma = np.full(ncols, sigma)
        sigma_matrix = sigma.reshape(ncols, 1) * np.ones((1, pkt_per_peak))
        freq_matrix = frequencies.reshape(ncols, 1) * np.ones((1, pkt_per_peak))
        s_dft_matrix = s_dft.reshape(ncols, 1) * np.ones((1, pkt_per_peak))
        dirac_val = gaussian(sigma=sigma_matrix, points=broad_freq, center=freq_matrix)

        flattened_spectrum = np.ravel(s_dft_matrix * dirac_val)
        flattened_freq = np.ravel(broad_freq)

        hist, bin_edges = np.histogram(flattened_freq, bins, weights=flattened_spectrum, density=False)
        return freq_points, hist

    elif scheme == 'gaussian':
        #  Gaussian broadening on the full grid (no range cutoff, sum over all peaks)
        kernels = gaussian(sigma=sigma,
                           points=freq_points.reshape(len(freq_points), 1),
                           center=frequencies)
        spectrum = np.dot(kernels, s_dft)
        return freq_points, spectrum

    elif scheme == 'gaussian_trunc':
        # Gaussian broadening on the full grid (sum over all peaks, Gaussian range limited to 3 sigma)
        kernels = gaussian_trunc(sigma=sigma,
                                 points=freq_points,
                                 center=frequencies)
        spectrum = np.dot(kernels.transpose(), s_dft)
        return freq_points, spectrum

    elif scheme == 'gaussian_trunc_windowed':
        # Gaussian broadening on the full grid (sum over all peaks, Gaussian range limited to 3 sigma)
        # All peaks use the same number of points; if the center is located close to the low- or high-freq limit,
        # the frequency point set is "justified" to lie inside the range

        bin_width = bins[1] - bins[0]
        freq_range = 3 * max(sigma)
        nrows = len(frequencies)
        ncols = int(np.ceil((freq_range * 2) / bin_width))

        start_indices = np.asarray((frequencies - freq_points[0] - freq_range + (bin_width / 2)) // bin_width,
                                    int)

        start_freqs = freq_points[start_indices]

        left_justified = frequencies < freq_range
        start_freqs[left_justified] = freq_points[0]
        start_indices[left_justified] = 0

        right_justified = frequencies > (freq_points[-1] - freq_range)
        start_freqs[right_justified] = freq_points[-1] - (ncols * bin_width)
        start_indices[right_justified] = len(freq_points) - ncols

        freq_matrix = start_freqs.reshape(nrows, 1) + np.arange(0, 2 * freq_range, bin_width)

        kernels = gaussian(sigma=sigma.reshape(nrows, 1),
                                points=freq_matrix,
                                center=frequencies.reshape(nrows, 1))

        hist, bin_edges = np.histogram(np.ravel(freq_matrix),
                                       bins,
                                       weights=np.ravel(s_dft.reshape(nrows, 1) * kernels),
                                       density=False)
        return freq_points, hist

    elif scheme == 'gaussian_trunc_forloop':
        # Gaussian broadening on the full grid (sum over all peaks, Gaussian range limited to 3 sigma)
        # All peaks use the same number of points; if the center is located close to the low- or high-freq limit,
        # the frequency point set is "justified" to lie inside the range
        # The final summation uses a for loop instead of np.histogram

        bin_width = bins[1] - bins[0]
        freq_range = 3 * max(sigma)
        nrows = len(frequencies)
        ncols = int(np.ceil((freq_range * 2) / bin_width))

        start_indices = np.asarray((frequencies - freq_points[0] - freq_range + (bin_width / 2)) // bin_width,
                                    int)

        start_freqs = freq_points[start_indices]

        left_justified = frequencies < freq_range
        start_freqs[left_justified] = freq_points[0]
        start_indices[left_justified] = 0

        right_justified = frequencies > (freq_points[-1] - freq_range)
        start_freqs[right_justified] = freq_points[-1] - (ncols * bin_width)
        start_indices[right_justified] = len(freq_points) - ncols

        freq_matrix = start_freqs.reshape(nrows, 1) + np.arange(0, 2 * freq_range, bin_width)

        kernels = gaussian(sigma=sigma.reshape(nrows, 1),
                           points=freq_matrix,
                           center=frequencies.reshape(nrows, 1))

        spectrum = np.zeros_like(freq_points)
        for start, kernel, s in zip(start_indices, kernels, s_dft):
            scaled_kernel = kernel * s
            spectrum[start:start+ncols] += scaled_kernel

        return freq_points, spectrum

    elif scheme == 'conv_15':
        # Not a real scheme, just for testing. Convolve with a fixed sigma of 15.
        from scipy.signal import convolve
        sigma = 15
        bin_width = bins[1] - bins[0]
        hist, bin_edges = np.histogram(frequencies, bins=bins, weights=s_dft, density=False)
        freq_range = 3 * sigma
        kernel_npts_oneside = np.ceil(freq_range / bin_width)
        kernel = gaussian(sigma=15,
                               points=np.arange(-kernel_npts_oneside, kernel_npts_oneside + 1, 1) * bin_width,
                               center=0)
        spectrum = convolve(hist, kernel, mode='same')
        return freq_points, spectrum

    elif scheme == 'interpolate':
        from scipy.signal import convolve
        # Use a log 2 base for now: get the range to cover
        n_kernels = int(np.ceil(np.log2(max(sigma) / min(sigma)))) + 1
        if n_kernels == 1:
            sigma_samples = np.array([min(sigma)])
        else:
            sigma_samples = 2**np.arange(n_kernels + 1) * min(sigma)

        # Get set of convolved spectra for interpolation
        bin_width = bins[1] - bins[0]
        hist, bin_edges = np.histogram(frequencies, bins=bins, weights=s_dft, density=False)
        freq_range = 3 * max(sigma)
        kernel_npts_oneside = np.ceil(freq_range / bin_width)
        kernels = gaussian(sigma=sigma_samples[:, np.newaxis],
                                points=np.arange(-kernel_npts_oneside, kernel_npts_oneside + 1, 1) * bin_width,
                                center=0)

        spectra = np.array([convolve(hist, kernel, mode='same') for kernel in kernels])

        # Interpolate with parametrised relationship
        sigma_locations = np.searchsorted(sigma, sigma_samples) # locations in full sigma of sampled values
        spectrum = np.zeros_like(freq_points)
        for sample_i, left_sigma_i, right_sigma_i in zip(range(len(sigma_samples)),
                                                         sigma_locations[:-1],
                                                         sigma_locations[1:]):
            sigma_chunk = sigma[left_sigma_i:right_sigma_i]
            if len(sigma_chunk) == 0:
                break   # This happens at the end of iteration; we get a repeat of the last position

            sigma_factors = sigma_chunk / sigma_chunk[0]
            left_mix = np.polyval([-0.1873, 1.464, -4.079, 3.803], sigma_factors)
            right_mix = np.polyval([0.2638, -1.968, 5.057, -3.353], sigma_factors)
            spectrum[left_sigma_i:right_sigma_i] = (left_mix * spectra[sample_i, left_sigma_i:right_sigma_i]
                                                    + right_mix * spectra[sample_i + 1, left_sigma_i:right_sigma_i])
        return freq_points, spectrum

def gaussian(sigma=None, points=None, center=None):
    """
    :param sigma: sigma defines width of Gaussian
    :param points: points for which Gaussian should be evaluated
    :param center: center of Gaussian
    :returns: numpy array with calculated Gaussian values
    """
    two_sigma = 2.0 * sigma
    sigma_factor = two_sigma * sigma
    norm = (AbinsModules.AbinsParameters.sampling['pkt_per_peak']
            / (AbinsModules.AbinsParameters.instruments['fwhm'] * two_sigma))
    return np.exp(-(points - center) ** 2 / sigma_factor) / (np.sqrt(sigma_factor * np.pi) * norm)

def gaussian_trunc(sigma=None, points=None, center=None, limit=3):
    """
    :param sigma: sigma defines width of Gaussian
    :param points: points for which Gaussian should be evaluated. These must be evenly-spaced for correct normalisation.
    :param center: center of Gaussian
    :returns: numpy array with calculated Gaussian values
    """
    points_matrix = points * np.ones((len(center), 1))
    center_matrix = center.reshape(len(center), 1) * np.ones(len(points))
    sigma_matrix = sigma.reshape(len(sigma), 1) * np.ones(len(points))

    distances = abs(points_matrix - center_matrix)
    samples_close_to_peaks = distances < (limit * sigma_matrix)

    results = np.zeros((len(center), len(points)))

    two_sigma = 2.0 * sigma_matrix[samples_close_to_peaks]
    sigma_factor = two_sigma * sigma_matrix[samples_close_to_peaks]

    results[samples_close_to_peaks] = np.exp(-(points_matrix[samples_close_to_peaks] - center_matrix[samples_close_to_peaks]) ** 2 / sigma_factor) / (np.sqrt(sigma_factor * np.pi))

    return results

def normal_trunc(bins=None, sigma=None, center=0):
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
    bin_contents = integral_at_bin_edges[1:] - integral_at_bin_edges[:-1]

    # This integral is guaranteed to have the _values_ sum to 1. Divide by bin-width to get consistent _area_ 1.
    bin_width = bins[1] - bins[0]

    return bin_contents / bin_width
