.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is for use with white-beam instruments with choppers. The
chopper cuts the range of wavelengths, so all detectors (including
monitors) should be reduced to the same wavelength range. This is done
using a "reference" flightpath, :math:`L_{ref}`, which is (usually, see
below) the flightpath of the farthest detectors.

If :math:`T_{min}` and :math:`T_{max}` are the beginning and end of the
frame, for each detector :math:`D` at total flightpath :math:`L_d` the
following times are defined:

.. math:: T_1 = T_{max} - \left ( T_{min} \times \left ( 1 - \frac{L_d}{L_{ref}} \right ) \right )

.. math:: T_2 = T_{max} \times \left ( \frac{L_d}{L_{ref}} \right )

Thus if :math:`L_d < L_{ref}` then :math:`T_1 > T_2`

Neutron velocities (and hence wavelengths) for the detector :math:`D`
are calculated in the following way:

-  For :math:`T_{min} < T < T_2`, velocities are calculated in the usual
   way, i.e. :math:`v = \frac{L_d}{T}`

-  Data in the range :math:`T_2 < T < T_1` are ignored (i.e. they are
   not used in the wavelength-converted histograms)

-  For :math:`T_1 < T < T_{max}`, velocities are calculated using the
   formula :math:`v = \frac{L_d}{T - T_{max} + T_{min}}`

Note that the minimum and maximum velocities for the points that are
actually *used* are:

.. math:: v_{max} = \frac{L_d}{T - T_{max} + T_{min}} = \frac{L_{ref}}{T_{min}}

and :math:`v_{min} = \frac{L_d}{T_2} = \frac{L_{ref}}{T_{max}}`

In other words, these velocities are the same for all detectors, so the
wavelength range in the transformed histogram will correspondingly be
the same and this algorithm rebins the data into common bins in this
range.

Occasionally, it may be that some detectors (typically downstream
monitors) may be at a \*longer\* flightpath than :math:`L_{ref}`. This
depends entirely on the chopper aperture/setting. These detectors are
"frame-overlapped" - in other words, there is an ambiguity in the
definition of the wavelength for certain points, which should therefore
be excluded. These points are at the very beginning and at the very end
of the frame for a range :math:`Dt` (equal on both sides) given by

.. math:: D_t = (T_{max} - T_{min}) \times \left (1 - \frac{L_{ref}}{L_d} \right)

In other words, points between :math:`T_{min}` and :math:`T_{min} + D_t`
and between :math:`T_{max} - D_t` and :math:`T_{max}` should be
excluded. For all other points, velocities and wavelengths are
calculated in the normal way.

Note that since we are dealing with histogrammed data, the cut-off
values above will almost certainly not fall exactly on a bin boundary.
The approach taken by this algorithm is that if any part of a bin has a
value that should be excluded, then the entire bin is excluded. What
this means in practice is that the edge bins will possibly have a
reduced number of counts.

Restrictions on the Input Workspace
###################################

The input workspace must contain histogram data where the X unit is
time-of-flight and the Y data is raw counts. The instrument associated with the
workspace must be fully defined because detector, source & sample position are needed.

Child Algorithms Used
#####################

If the input workspace contains more than a single spectrum, Unwrap
makes use of the :ref:`rebin <algm-Rebin>` algorithm to set the bins on the
output workspace to common values which cover the maximum theoretically
accessible wavelength range.

Usage
-----

**Example - unwrapping a raw workspace.**

.. testcode:: UnwrapMonitor

   # Create a raw data workspace.
   ws = CreateSampleWorkspace()

   # The result variable will contain a tuple: (OutputWorkspace, JoinWaveLength)
   # To access individual outputs use result[i] where i is the index of the required output.
   result = UnwrapMonitor(InputWorkspace=ws,LRef=11)

   print("JoinWaveLength is: " + str(result[1]))

Output:

.. testoutput:: UnwrapMonitor
   :options: +ELLIPSIS

   JoinWaveLength is: 1.4241722...

.. categories::

.. sourcelink::
