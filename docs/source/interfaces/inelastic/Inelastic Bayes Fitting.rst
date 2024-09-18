.. _interface-inelastic-bayes-fitting:

Inelastic Bayes Fitting
=======================

.. contents:: Table of Contents
  :local:

Overview
--------

Provides Bayesian analysis routines primarily for use with QENS data.

.. interface:: Bayes Fitting
  :align: right
  :width: 350

Action Buttons
--------------

Settings
  Opens the :ref:`Settings <inelastic-interface-settings>` GUI which allows you to
  customize the settings for the Indirect interfaces.

?
  Opens this help page.

Manage Directories
  Opens the Manage Directories dialog allowing you to change your search directories
  and default save directory and enable/disable data archive search.

ResNorm
-------

This tab creates a group 'normalisation' file by taking a resolution file and
fitting it to all the groups in the resolution (vanadium) data file which has
the same grouping as the sample data of interest.

The routine fits the width of the resolution file to give a 'stretch factor'
and the area provides an intensity normalisation factor.

The fitted parameters are in the group workspace with suffix *_ResNorm* with
additional suffices of *_Intensity* & *_Stretch*.

The processing on this tab is provided by the :ref:`ResNorm <algm-ResNorm>`
algorithm.

.. interface:: Bayes Fitting
  :widget: ResNorm

Options
~~~~~~~

Vanadium File
  Either a reduced file created using the Energy Transfer tab or an
  :math:`S(Q, \omega)` file.

Resolution File
  A resolution file created using the Calibrtion tab.

EMin & EMax
  The energy range to perform fitting within.

Preview Spectrum
  Changes the spectrum displayed in the preview plot.

Plot Current Preview
  Plots the currently selected preview plot in a separate external window

Run
  Runs the processing configured on the current tab.

Plot
  Plots the selected parameter stored in the result workspaces.

Save Result
  Saves the result in the default save directory.

Quasi
-----

.. warning:: This interface is only available on Windows

The model that is being fitted is that of a :math:`\delta`-function (elastic component)
of amplitude :math:`A(0)` and Lorentzians of amplitude :math:`A(j)` and HWHM
:math:`W(j)` where :math:`j=1,2,3`. The whole function is then convolved with
the resolution function. The -function and Lorentzians are intrinsically
normalised to unity so that the amplitudes represent their integrated areas.

For a Lorentzian, the Fourier transform does the conversion:
:math:`1/(x^{2}+\delta^{2}) \Leftrightarrow exp[-2\pi(\delta k)]`.  If :math:`x`
is identified with energy :math:`E` and :math:`2\pi k` with :math:`t/\hbar`
where t is time then: :math:`1/[E^{2}+(\hbar / \tau)^{2}] \Leftrightarrow exp[-t
/\tau]` and :math:`\sigma` is identified with :math:`\hbar / \tau`.  The program
estimates the quasielastic components of each of the groups of spectra and
requires the resolution file and optionally the normalisation file created by
ResNorm.

For a Stretched Exponential, the choice of several Lorentzians is replaced with
a single function with the shape : :math:`\psi\beta(x) \Leftrightarrow
exp[-2\pi(\sigma k)\beta]`. This, in the energy to time FT transformation, is
:math:`\psi\beta(E) \Leftrightarrow exp[-(t/\tau)\beta]`. So :math:`\sigma` is
identified with :math:`(2\pi)\beta\hbar/\tau` .  The model that is fitted is
that of an elastic component and the stretched exponential and the program gives
the best estimate for the :math:`\beta` parameter and the width for each group
of spectra.

.. interface:: Bayes Fitting
  :widget: Quasi

Options
~~~~~~~

Sample
  Either a reduced file created using the Energy Transfer tab or an
  :math:`S(Q, \omega)` file.

Resolution
  A resolution file created using the Calibration tab.

Program
  The curve fitting program to use.

Background
  The background fitting program to use.

Elastic Peak
  If an elastic peak should be used.

Sequential Fit
  Enables multiple fitting iterations.

Fix Width
  Allows selection of a width file.

Use ResNorm
  Allows selection of a ResNorm output file or workspace to use with fitting.

EMin & EMax
  The energy range to perform fitting within.

Sample Binning
  Sample binning to use.

Resolution Binning
  Resolution binning to use.

Preview Spectrum
  Changes the spectrum displayed in the preview plot.

Plot Current Preview
  Plots the currently selected preview plot in a separate external window

Run
  Runs the processing configured on the current tab.

Plot
  Plots the selected parameter stored in the result workspaces.

Save Result
  Saves the result in the default save directory.

Stretch
-------

.. warning:: This interface is only available on Windows

This is a variation of the stretched exponential option of Quasi. For each
spectrum, a fit is performed for a grid of β and σ values. The distribution of
goodness of fit values is plotted.

.. interface:: Bayes Fitting
  :widget: Stretch

Options
~~~~~~~

Sample
  Either a reduced file created using the Energy Transfer tab or an
  :math:`S(Q, \omega)` file.

Resolution
  A resolution file created using the Calibration tab.

Background
  The background fitting program to use.

Elastic Peak
  If an elastic peak should be used.

Sequential Fit
  Enables multiple fitting iterations.

EMin & EMax
  The energy range to perform fitting within.

Sample Binning
  Sample binning to use.

Sigma
  Value of Sigma to use.

Beta
  Value of Beta to use.

Preview Spectrum
  Changes the spectrum displayed in the preview plot.

Plot Current Preview
  Plots the currently selected preview plot in a separate external window

Run
  Runs the processing configured on the current tab.

Plot
  Plots the selected parameter stored in the result workspaces.

Plot Contour
  Produces a contour plot of the selected workspace.

Save Result
  Saves the result in the default save directory.

.. categories:: Interfaces Inelastic
