.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm starts with a PeaksWorkspace containing the expected
positions of peaks in reciprocal space. It calculates the centroid of
the peak by calculating the average of the coordinates of all events
within a given radius of the peak, weighted by the weight (signal) of
the event.



Usage
------

**Example - CentroidPeaksMD:**

The code iteslef works but disabled from doc tests as takes too long to complete. User should provide its own 
event nexus file instead of **TOPAZ_3132_event.nxs** used within this example. The original **TOPAZ_3132_event.nxs**
file is availible in `Mantid system tests repository <https://github.com/mantidproject/systemtests/tree/master/Data/TOPAZ_3132_event.nxs>`_.

The example shows how applying centroid peaks changes the peak posisions previously calculated by 
FindPeaksMD algorithm.


.. code-block:: python
   :linenos:

   #.. testcode:: exCentroidPeaksMD

   def print_WSDifference(pTWS1,pTWS2,nRows):
          ''' Method to print difference between two table workspaces before and after applying CentroidPeaks '''
          # columns to compare
          tab_names=['RunNumber','DetID','Energy','DSpacing','QLab','QSample']      
          common = tab_names[0:2];
       long = tab_names[-2:];

   
          for name in tab_names:
              if name in common :
                   print("| {0:>10}  ".format(name))
              else:
                  if name in long:
                       print("|FindPeaksMD found (old):{0:>7} |IntegrEllipsoids (new): {0:>7}  ".format(name))
                  else:
                      ntp = name;
                      if len(ntp )>6:
                          ntp = ntp[0:6]
                      print("| old {0:>6} | new {0:>6}  ".format(ntp))
          print("|\n" )
      
          for i in xrange(0,nRows):
              for name in tab_names:
                    col1 = pTWS1.column(name);
                    data1_toPr=col1[i]
                    col2 = pTWS2.column(name);
                    data2_toPr=col2[i]
                    if name in common :
                         print("| {0:>10}  ".format(data1_toPr))
                    else:
                         if name in long:
                            print("| {0:>30} | {1:>30}  ".format(data1_toPr,data2_toPr))
                         else:
                            print("| {0:>10.2f} | {1:>10.2f}  ".format(data1_toPr,data2_toPr))
   
              print("|\n ")
    

   # load test workspace
   Load(Filename=r'TOPAZ_3132_event.nxs',OutputWorkspace='TOPAZ_3132_event',LoadMonitors='1')
      
   # build peak workspace necessary for IntegrateEllipsoids algorithm to work
   ConvertToMD(InputWorkspace='TOPAZ_3132_event',QDimensions='Q3D',dEAnalysisMode='Elastic',Q3DFrames='Q_sample',LorentzCorrection='1',OutputWorkspace='TOPAZ_3132_md',\
   MinValues='-25,-25,-25',MaxValues='25,25,25',SplitInto='2',SplitThreshold='50',MaxRecursionDepth='13',MinRecursionDepth='7')
   
   # get initial peak workspace
   peaks=FindPeaksMD(InputWorkspace='TOPAZ_3132_md',PeakDistanceThreshold='0.37680',MaxPeaks='50',DensityThresholdFactor='100',OutputWorkspace='TOPAZ_3132_peaks')      
   
   # refine peaks position with centroid peaks
   peaks2=CentroidPeaksMD(InputWorkspace='TOPAZ_3132_md', PeaksWorkspace='TOPAZ_3132_peaks', OutputWorkspace='TOPAZ_3132_peaks2')
   
   print_WSDifference(peaks,peaks2,10)
   
   
   **Output:**
   
.. code-block:: python
   :linenos:
   
   #.. testoutput:: exCentroidPeaksMD
     
      
   |  RunNumber  |      DetID  | old Energy | new Energy  | old DSpaci | new DSpaci  |FindPeaksMD found (old):   QLab |IntegrEllipsoids (new):    QLab  |FindPeaksMD found (old):QSample |IntegrEllipsoids (new): QSample  |
   |       3132  |    1124984  |       8.49 |      10.39  |       2.02 |       1.93  |      [1.57771,1.21779,2.37854] |       [1.9157,1.15022,2.37669]  |  [2.99396,0.815958,0.00317344] |    [3.13041,0.861402,0.316416]  |
   |       3132  |    1156753  |      18.82 |      18.87  |       1.30 |       1.29  |      [2.48964,1.45725,3.88666] |      [2.50792,1.41823,3.91448]  |     [4.52618,1.71025,0.129461] |     [4.52916,1.75746,0.149293]  |
   |       3132  |    1141777  |      28.09 |      29.63  |       1.05 |       1.04  |      [2.60836,2.31423,4.86391] |        [2.9387,2.15218,4.7974]  |    [5.69122,1.79492,-0.452799] |   [5.72802,1.86148,-0.0867018]  |
   |       3132  |    1125241  |      33.86 |      32.09  |       1.01 |       1.01  |      [3.15504,2.42573,4.75121] |      [3.12135,2.20547,4.87426]  |    [5.97829,1.63473,0.0118744] |     [5.9025,1.87759,0.0200907]  |
   |       3132  |    1170598  |      34.12 |      32.63  |       0.95 |       0.96  |      [3.43363,1.70178,5.39301] |       [3.2557,1.75038,5.41104]  |     [6.07726,2.59962,0.281759] |     [6.02352,2.57854,0.105647]  |
   |       3132  |    1214951  |      22.79 |      19.55  |       1.68 |       1.67  |      [2.73683,1.43808,2.11574] |      [2.60506,1.43592,2.30563]  |      [3.5786,0.470838,1.00329] |    [3.62222,0.607039,0.821705]  |
   |       3132  |    1207827  |      27.89 |      29.54  |       1.32 |       1.31  |      [2.80324,2.29519,3.09134] |      [2.99683,2.18047,3.05302]  |     [4.71517,0.554412,0.37714] |    [4.72528,0.607846,0.598834]  |
   |       3132  |    1232949  |      53.28 |      57.02  |       0.93 |       0.93  |      [4.29033,2.63319,4.46168] |      [4.40869,2.69431,4.34027]  |      [6.52658,1.27985,1.00646] |       [6.5525,1.15043,1.12919]  |
   |       3132  |    1189484  |      63.42 |      60.85  |       0.96 |       0.96  |      [4.02414,3.39659,3.83664] |      [4.15914,3.15181,3.95843]  |     [6.4679,0.298896,0.726133] |    [6.46553,0.557683,0.887368]  |
   |       3132  |    1218337  |      79.81 |      87.16  |       0.77 |       0.77  |      [4.96622,3.61607,5.32554] |      [5.17998,3.67105,5.16175]  |     [7.99244,1.19363,0.892655] |      [8.03942,1.03829,1.11448]  |
   
   
.. categories::

.. sourcelink::
