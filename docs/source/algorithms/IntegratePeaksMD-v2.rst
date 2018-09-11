.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm performs integration of single-crystal peaks within a
radius (with optional background subtraction) in reciprocal space.

Similar algorithms
##################

See :ref:`algm-IntegrateEllipsoids` for a ways of integrating peaks from data collected in
`EventWorkspace <http://www.mantidproject.org/EventWorkspace>`_. :ref:`algm-PeakIntensityVsRadius`
is meant to help determine an appropriate value for `PeakRadius`.

Inputs
######

The algorithms takes two input workspaces:

-  A MDEventWorkspace containing the events in multi-dimensional space.
   This would be the output of
   :ref:`algm-ConvertToDiffractionMDWorkspace`.
-  As well as a PeaksWorkspace containing single-crystal peak locations.
   This could be the output of :ref:`algm-FindPeaksMD`
-  The OutputWorkspace will contain a copy of the input PeaksWorkspace,
   with the integrated intensity and error found being filled in.

Calculations
############

Integration is performed by summing the weights of each MDEvent within
the provided radii. Errors are also summed in quadrature.

.. figure:: /images/IntegratePeaksMD_graph1.png
   :alt: IntegratePeaksMD_graph1.png

   IntegratePeaksMD\_graph1.png

-  All the Radii are specified in :math:`\AA^{-1}`
-  A sphere of radius **PeakRadius** is integrated around the center of
   each peak.

   -  This gives the summed intensity :math:`I_{peak}` and the summed
      squared error :math:`\sigma I_{peak}^2`.
   -  The volume of integration is :math:`V_{peak}`.

-  If **BackgroundOuterRadius** is specified, then a shell, with radius
   r where **BackgroundInnerRadius** < r < **BackgroundOuterRadius**, is
   integrated.

   -  This gives the summed intensity :math:`I_{shell}` and the summed
      squared error :math:`\sigma I_{shell}^2`.
   -  The volume of integration is :math:`V_{shell}`.
   -  **BackgroundInnerRadius** allows you to give some space between
      the peak and the background area.
   -  If the option **UseOnePercentBackgroundCorrection** is enabled, which it is by default, then the top one percent of the background events are removed so that there are no intensity spikes near the edges.

-  **AdaptiveQMultiplier** can be used for the radius to vary as a function of the modulus of Q. If the AdaptiveQBackground option is set to True, the background radius also changes so each peak has a different integration radius.  Q includes the 2*pi factor.

   -  PeakRadius + AdaptiveQMultiplier * **|Q|** 
   -  BackgroundOuterRadius + AdaptiveQMultiplier * **|Q|** 
   -  BackgroundInnerRadius + AdaptiveQMultiplier * **|Q|**

Background Subtraction
######################

The background signal within PeakRadius is calculated by scaling the
background signal density in the shell to the volume of the peak:

:math:`I_{bg} = I_{shell} \frac{V_{peak}}{V_{shell}}`

with the error squared on that value:

:math:`\sigma I_{bg}^2 = (\frac{V_{peak}}{V_{shell}})^2 \sigma I_{shell}^2`

This is applied to the integrated peak intensity :math:`I_{peak}` to
give the corrected intensity :math:`I_{corr}`:

:math:`I_{corr} = I_{peak} - I_{bg}`

with the errors summed in quadrature:

:math:`\sigma I_{corr}^2 = \sigma I_{peak}^2 + \sigma I_{bg}^2`

If BackgroundInnerRadius is Omitted
###################################

If BackgroundInnerRadius is left blank, then **BackgroundInnerRadius** =
**PeakRadius**, and the integration is as follows:

.. figure:: /images/IntegratePeaksMD_graph2.png
   :alt: IntegratePeaksMD_graph2.png

   IntegratePeaksMD\_graph2.png

IntegrateIfOnEdge option
###################################

Edges for each bank or pack of tubes of the instrument are defined by masking the edges in the PeaksWorkspace instrument. 
e.g. For TOPAZ pixels 0 and 255 in both directions for the Rectangular Detector.
Q in the lab frame for every peak is calculated, call it C
For every point on the edge, the trajectory in reciprocal space is a straight line, going through:

