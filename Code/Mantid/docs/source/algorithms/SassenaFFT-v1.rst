.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The `Sassena <http://sassena.org>`__  application generates
intermediate scattering factors from molecular dynamics trajectories.
This algorithm reads Sassena output and stores all data in workspaces of
type :ref:`Workspace2D <Workspace2D>`, grouped under a single
:ref:`WorkspaceGroup <WorkspaceGroup>`. It is implied that the time unit is
one **picosecond**.

Sassena ouput files are in `HDF5 <http://www.hdfgroup.org/HDF5>`__ format, and can be made up of the
following datasets: *qvectors*, *fq*, *fq0*, *fq2*, and *fqt*

The group workspace should contain workspaces **\_fqt.Re** and
**\_fqt.Im** containing the real and imaginary parts of the intermediate
structure factor, respectively. This algorithm will take both and
perform :ref:`algm-FFT`, storing the real part of the transform in
workspace **\_fqw** and placing this workspace under the input group
workspace. Assuming the time unit to be one picosecond, the resulting
energies will be in units of one **micro-eV**.

The Schofield correction (P. Schofield, *Phys. Rev. Letters* **4**\ (5),
239 (1960)) is optionally applied to the resulting dynamic structure
factor to reinstate the detailed balance condition
:math:`S(Q,\omega)=e^{\beta \hbar \omega}S(-Q,-\omega)`.

Details
-------

Parameter FFTonlyRealPart
#########################

Setting parameter FFTonlyRealPart to true will produce a transform on
only the real part of I(Q,t). This is convenient if we know that I(Q,t)
should be real but a residual imaginary part was left in a Sassena
calculation due to finite orientational average in Q-space.

Below are plots after application of SassenaFFT to
:math:`I(Q,t) = e^{-t^2/(2\sigma^2)} + i\cdot t \cdot e^{-t^2/(2\sigma^2)}`
with :math:`\sigma=1ps`. Real an imaginary parts are shown in panels (a)
and (b). Note that :math:`I(Q,t)*=I(Q,-t)`. If only :math:`Re[I(Q,t)]`
is transformed, the result is another Gaussian:
:math:`\sqrt{2\pi}\cdot e^{-E^2/(2\sigma'^2)}` with
:math:`\sigma'=4136/(2\pi \sigma)` in units of :math:`\mu`\ eV (panel
(c)). If I(Q,t) is transformed, the result is a modulated Gaussian:
:math:`(1+\sigma' E)\sqrt{2\pi}\cdot e^{-E^2/(2\sigma'^2)}`\ (panel
(d)).

.. figure:: /images/SassenaFFTexample.jpg
   :alt: SassenaFFTexample.jpg

   SassenaFFTexample.jpg

Usage
-----

**Example - Load a Sassena file, Fourier transform it, and do a fit of S(Q,E):**

.. testcode:: Ex

    ws = LoadSassena("loadSassenaExample.h5", TimeUnit=1.0)
    SassenaFFT(ws, FFTonlyRealPart=1, Temp=1000, DetailedBalance=1)

    print 'workspaces instantiated: ', ', '.join(ws.getNames())

    sqt = ws[3] # S(Q,E)
    # I(Q,t) is a Gaussian, thus S(Q,E) is a Gaussian too (at high temperatures)
    # Let's fit it to a Gaussian. We start with an initial guess
    intensity = 100.0
    center = 0.0
    sigma = 0.01    #in meV
    startX = -0.1   #in meV
    endX = 0.1 
    myFunc = 'name=Gaussian,Height={0},PeakCentre={1},Sigma={2}'.format(intensity,center,sigma)

    # Call the Fit algorithm and perform the fit
    fitStatus, chiSq, covarianceTable, paramTable, fitWorkspace =\
    Fit(Function=myFunc, InputWorkspace=sqt, WorkspaceIndex=0, StartX = startX, EndX=endX, Output='fit')

    print "The fit was: " + fitStatus
    print("Fitted Height value is: %.1f" % paramTable.column(1)[0])
    print("Fitted centre value is: %.1f" % abs(paramTable.column(1)[1]))
    print("Fitted sigma value is: %.4f" % paramTable.column(1)[2])
    # fitWorkspace contains the data, the calculated and the difference patterns
    print "Number of spectra in fitWorkspace is: " +  str(fitWorkspace.getNumberHistograms())

Output:

.. testoutput:: Ex

    workspaces instantiated:  ws_qvectors, ws_fqt.Re, ws_fqt.Im, ws_sqw
    The fit was: success
    Fitted Height value is: 250.7
    Fitted centre value is: 0.0
    Fitted sigma value is: 0.0066
    Number of spectra in fitWorkspace is: 3

.. categories::
