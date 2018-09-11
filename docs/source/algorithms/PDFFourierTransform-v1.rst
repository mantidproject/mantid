.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The algorithm transforms a single spectrum workspace containing 
spectral density :math:`S(Q)`, :math:`S(Q)-1`, or :math:`Q[S(Q)-1]` 
(as a fuction of **MomentumTransfer** or **dSpacing** `units <http://www.mantidproject.org/Units>`_ ) to a PDF 
(pair distribution function) as described below.

The input Workspace spectrum should be in the Q-space (\ **MomentumTransfer**\ ) `units <http://www.mantidproject.org/Units>`_ . 
(d-spacing is not supported any more. Contact development team to fix that and enable **dSpacing** again)

References
----------

#. B. H. Toby and T. Egami, *Accuracy of Pair Distribution Functions Analysis Appliced to Crystalline and Non-Crystalline Materials*, Acta Cryst. (1992) A **48**, 336-346
   `doi: 10.1107/S0108767391011327 <http://dx.doi.org/10.1107/S0108767391011327>`_
#. B.H. Toby and S. Billinge, *Determination of Standard uncertainities in fits to pair distribution functions*  Acta Cryst. (2004) A **60**, 315-317]
   `doi: 10.1107/S0108767304011754 <http://dx.doi.org/10.1107/S0108767304011754>`_

.. The algorithm itself is able to identify the unit.  -- not any more. TODO:  should be investigated why this has been disabled


Output Options
--------------

**G(r)**
########

.. raw:: html

   <center>

:math:`G(r) = 4\pi r[\rho(r)-\rho_0] = \frac{2}{\pi} \int_{0}^{\infty} Q[S(Q)-1]\sin(Qr)dQ`

.. raw:: html

   </center>

and in this algorithm, it is implemented as

.. raw:: html

   <center>

:math:`G(r) =  \frac{2}{\pi} \sum_{Q_{min}}^{Q_{max}} Q[S(Q)-1]\sin(Qr) M(Q,Q_{max}) \Delta Q`

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


**g(r)**
########

.. raw:: html

   <center>

:math:`G(r) = 4 \pi \rho_0 r [g(r)-1]`

.. raw:: html

   </center>


transforms to

.. raw:: html

   <center>

:math:`g(r) = \frac{G(r)}{4 \pi \rho_0 r} + 1`

.. raw:: html

   </center>


**RDF(r)**
##########

.. raw:: html

   <center>

:math:`RDF(r) = 4 \pi \rho_0 r^2 g(r)`

.. raw:: html

   </center>

transforms to

.. raw:: html

   <center>

:math:`RDF(r) = r G(r) + 4 \pi \rho_0 r^2`

.. raw:: html

   </center>
   
**Note:** All output forms except :math:`G(r)` are calculated by transforming :math:`G(r)`.   

Usage
-----

**Example - PDF transformation examples:**

.. testcode:: ExPDFFouurierTransform

    # Simulates Load of a workspace with all necessary parameters
    import numpy as np;
    xx = np.array(range(0,100))*0.1
    yy = np.exp(-(2.0 * xx)**2)
    ws = CreateWorkspace(DataX=xx, DataY=yy, UnitX='MomentumTransfer')
    Rt = PDFFourierTransform(ws, InputSofQType='S(Q)', PDFType='g(r)')   

    # Look at sample results:
    print('part of S(Q) and its correlation function')
    for i in range(10): 
       print('! {0:4.2f} ! {1:5f} ! {2:f} ! {3:5f} !'.format(xx[i], yy[i], Rt.readX(0)[i], Rt.readY(0)[i]))


.. testcleanup:: ExPDFFouurierTransform

   DeleteWorkspace(ws)
   DeleteWorkspace(Rt)   

**Output:**

.. testoutput:: ExPDFFouurierTransform

   part of S(Q) and its correlation function
   ! 0.00 ! 1.000000 ! 0.317333 ! -3.977330 !
   ! 0.10 ! 0.960789 ! 0.634665 ! 2.247452 !
   ! 0.20 ! 0.852144 ! 0.951998 ! 0.449677 !
   ! 0.30 ! 0.697676 ! 1.269330 ! 1.313403 !
   ! 0.40 ! 0.527292 ! 1.586663 ! 0.803594 !
   ! 0.50 ! 0.367879 ! 1.903996 ! 1.140167 !
   ! 0.60 ! 0.236928 ! 2.221328 ! 0.900836 !
   ! 0.70 ! 0.140858 ! 2.538661 ! 1.079278 !
   ! 0.80 ! 0.077305 ! 2.855993 ! 0.940616 !
   ! 0.90 ! 0.039164 ! 3.173326 ! 1.050882 !

   

.. categories::

.. sourcelink::