:math:`\vec{O}=(0,0,0)`

Calculate a point at a fixed momentum, say k=1. 
Q in the lab frame:

:math:`\vec{E}=(-k*sin(\theta)*cos(\phi),-k*sin(\theta)*sin(\phi),k-k*cos(\phi))`

Normalize E to 1: 

:math:`\vec{E}=\vec{E}*(1./\left|\vec{E}\right|)`

The distance from C to OE is given by:

:math:`dv=\vec{C}-\vec{E}*(\vec{C} \cdot \vec{E})`

If:

:math:`\left|dv\right|<PeakRadius`

for the integration, one of the detector trajectories on the edge is too close to the peak 
This method is also applied to all masked pixels.  If there are masked pixels trajectories inside an integration volume, the peak must be rejected.

   
CorrectIfOnEdge option
###################################

This is an extension of what was calculated for the IntegrateIfOnEdge option.  It will only be calculated if this option  
is true and the minimum dv is less than PeakRadius or BackgroundOuterRadius.  

For the background if

:math:`\left|dv\right|_{min}<BackgroundOuterRadius` 

:math:`h = BackgroundOuterRadius - \left|dv\right|_{min}`

From the minimum of dv the volume of the cap of the sphere is found:

:math:`V_{cap} = \pi h^2 / 3 (3 * BackgroundOuterRadius - h)`

The volume of the total sphere is calculated and for the background the volume of the inner radius must be subtracted:

:math:`V_{shell} = 4/3 \pi (BackgroundOuterRadius^3 - BackgroundInnerRadius^3)`

The integrated intensity is multiplied by the ratio of the volume of the sphere divided by the volume where data was collected

:math:`I_{bkgMultiplier} = V_{shell} / (V_{shell} - V_{cap})`


For the peak assume that the shape is Gaussian.  If

:math:`\left|dv\right|_{min}<PeakRadius`

:math:`\sigma = PeakRadius / 3`

:math:`h = PeakRadius * exp(-\left|dv\right|_{min}^2 / (2 \sigma^2)`

From the minimum of dv the volume of the cap of the sphere is found:

:math:`V_{cap} = \pi h^2 / 3 (3 * PeakRadius - h)`

and the volume of the sphere is calculated

:math:`V_{sphere} = 4/3 \pi PeakRadius^3`

The integrated intensity is multiplied by the ratio of the volume of the sphere divided by the volume where data was collected

:math:`I_{peakMultiplier} = V_{sphere} / (V_{sphere} - V_{cap})`


   
Usage
------

**Example - IntegratePeaks:**

User should provide its own 
event nexus file instead of **TOPAZ_3132_event.nxs** used within this example. The original **TOPAZ_3132_event.nxs**
file is availible in `Mantid system tests repository <https://github.com/mantidproject/systemtests/tree/master/Data/TOPAZ_3132_event.nxs>`_.

.. The code itself works but disabled from doc tests as takes too long to complete. 
.. .. testcode:: exIntegratePeaksMD

.. code-block:: python
   :linenos:

   def print_tableWS(pTWS,nRows):
       ''' Method to print part of the table workspace '''
       tab_names=pTWS.keys()
       row = ""
       for name in tab_names:
           if len(name)>8:
              name= name[:8]
           row += "| {:8} ".format(name)
       print(row + "|")
   
       for i in range(nRows):
           row = ""
           for name in tab_names:
                 col = pTWS.column(name);
                 data2pr=col[i]
                 if type(data2pr) is float:
                     row += "| {:8.1f} ".format(data2pr)
                 else:
                     row += "| {:8} ".format(str(data2pr))
           print(row + "|")

    # Load a SCD data set and find the peaks
   LoadEventNexus(Filename=r'TOPAZ_3132_event.nxs',OutputWorkspace='TOPAZ_3132_nxs')
   ConvertToDiffractionMDWorkspace(InputWorkspace='TOPAZ_3132_nxs',OutputWorkspace='TOPAZ_3132_md',LorentzCorrection='1')
   FindPeaksMD(InputWorkspace='TOPAZ_3132_md',PeakDistanceThreshold='0.15',MaxPeaks='100',OutputWorkspace='peaks')
   FindUBUsingFFT(PeaksWorkspace='peaks',MinD='2',MaxD='16')

    # Perform the peak integration, in-place in the 'peaks' workspace.
   peaks= IntegratePeaksMD(InputWorkspace='TOPAZ_3132_md', PeaksWorkspace='peaks',\
        PeakRadius=0.12, BackgroundOuterRadius=0.2, BackgroundInnerRadius=0.16,\
        OutputWorkspace='peaks')
        
   # print the integration results
   print_tableWS(peaks,10)   

