.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The algorithm transforms a single spectrum workspace containing
spectral density :math:`S(Q)`, :math:`S(Q)-1`, or :math:`Q[S(Q)-1]`
(as a function of **MomentumTransfer** or **dSpacing** :ref:`units <Unit Factory>`) to a PDF
(pair distribution function) as described below and also the reverse. The available PDF types are the
reduced pair distribution function :math:`G(r)`, the pair distribution function :math:`g(r)`, the
radial distribution function :math:`RDF(r)`, and the total radial distribution function :math:`G_k(r)`.

The output from this algorithm will have an x-range between the minimum and maximum parameter of the output,
i.e. if converting from `g(r)` to `S(Q)` the output will be between `Qmin` and `Qmax`.

The spectrum density should be in the Q-space (\ **MomentumTransfer**\ ) :ref:`units <Unit Factory>` .
(d-spacing is not supported any more. Contact development team to fix that and enable **dSpacing** again)

References
----------

#. B. H. Toby and T. Egami, *Accuracy of Pair Distribution Functions Analysis Appliced to Crystalline and Non-Crystalline Materials*, Acta Cryst. (1992) A **48**, 336-346
   `doi: 10.1107/S0108767391011327 <http://dx.doi.org/10.1107/S0108767391011327>`_
#. B.H. Toby and S. Billinge, *Determination of Standard uncertainties in fits to pair distribution functions*  Acta Cryst. (2004) A **60**, 315-317]
   `doi: 10.1107/S0108767304011754 <http://dx.doi.org/10.1107/S0108767304011754>`_

.. The algorithm itself is able to identify the unit.  -- not any more. TODO:  should be investigated why this has been disabled


PDF Options
--------------

**g(r)**
########

.. raw:: html

   <center>

:math:`g(r) = \rho(r)/\rho_0 = 1+\frac{1}{2\pi^2\rho_0r} \int_{0}^{\infty} Q[S(Q)-1]\sin(Qr)dQ`

.. raw:: html

   </center>

and in this algorithm, it is implemented as

.. raw:: html

   <center>

:math:`g(r)-1 =  \frac{1}{2\pi \rho_0 r^3} \sum_{Q_{min}}^{Q_{max}} M(Q,Q_{max})(S(Q)-1)[\sin(Qr) - Qr\cos(Qr)]^{right bin}_{left bin}`

.. raw:: html

   </center>

where :math:`M(Q,Q_{max})` is an optional filter function. If Filter
property is set (true) then

.. raw:: html

   <center>

:math:`M(Q,Q_{max}) = \frac{\sin(\pi Q/Q_{max})}{\pi Q/Q_{max}}`

.. raw:: html

   </center>

otherwise

.. raw:: html

   <center>

:math:`M(Q,Q_{max}) = 1\,`

.. raw:: html

   </center>

**G(r)**
########

.. raw:: html

   <center>

:math:`G(r) = 4 \pi \rho_0 r [g(r)-1]`

.. raw:: html

   </center>

**RDF(r)**
##########

.. raw:: html

   <center>

:math:`RDF(r) = 4 \pi \rho_0 r^2 g(r)`

.. raw:: html

   </center>

**G_k(r)**
##########

.. raw:: html

   <center>

:math:`G_k(r) = \frac{0.01 * \langle b_{coh} \rangle ^2 G^{PDF}(r)}{(4 \pi)^2 \rho_0 r} = 0.01 * \langle b_{coh} \rangle ^2 [g(r)-1]`

.. raw:: html

   </center>

**Note:** All output forms are calculated by transforming :math:`g(r)-1`.

Usage
-----

**Example - PDF transformation examples:**

.. testcode:: ExPDFFourierTransform

    # Simulates Load of a workspace with all necessary parameters
    import numpy as np;
    xx = np.array(range(0,100))*0.1
    yy = np.exp(-(2.0 * xx)**2)
    yy = np.delete(yy,-1) # need one less Y value than X value for histogram data
    ws = CreateWorkspace(DataX=xx, DataY=yy, UnitX='MomentumTransfer')
    Rt = PDFFourierTransform(ws, SofQType='S(Q)-1', PDFType='g(r)')

    # Look at sample results:
    print('part of S(Q)-1 and its correlation function')
    for i in range(10):
       print('! {0:4.2f} ! {1:5f} ! {2:f} ! {3:5f} !'.format(xx[i], yy[i], Rt.readX(0)[i], Rt.readY(0)[i]))


.. testcleanup:: ExPDFFourierTransform

   DeleteWorkspace(ws)
   DeleteWorkspace(Rt)

**Output:**

.. testoutput:: ExPDFFourierTransform

   part of S(Q)-1 and its correlation function
   ! 0.00 ! 1.000000 ! 0.158666 ! 1.003494 !
   ! 0.10 ! 0.960789 ! 0.475999 ! 1.003423 !
   ! 0.20 ! 0.852144 ! 0.793331 ! 1.003308 !
   ! 0.30 ! 0.697676 ! 1.110664 ! 1.003154 !
   ! 0.40 ! 0.527292 ! 1.427997 ! 1.002965 !
   ! 0.50 ! 0.367879 ! 1.745329 ! 1.002750 !
   ! 0.60 ! 0.236928 ! 2.062662 ! 1.002515 !
   ! 0.70 ! 0.140858 ! 2.379994 ! 1.002269 !
   ! 0.80 ! 0.077305 ! 2.697327 ! 1.002018 !
   ! 0.90 ! 0.039164 ! 3.014660 ! 1.001770 !


.. categories::

.. sourcelink::
