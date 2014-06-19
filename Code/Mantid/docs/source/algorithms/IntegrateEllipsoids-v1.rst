.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Overview
########

This algorithm will integrate disjoint single crystal Bragg peaks by
summing the number of raw events in a 3D ellipsoidal peak region in
reciprocal space and subtracting an estimate of the background obtained
from an ellipsoidal shell. In some ways it is similar to the
IntegratePeaksMD algorithm, and it would be useful to also see the
documentation for that algorithm. In particular the size parameters to
this algorithm are also specified in inverse Angstroms and the
background subtraction is done in the same way for both the intensity
and the estimated standard deviations. However, this algorithm differs
from IntegratePeaksMD in several critical ways.

-  This algorithm counts raw, un-weighted events while IntegratePeaksMD
   uses weighted events.
-  This algorithm uses 3D ellipsoidal regions with aspect ratios that
   are adapted to the set of events that are near the peak center, while
   IntegratePeaksMD uses spherical regions.
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
   EventWorkspace with an X-axis in time-of-flight, as loaded from a
   NeXus event file. This algorithm maps the events to reciprocal space.

-  The peaks to be integrated are obtained from a PeaksWorkspace. The
   peaks must be indexed, and any peaks indexed as (0,0,0) will be
   ignored. The HKL values for valid peaks should all be integers, to
   make this check for unindexed peaks reliable.

-  Only events that are near a peak are considered when constructing the
   ellipsoids. The RegionRadius specifies the maximum distance from the
   peak center to an event in reciprocal space, for that event to used.
   See the figure below. Also, each event will be counted for at most
   one peak, the one with the nearest HKL value. The RegionRadius should
   be specified to be just slightly larger than the expected peak region
   to avoid overlap with other peaks, and to avoid including excessive
   background. As the size of the RegionRadius increases, the ellipsoids
   will become more spherical and less well adapted to the actual shape
   of the peak.

.. figure:: /images/IntegrateEllipsoids.png
   :alt: IntegrateEllipsoids.png

   IntegrateEllipsoids.png

-  If the SpecifySize option is selected, then the user MUST specify the
   PeakSize, BackgroundInnerSize and BackgroundOuterSize. In this mode,
   the algorithm is similar to the IntegratePeaksMD algorithm. As shown
   in the figure, these values determine the length of the major axis
   for the ellipsoidal peak region, and of the inner and outer
   ellipsoids bounding the background region. The same major axis
   lengths are used for all peaks, but the lengths of the other two axes
   of the ellipsoids are adjusted based on the standard deviations of
   the events in those directions. If SpecifySize is false, then the
   major axis length for each peak will be set to include a range of
   plus or minus three times the standard deviation of the events in
   that direction. That is, PeakSize is set to three times the standard
   deviation in the direction of the first principal axis. Also, in this
   case the BackgroundInnerSize is set to the PeakSize and the
   BackgroundOuterSize is set so that the background ellipsoidal shell
   has the same volume as the peak ellipsoidal region. If specified by
   the user, these parameters must be ordered correctly with:
   :math:`0 < PeakSize \leq BackgroundInnerSize < BackgroundOuterSize \leq RegionRadius`

-  The integrated intensities will be set in the specified
   OutputWorkspace. If this is different from the input PeaksWorkspace,
   the input peaks workspace will be copied to the OutputWorkspace
   before setting the integrated intensities.

Detailed Algorithm Description
##############################

This algorithm will integrate a list of indexed single-crystal
diffraction peaks from a PeaksWorkspace, using events from an
EventWorkspace. The indexed peaks are first used to determine a UB
matrix. The inverse of that UB matrix is then used to form lists of
events that are close to peaks in reciprocal space. An event will be
added to the list of events for a peak provided that the fractional
h,k,l value of that event (obtained by applying UB-inverse to the
Q-vector) is closer to the h,k,l of that peak, than to the h,k,l of any
other peak AND the Q-vector for that event is within the specified
radius of the Q-vector for that peak.

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
deviations in the direction of the other two princial axes are .8 and .7
times the standard deviation in the direction of the major axis, then
the ellipse will extend only .8 and .7 times as far in the direction of
those axes, as in the direction of the major axis. Overall, the user
specified sizes for the PeakSize, BackgroundInnerSize and
BackgroundOuterSize are similar to the PeakRadius, BackgroundInnerRadius
and BackgrounOuterRadius for the IntegratePeaksMD algorithm. The
difference is that the regions used in this algorithm are not spherical,
but are ellipsoidal with axis directions obtained from the principal
axes of the events near a peak and the ellipsoid shape (relative axis
lengths) is determined by the standard deviations in the directions of
the principal axes.

Second, if the user does not specifiy the size of the peak and
background ellipsoids, then the three axes of the peak ellipsoid are
again set to the principal axes of the set of nearby events but in this
case their axis lengths are set to cover a range of plus or minus three
standard deviations in the axis directions. In this case, the background
ellipsoidal shell is chosen to have the same volume as the peak
ellipsoid and it's inner surface is the outer surface of the peak
ellipsoid. The outer surface of the background ellipsoidal shell is an
ellipsoidal surface with the same relative axis lengths as the inner
surface.

.. categories::
