.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm calculates a single frequency spectrum from the time domain spectra recorded by multiple groups/detectors.

If a group contains zero counts (i.e. the detectors are dead) then they are excluded from the frequency calculation. In the outputs these groups record the phase and asymmetry as zero and :math:`999` respectively.

The time domain data :math:`D_k(t)`, where :math:`t` is time and :math:`k` is the spectrum number, has associated errors :math:`E_k(t)`. If the number of points chosen is greater than the number of time domain data points then extra points are
added with infinite errors. The time domain data prior to :code:`FirstGoodTime` also have their errors set to infinity. The algorithm will produce the frequency spectra :math:`f(\omega)` and this is assumed to be real and positive.
The upper limit of the frequency spectra is determined by :code:`MaxField`. The maximum frequency, :math:`\omega_\mathrm{max}` can be less than the Nyquist limit :math:`\frac{\pi}{\delta T}` if the instrumental frequency response function for
:math:`\omega>\omega_\mathrm{max}` is approximately zero. The initial estimate of the frequency spectrum is flat.

The algorithm calculates an estimate of each time domain spectra, :math:`g_k(t)` by the equation

.. math::  g_k(t)=(1+A_k \Re(\mathrm{IFFT}(f(\omega) R(\omega))\exp(-j\phi_k) ) ),

where :math:`\Re(z)` is the real part of :math:`z`, :math:`\mathrm{IFFT}` is the inverse fast Fourier transform (as defined by `numpy
<https://docs.scipy.org/doc/numpy-1.12.0/reference/routines.fft.html#module-numpy.fft>`_), :math:`\phi_k` is the phase and :math:`A_k` is the asymmetry of the of the  :math:`k^\mathrm{th}` spectrum.
The asymmetry is normalised such that :math:`\sum_k A_k = 1`.
The instrumental frequency response function, :math:`R(\omega)`, is  is in general complex (due to a
non-symmetric pulse shape) and is the same for all spectra. The values of the phases and asymmetries are fitted in the outer loop of the algorithm.

The :math:`\chi^2` value is calculated via the equation

.. math:: \chi^2 = F\frac{\sum_{k,t} (D_k(t)-g_k(t))^2 }{E_k(t)^2},

where :math:`F` is the :code:`Factor` and is of order 1.0 (but can be adjusted by the user at the start of the algorithm for a better fit).
The entropy is given by

.. math:: S = - \sum_\omega f(\omega) \log\left(\frac{f(\omega)}{A}\right),

where :math:`A` is the :code:`DefaultLevel`; it is a parameter of the entropy function. It has a number of names in the literature, one of which
is default-value since the maximum entropy solution with no data is :math:`f(\omega)=A` for all :math:`\omega`. The algorithm maximises
:math:`S-\chi^2` and it is seen from the definition of :code:`Factor` above that this algorithm property acts a Lagrange multiplier, i.e. controlling the value :math:`\chi^2` converges to.


Usage
-----

.. testcode::

  # load data
  Load(Filename='MUSR00022725.nxs', OutputWorkspace='MUSR00022725')
  # estimate phases
  CalMuonDetectorPhases(InputWorkspace='MUSR00022725', FirstGoodData=0.10000000000000001, LastGoodData=16, DetectorTable='phases', DataFitted='fitted', ForwardSpectra='9-16,57-64', BackwardSpectra='25-32,41-48')
  MuonMaxent(InputWorkspace='MUSR00022725', InputPhaseTable='phases', Npts='16384', OuterIterations='9', InnerIterations='12', DefaultLevel=0.11, Factor=1.03, OutputWorkspace='freq', OutputPhaseTable='phasesOut', ReconstructedSpectra='time')
  # get data
  freq = AnalysisDataService.retrieve("freq")
  print('frequency values {:.3f} {:.3f} {:.3f} {:.3f} {:.3f}'.format(freq.readY(0)[5], freq.readY(0)[690],freq.readY(0)[700], freq.readY(0)[710],freq.readY(0)[900]))

Output
######

.. testoutput::

  frequency values 0.110 0.789 0.871 0.821 0.105

.. categories::

.. sourcelink::
