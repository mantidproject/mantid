.. _AbinsImplementation:

Abins: Implementation details
=============================


Introduction
------------

Abins is a relatively complex algorithm with some unique
infrastructure and built-in design decisions. Details are collected
here: this is primarily for the benefit of those developing or
hacking Abins.

Sampling
--------

While the scattering cross-sections are calculated for individual
events and enumerated combinations of events, the output spectra are
histograms which have been resampled and broadened on a finite grid.

The data range is determined by three parameters:

- *min_wavenumber*, set in ``AbinsParameters.sampling``
- *max_wavenumber*, set in ``AbinsParameters.sampling``
- *bin_width*, a parameter set in the main Abins algorithm interface
  and passed as an argument to internal functions as appropriate. This
  parameter is more exposed than the energy range, as a convenient to
  the user. However, it should be applied consistently within a single
  application of Abins and functions may assume that this value was
  also used elsewhere.

These parameters are used to establish the edges of the sample *bins*;
the largest value is rounded up from *max_wavenumber* to contain a
whole number of *bin_width*.

The histogram of data sampled in these N bins has N-1 "points", which
should be aligned with the corresponding bin centres if plotted as a
line. In the code this array of frequency coordinates is generally
named _freq_points_.

Broadening
----------

Instrumental broadening in Abins involves an energy-dependent
resolution function; in the implemented case (the TOSCA instrument)
this is convolution with a Gaussian kernel with the energy-dependent
width parameter (sigma) determined by a quadratic polynomial.
Convolution with a varying kernel is not a common operation and is
implemented within AbinsModules.

Earlier versions of Abins implemented a Gaussian kernel with a
fixed number of points spread over a range scaled to the peak width,
set by *pkt_per_peak* and *fwhm* parameters in
``AbinsParameters.sampling``. This method leads to aliasing when the
x-coordinates are not commensurate with the histogram bin.

.. image:: ../images/gaussian_aliasing.png
    :align: center

The safest way to avoid such trouble is for all broadening methods to
output onto the regular _freq_points_ grid. A number of broadening
implementations have been provided in
_AbinsModules.Instruments.Broadening_. It is up to the Instrument
logic to dispatch broadening calls to the requested implementation,
and it is up to specific Instruments to select an appropriate scheme
for their needs.
The advanced parameter *AbinsParameters.sampling['broadening_scheme']*
is made available so that this can be overruled, but it is up to the
Instrument to interpret this information. 'auto' should select an
intelligent scheme and inappropriate methods can be forbidden.

Fast approximate broadening with "interpolate"
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The "interpolate" scheme estimates broadened spectra using a limited
number of kernel evaluations or convolution steps in order to reduce
the computation time. The method appears to be novel, so some
explanation is needed.

Consider first that we can approximate a Gaussian function by a linear
combination of two other Gaussians; one narrower and one wider. If the
mixing parameters are linearly based on the relationship between the
widths the results are not impressive:

.. image:: ../images/gaussian_mix_linear.png

But if we optimise the mixing parameter at each width then the
magnitudes improve significantly, even if the shapes remain distinctly non-Gaussian:

.. image:: ../images/gaussian_mix_optimal_scale4.png

This error is closely related to the width difference between the
endpoints. Here the range is reduced from a factor 4 to a factor 2,
and the resulting functions are visually quite convincing

.. image:: ../images/gaussian_mix_optimal_scale2.png

while a gap of :math:`\sqrt{2}` is practically indistinguishable with error below 1% of the peak maximum.

.. image:: ../images/gaussian_mix_optimal_scale_sqrt2.png

For TOSCA :math:`\sigma = a f^2 + b f + c` where :math:`a, b, c$ = $10^{-7}, 0.005, 2.5`. For an energy range of 32 cm\ :sup:`-1` to 4100 cm\ :sup:`-1` sigma ranges from 2.66 to 24.68, which could covered by 5 Gaussians separated by width factor 2 or 9 Gaussians seperated by width factor :math:`\sqrt{2}`.
This could present a significant cost saving compared to full evaluation of ~4000 convolution kernels (one per convolution bin).

Alternatively we can perform convolution of the full spectrum with each of the sampled kernels, and then interpolate *between the spectra* using the predetermined mixing weights. The convolution is performed efficiently using FFTs, and relatively little memory is required to hold this limited number of spectra and interpolate between them.

NEW FIGURE NEEDED: ILLUSTRATE INTERP BETWEEN SPECTRA

This procedure is not strictly equivalent to a summation over frequency-dependent functions, even if there is no interpolation error.
At each energy coordinate :math:`\epsilon` we "see" a fragment of full spectrum convolved at the same width as any points at :math:`\epsilon` would be.
In a typical indirect INS spectrum which becomes broader at high energy, this would overestimate the contribution from peaks originating below this :math:`\epsilon` and underestimate the contribution from peaks originating above :math:`\epsilon`.
As a result, peaks will appear asymmetric.
In practice, the magnitude of this error depends on the rate of change of :math:`\sigma` relative to the size of :math:`\sigma`.
In the case of the TOSCA parameters, the error is very small. This should be re-evaluated for other instruments with different energy-dependent broadening functions.

.. image:: ../images/abins-interpolation-benzene.png

