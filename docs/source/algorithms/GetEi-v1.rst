.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Uses :math:`E=\frac{1}{2}mv^2` to calculate the energy of neutrons leaving the
source. The velocity is calculated from the time it takes for the
neutron pulse to travel between the two monitors whose spectra were
specified.

An initial energy guess is required for the algorithm to find the
correct peak. The analysis will be done on the highest peak that is
within 8% of the estimated TOF given by the estimate.

Not all neutrons arrive at the monitors at the same time because their
kinetic energies, and therefore velocities, are all different. The time
of arrival of the neutron pulse is taken to be the mean of the two half
peak height locations. The half height points are found as follows:

#. the peak height is the largest number of counts above the background
   in any bin in the window
#. the half height is half the above number
#. examine bins to the left of the bin with the highest number of counts
   looking for a bin with less than half that number above background
#. interpolate between this point bin and the one immediately previous
   to find the first half height location
#. repeat the steps 3 and 4 looking to the right of the highest point to
   get the second half height point
#. the mean of the X-values of the two half height points is the TOF
   arrival time of the neutrons

The above process is illustrated on a peak is shown below in the image
below

.. image:: /images/Monitorspect_getei.jpg
    :align: center
    :alt: Monitor Peak
    :width: 618px

The distances between the monitors are read from the instrument
definition file. It is assumed that the source and the monitors all lie
on one line and that the monitors have the same delay time.

.. warning::

    This version is an older algorithm and is no longer in current use, although
    it is not deprecated.

.. categories::

.. sourcelink::
