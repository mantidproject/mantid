
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Calculates the 3D-ΔPDF [#f1]_ from a HKL MDHistoWorkspace. This
algorithm can remove the Bragg peaks by either the punch-and-fill
method [#f2]_ or the `KAREN`_ algorithm.

This algorithm is still in development and features may be added,
removed or changed. The scale of the resulting 3D-ΔPDF is, as yet, not
scaled correctly, so while the magnitude is not correct the sign
should be.

The input workspace must be a :ref:`MDHistoWorkspace
<MDHistoWorkspace>` with dimensions '[H,0,0]', '[0,K,0]' and
'[0,0,L]', The dimensions must be centered on zero.


Peak removal
------------

Two method are available to remove the Bragg peaks.

Punch-and-fill
==============

The punch-and-fill method is described in [#f2]_. Basically it will
punch out a volume of reciprocal space of shape defined by the
property `Shape` and `Size`. After punch-and-fill the removed Bragg
peaks are filled back in by applying a Gaussian convolution.

The convolution option requires `astropy
<http://docs.astropy.org/en/stable/index.html>`_ to be installed as it
uses `astropy.convolution
<http://docs.astropy.org/en/stable/convolution/>`_. The convolution
can be very slow for large workspaces, it will attempt to use
astropy.convolution.convolve_fft (which is fast but only works for
small workspace) but will use astropy.convolution.convolve (which is
slow) if the workspace is too large.

KAREN
=====

The KAREN (K-space Algorithmic REconstructioN) method [#f3]_, is a
more advanced approach that applies a filter over the data removing
any points that are outliers in a moving window, with width set by the
property `KARENWidth`. Outliers are defined as values more than 3
sigma away from the median. Sigma is estimated using 1.4826*MAD
(median absolute deviation). Outliers are replaced with a value of
median+2.2*MAD of window.

Window function
---------------

A window function can be applied to the volume that will produce a
smooth transition to zero and you approach the edge of the data.

Currently implemented functions are Blackman (:func:`numpy.blackman`,
default), Gaussian (:func:`scipy.signal.windows.gaussian`), Tukey
(:func:`scipy.signal.windows.tukey`) and Kaiser (:func:`numpy.kaiser`)

The `WindowParameter` allows you to define the Gaussian window sigma,
the Tukey window alpha and the Kaiser window beta.

References
----------

.. [#f1] Weber, T and Simonov, A, *The three-dimensional pair distribution function analysis of disordered single crystals: basic concepts.* Zeitschrift für Kristallographie (2012), **227**, 5, 238-247
   `doi: 10.1524/zkri.2012.1504 <https://doi.org/10.1524/zkri.2012.1504>`_

.. [#f2] Kobas, M and Weber, T and Steurer, W, *Structural disorder in the decagonal Al-Co-Ni. I. Patterson analysis of diffuse x-ray scattering data.* Phys. Rev. B (2005), **71**, 22, 224205
   `doi: 10.1103/PhysRevB.71.224205 <https://doi.org/10.1103/PhysRevB.71.224205>`_

.. [#f3] Weng, J *et al.* *K-space Algorithmic REconstructioN (KAREN): A robust statistical methodology to separate Bragg and diffuse scattering.* J. Appl. Crystallogr. In-preparation.


Usage -  Punch-and-Fill
-----------------------

.. testsetup:: *

   # Create a fake workspace to test
   DeltaPDF3D_MDE = CreateMDWorkspace(Dimensions='3', Extents='-3.1,3.1,-3.1,3.1,-0.1,0.1', Names='[H,0,0],[0,K,0],[0,0,L]',
                                      Units='rlu,rlu,rlu', SplitInto='4',Frames='HKL,HKL,HKL')
   # Add some Bragg peaks
   for h in range(-3,4):
      for k in range(-3,4):
         FakeMDEventData(DeltaPDF3D_MDE, PeakParams='100,'+str(h)+','+str(k)+',0,0.01', RandomSeed='1337')
   # Add addiontal peaks on [0.5,0.5,0.5] type positions
   # This would correspond to negative substitutional correlations
   for h in [-2.5,-1.5,-0.5,0.5,1.5,2.5]:
      for k in range(-3,4):
         FakeMDEventData(DeltaPDF3D_MDE, PeakParams='20,'+str(h)+','+str(k)+',0,0.1', RandomSeed='13337')
   # Create MHHistoWorkspace
   BinMD(InputWorkspace='DeltaPDF3D_MDE', AlignedDim0='[H,0,0],-3.05,3.05,61', AlignedDim1='[0,K,0],-3.05,3.05,61',
         AlignedDim2='[0,0,L],-0.1,0.1,1', OutputWorkspace='DeltaPDF3D_MDH')


The example here is MDHistoWorkspace that corresponds to negative
substitutional correlation in the [100] direction. If you just run it
without any alterations to the workspace the 3D-ΔPDF will be
dominated by the Bragg peaks and will just be a 3D-PDF instead.

.. testcode:: fft1

   DeltaPDF3D(InputWorkspace='DeltaPDF3D_MDH',OutputWorkspace='fft',
              Method='None', WindowFunction='None')
   print("The value at [1,0,0] is {:.4f}".format(mtd['fft'].signalAt(1866)))
   print("The value at [0,1,0] is {:.4f}".format(mtd['fft'].signalAt(2226)))

.. testoutput:: fft1

   The value at [1,0,0] is 4057.7079
   The value at [0,1,0] is 5565.6700

The results 3D-ΔPDF workspace looks like

+--------------------+--------------------+
| Starting workspace | Resulting 3D-PDF   |
+--------------------+--------------------+
| |int1|             | |fft1|             |
+--------------------+--------------------+

.. |fft1| image:: /images/DeltaPDF3D_fft1.png
   :width: 100%
.. |int1| image:: /images/DeltaPDF3D_testWS.png
   :width: 100%

**Removing Reflections**

To get a Δ-PDF you need to remove the Bragg peaks. If we now
remove the reflections you will see that negative value at [±1,0,0].

The IntermediateWorkspace shows the changes to the input workspace.

.. testcode:: fft2

   DeltaPDF3D(InputWorkspace='DeltaPDF3D_MDH',OutputWorkspace='fft2',IntermediateWorkspace='int2',
              Method='Punch and fill',Size=0.3,Convolution=False, WindowFunction='None')
   print("The value at [1,0,0] is {:.4f}".format(mtd['fft2'].signalAt(1866)))
   print("The value at [0,1,0] is {:.4f}".format(mtd['fft2'].signalAt(2226)))

.. testoutput:: fft2

   The value at [1,0,0] is -738.9594
   The value at [0,1,0] is 769.0027

+--------------------------------------------------+--------------------------------------------------+
| Intermediate workspace after reflections removed | Resulting 3D-ΔPDF                                |
+--------------------------------------------------+--------------------------------------------------+
| |int2|                                           | |fft2|                                           |
+--------------------------------------------------+--------------------------------------------------+

.. |fft2| image:: /images/DeltaPDF3D_fft2.png
   :width: 100%
.. |int2| image:: /images/DeltaPDF3D_int2.png
   :width: 100%

**Removing Reflections and crop to sphere**

.. testcode:: fft3

   DeltaPDF3D(InputWorkspace='DeltaPDF3D_MDH',OutputWorkspace='fft3',IntermediateWorkspace='int3',
              Method='Punch and fill',Size=0.3,CropSphere=True,SphereMax=3,Convolution=False, WindowFunction='None')
   print("The value at [1,0,0] is {:.4f}".format(mtd['fft3'].signalAt(1866)))
   print("The value at [0,1,0] is {:.4f}".format(mtd['fft3'].signalAt(2226)))

.. testoutput:: fft3

   The value at [1,0,0] is -477.1737
   The value at [0,1,0] is 501.0818

+---------------------------------------------------------------------+---------------------------------------------------------------------+
| Intermediate workspace after reflections removed and crop to sphere | Resulting 3D-ΔPDF                                                   |
+---------------------------------------------------------------------+---------------------------------------------------------------------+
| |int3|                                                              | |fft3|                                                              |
+---------------------------------------------------------------------+---------------------------------------------------------------------+

.. |fft3| image:: /images/DeltaPDF3D_fft3.png
   :width: 100%
.. |int3| image:: /images/DeltaPDF3D_int3.png
   :width: 100%

**Removing Reflections and crop to sphere with fill value**
The fill value should be about the background level

.. testcode:: fft3_2

   DeltaPDF3D(InputWorkspace='DeltaPDF3D_MDH',OutputWorkspace='fft3',IntermediateWorkspace='int3',
              Method='Punch and fill',Size=0.3,CropSphere=True,SphereMax=3,Convolution=False, WindowFunction='None')
   print("The value at [1,0,0] is {:.4f}".format(mtd['fft3'].signalAt(1866)))
   print("The value at [0,1,0] is {:.4f}".format(mtd['fft3'].signalAt(2226)))

.. testoutput:: fft3_2

   The value at [1,0,0] is -477.1737
   The value at [0,1,0] is 501.0818

+---------------------------------------------------------------------+---------------------------------------------------------------------+
| Intermediate workspace after reflections removed and crop to sphere | Resulting 3D-ΔPDF                                                   |
+---------------------------------------------------------------------+---------------------------------------------------------------------+
| |int3_2|                                                            | |fft3_2|                                                            |
+---------------------------------------------------------------------+---------------------------------------------------------------------+

.. |fft3_2| image:: /images/DeltaPDF3D_fft3_2.png
   :width: 100%
.. |int3_2| image:: /images/DeltaPDF3D_int3_2.png
   :width: 100%

**Applying convolution**

.. code-block:: python

   DeltaPDF3D(InputWorkspace='DeltaPDF3D_MDH',OutputWorkspace='fft4',IntermediateWorkspace='int4'
              Method='Punch and fill',Size=0.3,CropSphere=True,SphereMax=3,Convolution=True, WindowFunction='None')
   print("The value at [1,0,0] is {:.4f}".format(mtd['fft4'].signalAt(1866)))
   print("The value at [0,1,0] is {:.4f}".format(mtd['fft4'].signalAt(2226)))

.. code-block:: none

   The value at [1,0,0] is -47.1984
   The value at [0,1,0] is 44.3406

+-----------------------------------------------------+-----------------------------------------------------+
| Intermediate workspace after convolution is applied | Resulting 3D-ΔPDF                                   |
+-----------------------------------------------------+-----------------------------------------------------+
| |int4|                                              | |fft4|                                              |
+-----------------------------------------------------+-----------------------------------------------------+

.. |fft4| image:: /images/DeltaPDF3D_fft4.png
   :width: 100%
.. |int4| image:: /images/DeltaPDF3D_int4.png
   :width: 100%

Usage - KAREN
-------------

.. testcode:: karen

   DeltaPDF3D(InputWorkspace='DeltaPDF3D_MDH',OutputWorkspace='fft',IntermediateWorkspace='int',KARENWidth=3)
   print("The value at [1,0,0] is {:.4f}".format(mtd['fft'].signalAt(1866)))
   print("The value at [0,1,0] is {:.4f}".format(mtd['fft'].signalAt(2226)))

.. testoutput:: karen

   The value at [1,0,0] is -18.4259
   The value at [0,1,0] is 18.4204

+----------------------------------------------------------------+----------------------------------------------------------------+
| Intermediate workspace after KAREN and window function applied | Resulting 3D-ΔPDF                                              |
+----------------------------------------------------------------+----------------------------------------------------------------+
| |int5|                                                         | |fft5|                                                         |
+----------------------------------------------------------------+----------------------------------------------------------------+

.. |fft5| image:: /images/DeltaPDF3D_karen_fft.png
   :width: 100%
.. |int5| image:: /images/DeltaPDF3D_karen_int.png
   :width: 100%

.. categories::

.. sourcelink::