We can see the artefacts of this approach more clearly if we use a wider sample spacing (factor 2) and zoom in on the spectrum. The interpolation method has a tendency to show small peaks at turning points; this may be related to the imperfection in the shape of the smooth bell.

.. image:: ../images/abins-interpolation-zoom.png
           
Deprecation plans
-----------------

- The *pkt_per_peak* and *fwhm* parameters in
  ``AbinsParameters.sampling`` are no longer in use and should be
  removed in a future release.

- The "SingleCrystal" modules and objects support non-existent
  functionality and should be removed. They may return when that
  functionality is added, but it is likely that their interfaces will
  change in the process.

- The *frequencies_threshold* parameter in
  ``AbinsParameters.sampling`` is currently non-functional and should
  be removed until it *is* functional.



.. Source code for Gaussian interpolation plots

    import numpy as np
    from scipy.optimize import curve_fit
    import matplotlib.pyplot as plt
    from matplotlib.lines import Line2D

    def gaussian(x, sigma=2, center=0):
        g = np.exp(-0.5 * ((x - center) / sigma)**2) / (sigma * np.sqrt(2 * np.pi))
        return g

    margin = 0.05

    def plot_linear_interp(filename='gaussian_mix_linear.png'):
        """Plot linearly-interpolated Gaussians with wide sigma range"""

        g1_center = 0
        g2_center = 40
        sigma_max = 4
        sigma_min = 1

        x = np.linspace(-10, 50, 401)

        fig, ax = plt.subplots()
        for sigma, color in zip(np.linspace(sigma_min, sigma_max, 5),
                                ['C0', 'C1', 'C2', 'C3', 'C4']):
            center = (g1_center
                      + ((sigma - sigma_min)
                         * (g2_center - g1_center) / (sigma_max - sigma_min)))
            ax.plot(x, gaussian(x, sigma=sigma, center=center), c=color)

            low_ref = gaussian(x, sigma=sigma_min, center=center)
            high_ref = gaussian(x, sigma=sigma_max, center=center)
            mix = (sigma - sigma_min) / (sigma_max - sigma_min)
            ax.plot(x, (1 - mix) * low_ref + mix * high_ref,
                    c=color, linestyle='--')

        ax.set_xlim([-10, 50])
        ax.set_ylim([0, None])
        ax.tick_params(labelbottom=False, labelleft=False)

        custom_lines = [Line2D([0], [0], color='k', linestyle='-', lw=2),
                        Line2D([0], [0], color='k', linestyle='--', lw=2)]

        ax.legend(custom_lines, ['Exact', 'Linear interpolation'])
        fig.subplots_adjust(left=margin, bottom=margin,
                            right=(1 - margin), top=(1 - margin))

        fig.savefig(filename)

    def plot_optimised_interp(filename='gaussian_mix_optimal_scale4.png',
                              sigma_max=4):
        g1_center = 0
        g2_center = 40
        sigma_min = 1

        x = np.linspace(-10, 10, 101)
        npts = 7

        fig, [ax1, ax2, ax3] = plt.subplots(nrows=3,
                                            sharex=True,
                                            gridspec_kw={
                                                'height_ratios': [3, 1, 1]})
        mix1_list, mix2_list = [], []

        def gaussian_mix(x, w1, w2):
            """Return a linear combination of two Gaussians with weights"""
            return (w1 * gaussian(x, sigma=sigma_min)
                    + w2 * gaussian(x, sigma=sigma_max))


        for sigma, color in zip(np.linspace(sigma_min, sigma_max, npts),
                                ['C0', 'C1', 'C2', 'C3', 'C4', 'C5', 'C6']):
            ydata = gaussian(x, sigma=sigma)
            (mix1, mix2), _ = curve_fit(gaussian_mix, x, ydata, p0=[0.5, 0.5])

            x_offset = (g1_center
                        + ((sigma - sigma_min)
                           * (g2_center - g1_center) / (sigma_max - sigma_min)))
            actual = gaussian(x, sigma=sigma)
            est = gaussian_mix(x, mix1, mix2)
            rms = np.sqrt(np.mean((actual - est)**2))
            ax1.plot(x + x_offset, actual, color=color)
            ax1.plot(x + x_offset, est, color=color, linestyle='--')
            ax2.plot([x_offset], [rms], 'o', c='C0')

            mix1_list.append(mix1)
            mix2_list.append(mix2)


        custom_lines = [Line2D([0], [0], color='k', linestyle='-', lw=2),
                        Line2D([0], [0], color='k', linestyle='--', lw=2)]

        ax1.legend(custom_lines, ['Exact', 'Optimised interpolation'])

        ax2.set_ylabel('RMS error')

        ax3.plot(np.linspace(g1_center, g2_center, npts), mix1_list)
        ax3.plot(np.linspace(g1_center, g2_center, npts), mix2_list)
        ax3.set_ylabel('Weights')
        ax3.set_ylim([0, 1])

        fig.savefig(filename)

    if __name__ == '__main__':
        plot_linear_interp()
        plot_optimised_interp()
        plot_optimised_interp(filename='gaussian_mix_optimal_scale2.png',
                              sigma_max=2)
        plot_optimised_interp(filename='gaussian_mix_optimal_scale_sqrt2.png',
                              sigma_max=np.sqrt(2))
