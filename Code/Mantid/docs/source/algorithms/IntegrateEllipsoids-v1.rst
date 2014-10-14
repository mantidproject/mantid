.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Overview and similar algorithms
###############################

This algorithm will integrate disjoint single crystal Bragg peaks by
summing the number of raw or weighted events in a 3D ellipsoidal peak region in
reciprocal space and subtracting an estimate of the background obtained
from an ellipsoidal shell. In some ways it is similar to the
:ref:`algm-IntegratePeaksMD` algorithm. In particular the size parameters to
this algorithm are also specified in inverse Angstroms and the
background subtraction is done in the same way for both the intensity
and the estimated standard deviations. However, this algorithm differs
from :ref:`algm-IntegratePeaksMD` in several critical ways.

-  This algorithm works directly with raw or weighted events 
   while :ref:`algm-IntegratePeaksMD` uses **MDEvents** from 
   `MDEventWorkspace <http://www.mantidproject.org/MDEventWorkspace>`_.
-  This algorithm uses 3D ellipsoidal regions with aspect ratios that
   are adapted to the set of events that are near the peak center, while
   :ref:`algm-IntegratePeaksMD` uses spherical regions.
-  This algorithm includes an option to automatically choose the size of
   the ellipsoidal regions based on the statistics of the set of events
   near the peak.
-  This algorithm only applies to peaks with integral HKL values and as
   currently implemented it cannot be used to integrate ellipsoidal
   regions at other locations in reciprocal space.

The algorithm calculates the three principal axes of the events near a
peak, and uses the standard deviations in the directions of the
principal axes to determine the aspect ratio of ellipsoids used for the
peak and background regions.

Explanation of Inputs
#####################

-  The event data to be integrated is obtained from an ordinary
   :ref:`EventWorkspace <EventWorkspace>` 
   with an X-axis in time-of-flight, as loaded from a
   NeXus event file. This algorithm maps the events to reciprocal space 
   using *PeaksWorkwpace* with indexed peaks to determine the parameters 
   of the transformation into the reciprocal space (UB matrix)

-  The peaks to be integrated are are also obtained from a *PeaksWorkspace*. The
   peaks must be indexed, and any peaks indexed as (0,0,0) will be
   ignored. The HKL values for valid peaks should all be integers, to
   make this check for unindexed peaks reliable.

-  Only events that are near a peak are considered when constructing the
   ellipsoids. The *RegionRadius* specifies the maximum distance from the
   peak center to an event in reciprocal space, for that event to used.
   See the figure below. Also, each event will be counted for at most
   one peak, the one with the nearest HKL value. The RegionRadius should
   be specified to be just slightly larger than the expected peak region
   to avoid overlap with other peaks, and to avoid including excessive
   background. As the size of the *RegionRadius* increases, the ellipsoids
   will become more spherical and less well adapted to the actual shape
   of the peak.

.. figure:: /images/IntegrateEllipsoids.png
   :alt: IntegrateEllipsoids.png
    
   Integrate ellipsoids algorithm regions map.

-  If the *SpecifySize* option is selected, then the user MUST specify the
   *PeakSize*, *BackgroundInnerSize* and *BackgroundOuterSize*. In this mode,
   the algorithm is similar to the :ref:`algm-IntegratePeaksMD` algorithm. As shown
   in the figure, these values determine the length of the major axis
   for the ellipsoidal peak region, and of the inner and outer
   ellipsoids bounding the background region. The same major axis
   lengths are used for all peaks, but the lengths of the other two axes
   of the ellipsoids are adjusted based on the standard deviations of
   the events in those directions. If *SpecifySize* is false, then the
   major axis length for each peak will be set to include a range of
   plus or minus three times the standard deviation of the events in
   that direction. That is, *PeakSize* is set to three times the standard
   deviation in the direction of the first principal axis. Also, in this
   case the *BackgroundInnerSize* is set to the *PeakSize* and the
   *BackgroundOuterSize* is set so that the background ellipsoidal shell
   has the same volume as the peak ellipsoidal region. If specified by
   the user, these parameters must be ordered correctly with: 
   :math:`0 < PeakSize \leq BackgroundInnerSize` and 
   :math:`BackgroundInnerSize < BackgroundOuterSize \leq RegionRadius`

-  The integrated intensities will be set in the specified
   *OutputWorkspace*. If this is different from the input *PeaksWorkspace*,
   the input peaks workspace will be copied to the *OutputWorkspace*
   before setting the integrated intensities.