**Output:**

.. .. testoutput:: exIntegratePeaksMD

.. code-block:: python
   :linenos:


   | RunNumbe | DetID    | h        | k        | l        | Waveleng | Energy   | TOF      | DSpacing | Intens   | SigInt   | BinCount | BankName | Row      | Col      | QLab     | QSample  | PeakNumb |
   | 3132     | 1168209  |      0.0 |      0.0 |      0.0 |      1.1 |     66.9 |   5158.0 |      0.7 |   2160.9 |     32.3 |   1326.0 | bank17   |     81.0 |    211.0 | [4.42961,2.81707,7.86314] | [8.75838,3.55459,-0.205083] | 1        |
   | 3132     | 1124983  |      0.0 |      0.0 |      0.0 |      1.6 |     33.9 |   7250.6 |      1.0 |   1990.0 |     14.4 |   1060.0 | bank17   |    119.0 |     42.0 | [3.14813,2.43563,4.75389] | [5.9822,1.62965,0.00130101] | 2        |
   | 3132     | 1141521  |      0.0 |      0.0 |      0.0 |      1.7 |     28.1 |   7959.1 |      1.0 |    644.6 |      7.3 |   1034.0 | bank17   |     17.0 |    107.0 | [2.60893,2.31831,4.86248] | [5.69311,1.79103,-0.453311] | 3        |
   | 3132     | 1125238  |      0.0 |      0.0 |      0.0 |      3.1 |      8.4 |  14518.9 |      2.0 |    750.5 |      2.2 |    880.0 | bank17   |    118.0 |     43.0 | [1.57116,1.21649,2.37775] | [2.98926,0.816337,-0.00161709] | 4        |
   | 3132     | 1170852  |      0.0 |      0.0 |      0.0 |      1.6 |     34.0 |   7235.3 |      1.0 |   1826.4 |     14.7 |    762.0 | bank17   |    164.0 |    221.0 | [3.4229,1.70246,5.39532] | [6.0734,2.6008,0.271523] | 5        |
   | 3132     | 1156497  |      0.0 |      0.0 |      0.0 |      2.1 |     18.9 |   9718.2 |      1.3 |   5137.6 |     13.4 |    518.0 | bank17   |    145.0 |    165.0 | [2.49117,1.46093,3.88649] | [4.5291,1.70753,0.129446] | 6        |
   | 3132     | 1207828  |      0.0 |      0.0 |      0.0 |      1.7 |     27.9 |   7989.1 |      1.3 |   3233.6 |     12.7 |   1024.0 | bank18   |     20.0 |    110.0 | [2.80538,2.29342,3.08833] | [4.71342,0.553533,0.380727] | 7        |
   | 3132     | 1218593  |      0.0 |      0.0 |      0.0 |      1.0 |     79.6 |   4729.3 |      0.8 |   3018.1 |     35.4 |    756.0 | bank18   |     33.0 |    152.0 | [4.96533,3.60693,5.32436] | [7.98578,1.19927,0.895763] | 8        |
   | 3132     | 1232694  |      0.0 |      0.0 |      0.0 |      1.2 |     53.4 |   5772.9 |      0.9 |   3464.5 |     25.9 |    631.0 | bank18   |     54.0 |    207.0 | [4.29539,2.63813,4.45945] | [6.53086,1.27477,1.00974] | 9        |
   | 3132     | 1200023  |      0.0 |      0.0 |      0.0 |      0.7 |    159.1 |   3345.1 |      0.6 |   3796.1 |     71.1 |    509.0 | bank18   |    151.0 |     79.0 | [6.75629,4.8092,5.93224] | [10.0166,0.773518,1.74245] | 10       |

.. categories::

.. sourcelink::
