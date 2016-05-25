.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Calculates tables of the Fourier transform of the stretched exponential with
:math:`\hbar \equiv 1`, relaxation time :math:`\tau \equiv 1`,
and for different values of the stretching exponent :math:`\beta`.

:math:`f(u,\beta) = \int_{0}^{\infty} ds \cdot cos(us) e^{-s^\beta}
(=\int_{-\infty}^{\infty} \frac{dt}{\hbar} e^{-i\frac{Et}{\hbar}}
e^{-(\frac{|t|}{\tau})^\beta})`

where :math:`s=\frac{t}{\tau}` is the reduced time and
:math:`u=\frac{E\tau}{\hbar}` is the reduced energy. This algorithm
will compute :math:`f(u,\beta)` for a set of :math:`(u,\beta)` points.

:math:`f(u,\beta)` is normalized over the energy-axis:
:math:`\int_{-\infty}^{\infty} dE f(u,\beta) =
\int_{-\infty}^{\infty} dE \int_{-\infty}^{\infty} \frac{dt}{\hbar}
e^{-i\frac{Et}{\hbar}} e^{-(\frac{|t|}{\tau})^\beta} =
\int_{-\infty}^{\infty} \frac{dt}{\hbar} \hbar\delta(t)
e^{-(\frac{|t|}{\tau})^\beta} \equiv 1`

Table properties 1. Sampling
============================

The range of reduced energies spans many orders of magnitude:

:math:`u_{min} \simeq 0.1\mu eV \cdot 1ps / \hbar \simeq 10^{-5}`

:math:`u_{max} \simeq 10meV \cdot 100ns / \hbar \simeq 10^{5}`

Evaluating :math:`f(u,\beta)` on a regular grid in the u-axis
from :math:`0` to :math:`u_{max}` with spacing
:math:`\delta u \equiv u_{min}` is non-achievable. Instead, the
algorithm evaluates :math:`f(u,\beta)` on a set of reduced energies
that will produce a semi-regular grid in the range
:math:`[f_{min}\equiv f(u_{max},\beta), f_{max}\equiv f(u_{min},\beta)]`
and with spacing
:math:`\delta f = \frac{f_{max}-f_{min}}{evalN}`.

.. figure:: /images/TabulateStretchedExpFT_1.png

Table properties 2. Integral convergence
========================================

For large values of the reduced energy :math:`u`, the :math:`cos(us)` term
rapidly changes sign as the integral is carried out in the reduced
time :math:`s`. As a consequence, the integral changes value and even
sign upon minimal changes in the range of integration.
In this scenario, the range of integration in :math:`s`
is divided into intervals of length :math:`2\pi`, and each interval is
subdivided into a regular grid of spacing :math:`\delta s = 2\pi/2^k`.
This division ensures that :math:`cos(us)` takes on :math:`2^{k-1}`
positive values on each :math:`2\pi`-interval, and the same values
with negative sign. This scheme ensures convergence.

.. figure:: /images/TabulateStretchedExpFT_2.png
In the figure, :math:`cos(us)` is evaluated on :math:`2^3=8` points.

Usage
-----
**Example - Tables for one value of the stretching exponent.**

.. testcode:: ExTablesOneBeta
  import os
  # Create a file path in the user home directory
  filePath = os.path.expanduser('~/tablesOnebeta.h5')

  ws = TabulateStretchedExpFT(BetaValues=[0.2, 0.1, 0.3],
                              EvalN=2, Taumax=1000.0, Emax=1.0,
                              OutputWorkspace="tables",
                              TablesFileName=filePath)

.. testcleanup::

  os.remove(filePath)
  DeleteWorkspace(ws)

.. categories::

.. sourcelink::

