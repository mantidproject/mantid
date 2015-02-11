.. algorithm::

.. summary::

.. alias::

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
e.g. For CORELLI, tubes 1 and 16, and pixels 0 and 255.
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

   
Usage
------

**Example - IntegratePeaks:**

The code iteslef works but disabled from doc tests as takes too long to complete. User should provide its own 
event nexus file instead of **TOPAZ_3132_event.nxs** used within this example. The original **TOPAZ_3132_event.nxs**
file is availible in `Mantid system tests repository <https://github.com/mantidproject/systemtests/tree/master/Data/TOPAZ_3132_event.nxs>`_.


.. code-block:: python
   :linenos:

   #.. testcode:: exIntegratePeaksMD


   def print_tableWS(pTWS,nRows):
       ''' Method to print part of the table workspace '''
       tab_names=pTWS.keys();
       
       for name in tab_names:
           if len(name)>8:
              name= name[0:8];
           print "| {0:8} ".format(name),
       print "|\n",
   
       for i in xrange(0,nRows):
           for name in tab_names:
                 col = pTWS.column(name);
                 data2pr=col[i]
                 if type(data2pr) is float:
                      print "| {0:8.3f} ".format(data2pr),
                 else:
                     print "| {0:8} ".format(data2pr),   
           print "|\n",


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

.. code-block:: python
   :linenos:

   #.. testoutput:: exIntegratePeaksMD

   | RunNumbe  | DetID     | h         | k         | l         | Waveleng  | Energy    | TOF       | DSpacing  | Intens    | SigInt    | BinCount  | BankName  | Row       | Col       | QLab      | QSample   |
   |     3132  |  1168976  |    0.000  |    0.000  |    0.000  |    1.106  |   66.853  | 5161.495  |    0.664  | 2161.555  |   32.493  | 1042.000  | bank17    |   80.000  |  214.000  | [4.42299,2.80447,7.87903]  | [8.7569,3.57474,-0.211883]  |
   |     3132  |  1156499  |    0.000  |    0.000  |    0.000  |    2.081  |   18.887  | 9708.954  |    1.297  | 5137.547  |   13.432  |  828.000  | bank17    |  147.000  |  165.000  | [2.49809,1.45732,3.88559]  | [4.53003,1.70942,0.137013]  |
   |     3132  |  1156756  |    0.000  |    0.000  |    0.000  |    1.040  |   75.677  | 4850.409  |    0.648  | 1597.017  |   30.643  |  577.000  | bank17    |  148.000  |  166.000  | [5.00569,2.90696,7.77943]  | [9.06543,3.43008,0.281929]  |
   |     3132  |  1141779  |    0.000  |    0.000  |    0.000  |    1.704  |   28.167  | 7952.321  |    1.049  |  648.434  |    7.481  |  379.000  | bank17    |   19.000  |  108.000  | [2.61862,2.31234,4.86545]  | [5.69642,1.79732,-0.443944]  |
   |     3132  |  1124982  |    0.000  |    0.000  |    0.000  |    1.555  |   33.819  | 7256.594  |    1.014  | 1990.427  |   14.457  |  330.000  | bank17    |  118.000  |   42.000  | [3.14235,2.43685,4.75299]  | [5.97935,1.62817,-0.00373607]  |
   |     3132  |  1170597  |    0.000  |    0.000  |    0.000  |    1.551  |   34.005  | 7237.138  |    0.951  | 1825.812  |   14.812  |  327.000  | bank17    |  165.000  |  220.000  | [3.42477,1.70221,5.38678]  | [6.06909,2.59493,0.276379]  |
   |     3132  |  1124982  |    0.000  |    0.000  |    0.000  |    3.111  |    8.454  | 14514.017  |    2.028  |  749.742  |    2.242  |  268.000  | bank17    |  118.000  |   42.000  | [1.57108,1.21836,2.37636]  | [2.9895,0.814038,-0.00186793]  |
   |     3132  |  1232181  |    0.000  |    0.000  |    0.000  |    1.238  |   53.388  | 5776.071  |    0.934  | 3460.775  |   25.974  | 1229.000  | bank18    |   53.000  |  205.000  | [4.28486,2.64933,4.45466]  | [6.52915,1.2635,0.998372]  |
   |     3132  |  1200023  |    0.000  |    0.000  |    0.000  |    1.433  |   39.816  | 6687.166  |    1.232  |  963.069  |    9.208  |  990.000  | bank18    |  151.000  |   79.000  | [3.37972,2.40572,2.9675]  | [5.01065,0.386939,0.871633]  |
   |     3132  |  1218594  |    0.000  |    0.000  |    0.000  |    1.016  |   79.240  | 4740.921  |    0.776  | 2999.159  |   35.467  |  901.000  | bank18    |   34.000  |  152.000  | [4.9551,3.59367,5.30453]  | [7.96049,1.19466,0.899379]  |

.. categories::
