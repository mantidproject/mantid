.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The input to this algorithm is intended as part of the DEMAND data
reduction workflow, using :ref:`HB3AAdjustSampleNorm
<algm-HB3AAdjustSampleNorm>` with `OutputType=Detector` which can be
seen below in the example usage.

This will reduced the input workspace using the region of interest
provided to create a 1-dimension workspace with an axis that is the
detector scan, either omega or chi in degrees. This reduced workspace
is then fitted using :ref:`Fit <algm-Fit>` with a :ref:`flat
background <func-FlatBackground>` and a :ref:`Gaussian
<func-Gaussian>`, then the area of the Gaussian is used as the peak
intensity and added to the output workspace. An optionally fitting
range of the scan axis can be provided with `StartX` and `EndX` in
degrees.

The `OptimizeQVector` option will convert the input data into Q and
use :ref:`CentroidPeaksdMD <algm-CentroidPeaksMD>` to find the
correct Q-vector starting from the known HKL and UB matrix of the
peaks. This will not effect the integration of the peak but allows the
UB matrix to be refined afterwards.

Usage
-----

**Example - DEMAND single detector peak integration**

.. testcode::

   data = HB3AAdjustSampleNorm(Filename='HB3A_data.nxs', OutputType='Detector')
   peaks = HB3AIntegrateDetectorPeaks(data,
                                      ChiSqMax=100,
                                      OutputFitResults=True,
                                      LowerLeft=[200, 200],
                                      UpperRight=[312, 312])
   print('HKL={h:.0f}{k:.0f}{l:.0f} λ={Wavelength}Å Intensity={Intens:.3f}'.format(**peaks.row(0)))

.. testoutput::

   HKL=006 λ=1.008Å Intensity=211.753

To check the ROI and peak fitting you can plot the results

.. code-block:: python

   import matplotlib.pyplot as plt
   fig = plt.figure(figsize=(9.6, 4.8))
   ax1 = fig.add_subplot(121, projection='mantid')
   ax2 = fig.add_subplot(122, projection='mantid')
   ax1.pcolormesh(mtd['peaks_data_ROI'], transpose=True)
   ax1.set_title("ROI")
   ax2.plot(mtd['peaks_data_Workspace'], wkspIndex=0, label='data')
   ax2.plot(mtd['peaks_data_Workspace'], wkspIndex=1, label='calc')
   ax2.plot(mtd['peaks_data_Workspace'], wkspIndex=2, label='diff')
   ax2.legend()
   ax2.set_title("Fitted integrated peak")
   fig.tight_layout()
   fig.show()

.. figure:: /images/HB3AIntegrateDetectorPeaks.png

**Example - DEMAND multiple files, indexing with modulation vector**

.. code-block:: python

   IPTS = 24855
   exp = 755
   scans = range(28, 96)
   filename = '/HFIR/HB3A/IPTS-{}/shared/autoreduce/HB3A_exp{:04}_scan{:04}.nxs'

   data = HB3AAdjustSampleNorm(','.join(filename.format(IPTS, exp, scan) for scan in scans), OutputType="Detector")
   peaks = HB3AIntegrateDetectorPeaks(data)
   IndexPeaks(peaks, ModVector1='0,0,0.5', MaxOrder=1, SaveModulationInfo=True)
   SaveReflections(peaks, Filename='peaks.hkl')

.. categories::

.. sourcelink::
