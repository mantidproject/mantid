.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm is used to find single-crystal peaks in a
multi-dimensional workspace (`MDEventWorkspace <http://www.mantidproject.org/MDEventWorkspace>`_ or
:ref:`MDHistoWorkspace <MDHistoWorkspace>`). It looks for high signal
density areas, and is based on an algorithm designed by Dennis Mikkelson
for ISAW.

The algorithm proceeds in this way:

-  Sorts all the boxes in the workspace by decreasing order of signal
   density (total weighted event sum divided by box volume).

   -  It will skip any boxes with a density below a threshold. The
      threshold is
      :math:`TotalSignal / TotalVolume * DensityThresholdFactor`.

-  The centroid of the strongest box is considered a peak.
-  The centroid of the next strongest box is calculated.

   -  We look through all the peaks that have already been found. If the
      box is too close to an existing peak, it is rejected. This
      distance is PeakDistanceThreshold.

-  This is repeated until we find up to MaxPeaks peaks.

Each peak created is placed in the output
`PeaksWorkspace <http://www.mantidproject.org/PeaksWorkspace>`_, which can be a new workspace or
replace the old one.

This algorithm works on a :ref:`MDHistoWorkspace <MDHistoWorkspace>`
resulting from the :ref:`algm-BinMD` algorithm also. It works in the
same way, except that the center of each bin is used since the centroid
is not accessible. It may give better results on
:ref:`Workspace2D <Workspace2D>`'s that were converted to
:ref:`MDWorkspaces <MDWorkspace>`.

   
Usage
------

**Example - IntegratePeaks:**

The code iteslef works but disabled from doc tests as takes too long to complete. User should provide its own 
event nexus file instead of **TOPAZ_3132_event.nxs** used within this example. The original **TOPAZ_3132_event.nxs**
file is availible in `Mantid system tests repository <https://github.com/mantidproject/systemtests/tree/master/Data/TOPAZ_3132_event.nxs>`_.


.. code-block:: python
   :linenos:

   #.. testcode:: exFindPeaksMD


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
                      print "| {0:>8.2f} ".format(data2pr),
                 else:
                     print "| {0:>8} ".format(data2pr),   
           print "|\n",
       
    
   # load test workspace
   Load(Filename=r'TOPAZ_3132_event.nxs',OutputWorkspace='TOPAZ_3132_event',LoadMonitors='1')
   
   # build peak workspace necessary for IntegrateEllipsoids algorithm to work
   ConvertToMD(InputWorkspace='TOPAZ_3132_event',QDimensions='Q3D',dEAnalysisMode='Elastic',Q3DFrames='Q_sample',LorentzCorrection='1',OutputWorkspace='TOPAZ_3132_md',\
   MinValues='-25,-25,-25',MaxValues='25,25,25',SplitInto='2',SplitThreshold='50',MaxRecursionDepth='13',MinRecursionDepth='7')
   peaks=FindPeaksMD(InputWorkspace='TOPAZ_3132_md',PeakDistanceThreshold='0.37680',MaxPeaks='50',DensityThresholdFactor='100',OutputWorkspace='TOPAZ_3132_peaks')   

   # print 10 rows of table workspace
   print_tableWS(peaks,10)

**Output:**

.. code-block:: python
   :linenos:


   #.. testoutput:: exFindPeaksMD

   | RunNumbe  | DetID     | h         | k         | l         | Waveleng  | Energy    | TOF       | DSpacing  | Intens    | SigInt    | BinCount  | BankName  | Row       | Col       | QLab      | QSample   |
   |     3132  |  1124984  |     0.00  |     0.00  |     0.00  |     3.10  |     8.49  | 14482.29  |     2.02  |     0.00  |     0.00  |  1668.00  |   bank17  |   120.00  |    42.00  | [1.57771,1.21779,2.37854]  | [2.99396,0.815958,0.00317344]  |
   |     3132  |  1156753  |     0.00  |     0.00  |     0.00  |     2.08  |    18.82  |  9725.74  |     1.30  |     0.00  |     0.00  |  1060.00  |   bank17  |   145.00  |   166.00  | [2.48964,1.45725,3.88666]  | [4.52618,1.71025,0.129461]  |
   |     3132  |  1141777  |     0.00  |     0.00  |     0.00  |     1.71  |    28.09  |  7963.17  |     1.05  |     0.00  |     0.00  |    96.00  |   bank17  |    17.00  |   108.00  | [2.60836,2.31423,4.86391]  | [5.69122,1.79492,-0.452799]  |
   |     3132  |  1125241  |     0.00  |     0.00  |     0.00  |     1.55  |    33.86  |  7252.16  |     1.01  |     0.00  |     0.00  |    83.00  |   bank17  |   121.00  |    43.00  | [3.15504,2.42573,4.75121]  | [5.97829,1.63473,0.0118744]  |
   |     3132  |  1170598  |     0.00  |     0.00  |     0.00  |     1.55  |    34.12  |  7224.59  |     0.95  |     0.00  |     0.00  |    73.00  |   bank17  |   166.00  |   220.00  | [3.43363,1.70178,5.39301]  | [6.07726,2.59962,0.281759]  |
   |     3132  |  1214951  |     0.00  |     0.00  |     0.00  |     1.89  |    22.79  |  8839.55  |     1.68  |     0.00  |     0.00  |   719.00  |   bank18  |   231.00  |   137.00  | [2.73683,1.43808,2.11574]  | [3.5786,0.470838,1.00329]  |
   |     3132  |  1207827  |     0.00  |     0.00  |     0.00  |     1.71  |    27.89  |  7991.70  |     1.32  |     0.00  |     0.00  |   447.00  |   bank18  |    19.00  |   110.00  | [2.80324,2.29519,3.09134]  | [4.71517,0.554412,0.37714]  |
   |     3132  |  1232949  |     0.00  |     0.00  |     0.00  |     1.24  |    53.28  |  5782.14  |     0.93  |     0.00  |     0.00  |    45.00  |   bank18  |    53.00  |   208.00  | [4.29033,2.63319,4.46168]  | [6.52658,1.27985,1.00646]  |
   |     3132  |  1189484  |     0.00  |     0.00  |     0.00  |     1.14  |    63.42  |  5299.28  |     0.96  |     0.00  |     0.00  |    31.00  |   bank18  |   108.00  |    38.00  | [4.02414,3.39659,3.83664]  | [6.4679,0.298896,0.726133]  |
   |     3132  |  1218337  |     0.00  |     0.00  |     0.00  |     1.01  |    79.81  |  4724.05  |     0.77  |     0.00  |     0.00  |    15.00  |   bank18  |    33.00  |   151.00  | [4.96622,3.61607,5.32554]  | [7.99244,1.19363,0.892655]  |


.. categories::
