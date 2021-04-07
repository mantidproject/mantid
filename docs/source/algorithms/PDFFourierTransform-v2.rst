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
reduced pair distribution function :math:`G(r)`, the pair distribution function :math:`g(r)`, and the
radial distribution function :math:`RDF(r)`.

The output from this algorithm will have an x-range between 0.0 and the maximum parameter of the output,
i.e. if converting from `g(r)` to `S(Q)` the output will be between 0.0 and `Qmax`.

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

**Note:** All output forms are calculated by transforming :math:`g(r)-1`.

Usage
-----

**Example - PDF transformation examples:**

.. testcode:: ExPDFFourierTransform

    # Simulates Load of a workspace with all necessary parameters
    import numpy as np;
    xx = np.array(range(0,100))*0.1
    yy = np.exp(-(2.0 * xx)**2)
    ws = CreateWorkspace(DataX=xx, DataY=yy, UnitX='MomentumTransfer')
    Rt = PDFFourierTransform(ws, SofQType='S(Q)', PDFType='g(r)')

    # Look at sample results:
    print('part of S(Q) and its correlation function')
    for i in range(10):
       print('! {0:4.2f} ! {1:5f} ! {2:f} ! {3:5f} !'.format(xx[i], yy[i], Rt.readX(0)[i], Rt.readY(0)[i]))


.. testcleanup:: ExPDFFourierTransform

   DeleteWorkspace(ws)
   DeleteWorkspace(Rt)

**Output:**

.. testoutput:: ExPDFFourierTransform

   part of S(Q) and its correlation function
   ! 0.00 ! 1.000000 ! 0.317333 ! -3.977042 !
   ! 0.10 ! 0.960789 ! 0.634665 ! 2.248558 !
   ! 0.20 ! 0.852144 ! 0.951998 ! 0.449916 !
   ! 0.30 ! 0.697676 ! 1.269330 ! 1.314437 !
   ! 0.40 ! 0.527292 ! 1.586663 ! 0.803744 !
   ! 0.50 ! 0.367879 ! 1.903996 ! 1.141098 !
   ! 0.60 ! 0.236928 ! 2.221328 ! 0.900872 !
   ! 0.70 ! 0.140858 ! 2.538661 ! 1.080090 !
   ! 0.80 ! 0.077305 ! 2.855993 ! 0.940530 !
   ! 0.90 ! 0.039164 ! 3.173326 ! 1.051576 !


.. categories::

.. sourcelink::