Detailed Algorithm Description
##############################

This algorithm will integrate a list of indexed single-crystal
diffraction peaks from a *PeaksWorkspace*, using events from an
( :ref:`EventWorkspace <EventWorkspace>` ).
The indexed peaks are first used to determine a UB
matrix. The inverse of that UB matrix is then used to form lists of
events that are close to peaks in reciprocal space. An event will be
added to the list of events for a peak provided that the fractional
:math:`h,k,l` value of that event (obtained by applying UB-inverse to the
:math:`Q` -vector) is closer to the :math:`h,k,l` of that peak, 
than to the :math:`h,k,l` of any
other peak AND the :math:`Q` -vector for that event is within the specified
radius of the :math:`Q` -vector for that peak.

When the lists of events near the peaks have been built, the three
principal axes of the set of events near each peak are found, and the
standard deviations of the projections of the events on each of the
three principal axes are calculated. The principal axes and standard
deviations for the events around a peak in the directions of the
principal axes are used to determine an ellipsoidal region for the peak
and an ellipsoidal shell region for the background. The number of events
in the peak ellipsoid and background ellipsoidal shell are counted and
used to determine the net integrated intensity of the peak.

The ellipsoidal regions used for the peak and background can be obtained
in two ways. First, the user may specify the size of the peak ellipsoid
and the inner and outer size of the background ellipsoid. If these are
specified, the values will be used for half the length of the major axis
of an ellipsoid centered on the peak. The major axis is in the direction
of the principal axis for which the standard deviation in that direction
is largest. The other two axes for the ellipsoid are in the direction of
the other two principal axes and are scaled relative to the major axes
in proportion to their standard deviations. For example if the standard
deviations in the direction of the other two principal axes are .8 and .7
times the standard deviation in the direction of the major axis, then
the ellipse will extend only .8 and .7 times as far in the direction of
those axes, as in the direction of the major axis. Overall, the user
specified sizes for the *PeakSize*, *BackgroundInnerSize* and
*BackgroundOuterSize* are similar to the *PeakRadius*, *BackgroundInnerRadius*
and *BackgrounOuterRadius* for the :ref:`algm-IntegratePeaksMD` algorithm. The
difference is that the regions used in this algorithm are not spherical,
but are ellipsoidal with axis directions obtained from the principal
axes of the events near a peak and the ellipsoid shape (relative axis
lengths) is determined by the standard deviations in the directions of
the principal axes.

Second, if the user does not specify the size of the peak and
background ellipsoids, then the three axes of the peak ellipsoid are
again set to the principal axes of the set of nearby events but in this
case their axis lengths are set to cover a range of plus or minus three
standard deviations in the axis directions. In this case, the background
ellipsoidal shell is chosen to have the same volume as the peak
ellipsoid and it's inner surface is the outer surface of the peak
ellipsoid. The outer surface of the background ellipsoidal shell is an
ellipsoidal surface with the same relative axis lengths as the inner
surface.

Usage
------

**Example - IntegrateEllipsoids:**

The code iteslef works but disabled from doc tests as takes too long to complete. User should provide its own 
event nexus file instead of **TOPAZ_3132_event.nxs** used within this example. The original **TOPAZ_3132_event.nxs**
file is availible in `Mantid system tests repository <https://github.com/mantidproject/systemtests/tree/master/Data/TOPAZ_3132_event.nxs>`_.

.. code-block:: python
   :linenos:

   #.. testcode:: exIntegrateEllipsoids

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
   
      
   # load test workspace
   Load(Filename=r'TOPAZ_3132_event.nxs',OutputWorkspace='TOPAZ_3132_event',LoadMonitors='1')
      
   # build peak workspace necessary for IntegrateEllipsoids algorithm to work
   ConvertToMD(InputWorkspace='TOPAZ_3132_event',QDimensions='Q3D',dEAnalysisMode='Elastic',Q3DFrames='Q_sample',LorentzCorrection='1',OutputWorkspace='TOPAZ_3132_md',\
   MinValues='-25,-25,-25',MaxValues='25,25,25',SplitInto='2',SplitThreshold='50',MaxRecursionDepth='13',MinRecursionDepth='7')
   FindPeaksMD(InputWorkspace='TOPAZ_3132_md',PeakDistanceThreshold='0.3768',MaxPeaks='50',DensityThresholdFactor='100',OutputWorkspace='TOPAZ_3132_peaks')   
   FindUBUsingFFT(PeaksWorkspace='TOPAZ_3132_peaks',MinD='3',MaxD='15',Tolerance='0.12')
   IndexPeaks(PeaksWorkspace='TOPAZ_3132_peaks',Tolerance='0.12')
   
   # integrate Ellipsoids   
   result=IntegrateEllipsoids(InputWorkspace='TOPAZ_3132_event',PeaksWorkspace='TOPAZ_3132_peaks',\
          RegionRadius='0.25',PeakSize='0.2',BackgroundInnerSize='0.2',BackgroundOuterSize='0.25',OutputWorkspace='TOPAZ_3132_peaks')
   
   # print 10 rows of resulting table workspace
   print_tableWS(result,10)

