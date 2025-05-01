.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm was developed for the Merlin instrument but may be used
with other instruments if appropriate scaling data is available. The
scaling data should give the true centre point location of each pixel
detector in the instrument. This may be obtained by a calibration run
and post-processing of the results. Since the calibration data may vary
with time, it is not convenient to store it in the instrument XML
definition file. Instead it can be stored as an ACSII file with the
extension ".sca" or within the ".raw" file associated with the data, as
data on the position of each detector (r,theta,phi).

A scaling file (extension .sca) is expected to be an ASCII file with
three header lines. Of these, only the second line is actual read and
the first item on this line should give the number of detectors
described by the file as an integer value. Each subsequent line after
the first three will give the information for one detector with at least
the five ordered values detector\_ID, detector\_offset, l2, code, theta
and phi. Of these values only the detector\_ID and the new position (l2,
theta, phi) are used. The latter three values are taken as defining the
true position of the detector in spherical polar coordinates relative to
the origin (sample position). If a raw file is given the true positions
are taken from this instead.

This algorithm creates a parameter map for the instrument that applies a
shift to each detector so that is at the correct position. Monitors are
not moved. Because the shift of detector locations can alter the
effective width of the pixel it is necessary to apply a scaling factor.
While each shift can have components in all three primary axes (X,Y,Z),
it is assumed that a single PSD will maintain the co-linear nature of
pixel centres. The width scaling factor for a pixel i is approximated as
average of the left and right side scalings cased by the change in
relative spacings with respect to neighbour pixels. End of detector
pixels only have one scaling value to use. It is assumed that the
scaling is both small and smooth so that the approximate scaling is
reasonable.

Scaling and position correction will be reflected in properties of the
detector objects including values such as the solid angle, bounding box,
etc. The detector numbering in Merlin uses sequential numbers for pixels
within a PSD and non-sequential jumps between PSDs. This algorithm uses
these jumps to identify individual PSDs.

To apply this algorithm to instruments other than Merlin it may be
necessary to modify the code depending on the type of detectors present
and how they are numbered.

If the tube detector performance enhancement is used the results of the
algorithm will not be visible in the instrument viewer, however
all subsequent calculations on the workspace will be performed correctly.

Optional properties
###################

ScalingOpt - this integer value controls the way in which the scaling is
calculated for pixels that have both left and right values for the
scaling. The default is to just average the two together. Setting this
to 1 causes the maximum scaling to be used and setting it to 2 uses the
maximum scaling plus 5% to be used.

ChildAlgorithms used
####################

None

.. categories::

.. sourcelink::
