.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Detector-space, single crystal peak finding. The algorithm can be used to
search through each spectra and find either all peaks or only the strongest
peak within a given x range. The x units of each spectrum can either be TOF or
d-spacing.


Strongest peak mode:

- The highest intensity bin is taken to be the peak, so the algorithm only 
  finds one peak per spectra. In this case the background is calculated as the
  average of the start and end intensity multiplied by the provided
  SignalBackground parameter. Peaks that are not above the background are
  culled.  
- This peak finding mode is mainly used with the ResolutionStrategy set to
  RelativeResolution.  In this case, the resolution parameter defines a
  tolerance which is compared to the absolute difference between the parameters
  :math:`\phi`, :math:`2\theta`, and :math:`t` of two found peaks.  If the
  absolute difference between any of the parameters for two peaks is greater
  than the product of the tolerance value and the parameter value then the two
  peaks are classed as not the same. i.e. if :math:`|\phi_1 - \phi_2| > tolerance * \phi_1` 
  then peaks 1 & 2 are not the same (as well as similar
  definitions for :math:`2\theta` and :math:`t`).


All peaks mode:

- All peaks in each spectra above a certain threshold are detected as peaks
  (see below for the threshold).  
- In this case the user provides an absolute, global background/threshold
  value which is set with the AbsoluteBackground parameter. Peaks that are not
  above the absolute background are culled. A good way of identifying the
  background is to inspect your data set in pick mode within the
  InstrumentView. Ensure that your absolute background has been set high
  enough, else the algorithm will pick up noise as peaks.  
- This peak finding mode provides the best results when used with the
  ResolutionStrategy set to AbsoluteResolution, which allows the user to set
  absolute resolution values for the XUnitResolution (either in units of TOF or
  d-spacing), PhiResolution and the TwoThetaResolution parameters. These
  resolution parameters define tolerances which are compared to the absolute
  difference between the parameters :math:`\phi`, :math:`2\theta`, and
  :math:`t` of two found peaks.  If the absolute difference between any of the
  parameters for two peaks is greater than the absolute tolerance then the two
  peaks are classed as not the same. i.e. if :math:`|\phi_1 - \phi_2| >  PhiTolerance` 
  then peaks 1 & 2 are not the same (as well as similar
  definitions for :math:`2\theta` and :math:`t`).


General points:

- Calculated Qlab follows the Busy, Levy 1967 convention.


Usage
-----

**Example**

.. testcode:: ExFindSXPeaksSimple

   # create histogram workspace
   ws=CreateSampleWorkspace()
   
   wsPeaks = FindSXPeaks(ws)

   print("Peaks found: {}".format(wsPeaks.getNumberPeaks()))

Output:

.. testoutput:: ExFindSXPeaksSimple

   Peaks found: 174

.. categories::

.. sourcelink::