**Output:**

.. code-block:: python
   :linenos:

   #.. testoutput:: exIntegrateEllipsoids

   | RunNumbe  | DetID     | h         | k         | l         | Waveleng  | Energy    | TOF       | DSpacing  | Intens    | SigInt    | BinCount  | BankName  | Row       | Col       | QLab      | QSample   |
   |     3132  |  1124984  |   -2.000  |   -1.000  |    2.000  |    3.104  |    8.491  | 14482.289  |    2.025  | 119890.000  |  372.000  | 1668.000  | bank17    |  120.000  |   42.000  | [1.57771,1.21779,2.37854]  | [2.99396,0.815958,0.00317344]  |
   |     3132  |  1156753  |   -3.000  |   -2.000  |    3.000  |    2.085  |   18.822  | 9725.739  |    1.298  | 148721.000  |  391.069  | 1060.000  | bank17    |  145.000  |  166.000  | [2.48964,1.45725,3.88666]  | [4.52618,1.71025,0.129461]  |
   |     3132  |  1141777  |   -4.000  |   -2.000  |    3.000  |    1.707  |   28.090  | 7963.171  |    1.050  | 8703.000  |  105.570  |   96.000  | bank17    |   17.000  |  108.000  | [2.60836,2.31423,4.86391]  | [5.69122,1.79492,-0.452799]  |
   |     3132  |  1125241  |   -4.000  |   -2.000  |    4.000  |    1.554  |   33.860  | 7252.155  |    1.014  | 19715.000  |  145.805  |   83.000  | bank17    |  121.000  |   43.000  | [3.15504,2.42573,4.75121]  | [5.97829,1.63473,0.0118744]  |
   |     3132  |  1170598  |   -4.000  |   -3.000  |    4.000  |    1.548  |   34.124  | 7224.587  |    0.950  | 15860.000  |  131.111  |   73.000  | bank17    |  166.000  |  220.000  | [3.43363,1.70178,5.39301]  | [6.07726,2.59962,0.281759]  |
   |     3132  |  1214951  |   -2.000  |   -1.000  |    4.000  |    1.894  |   22.795  | 8839.546  |    1.677  | 121613.000  |  352.155  |  719.000  | bank18    |  231.000  |  137.000  | [2.73683,1.43808,2.11574]  | [3.5786,0.470838,1.00329]  |
   |     3132  |  1207827  |   -3.000  |   -1.000  |    4.000  |    1.713  |   27.890  | 7991.697  |    1.319  | 64063.000  |  257.175  |  447.000  | bank18    |   19.000  |  110.000  | [2.80324,2.29519,3.09134]  | [4.71517,0.554412,0.37714]  |
   |     3132  |  1232949  |   -4.000  |   -2.000  |    6.000  |    1.239  |   53.277  | 5782.138  |    0.934  | 18185.000  |  139.072  |   45.000  | bank18    |   53.000  |  208.000  | [4.29033,2.63319,4.46168]  | [6.52658,1.27985,1.00646]  |
   |     3132  |  1189484  |   -4.000  |   -1.000  |    6.000  |    1.136  |   63.418  | 5299.275  |    0.964  | 13470.000  |  120.607  |   31.000  | bank18    |  108.000  |   38.000  | [4.02414,3.39659,3.83664]  | [6.4679,0.298896,0.726133]  |
   |     3132  |  1218337  |   -5.000  |   -2.000  |    7.000  |    1.012  |   79.807  | 4724.051  |    0.773  | 7405.000  |   88.210  |   15.000  | bank18    |   33.000  |  151.000  | [4.96622,3.61607,5.32554]  | [7.99244,1.19363,0.892655]  |
  

.. categories::
