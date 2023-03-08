.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Takes a list of exported UB files (from SaveISawUB) and saves (in the default save directory) UB files that are consistent with the first UB file specified such that the same reflection in different runs has the same index (i.e. no inversion or swapping of crystallographic axes relative to the reference UB). Note this algorithm assume the sample is perfectly on the centre of rotation.

.. figure:: /images/FindGoniometerFromUB_schematic.PNG
   :alt: FindGoniometerFromUB_schematic.PNG

   Fig.1: Schematic of the coordinate system used and definition of the goniometer axes. The arrow associated with an angle denote the counter clockwise rotation which corresponds to a handedness of +1 (right-handed). If the log file angles are for a clockwise rotation then the corresponding handedness shuold be specified as -1 in the input to the algroithm.

The algorithm uses the right-hand coordinate system where X is the beam direction and Z is vertically upward (IPNS convention). The algorithm considers two rotation axes: rotation about the Z-axis, :math:`\Omega` ,and rotation of :math:`\phi` around the goniometer axis. The angles :math:`\Omega` and :math:`\phi` for each run are read from the first value of the specified log entry in the run log files.
The goniometer axis is defined by the angle from the vertical, :math:`\chi`, and the angle between the component of the goniometer axis in the scattering plane (XY) from the positive Y-axis, :math:`d\Omega`, defined when :math:`\Omega=0` - for which nominal/best-guess values need to be supplied as arguments.

The test for consistency requires some tolerances to be supplied on :math:`\chi` and :math:`\phi`, if the nominal goniometer axis is not within those tolerances the most likely permutation of the axes is tried (see :ref:`algm-FindPeaksMD`) and the consistency is checked again. The algorithm displays a table with the actual :math:`\chi` and :math:`\phi` and the unit vector of the goniometer axis for each run for which a consistent UB was found

Useage
-----------

**Example:**

.. code-block:: python

    from mantid.simpleapi import *

    ubList = ['WISH00043350', 'WISH00043351', 'WISH00043352','WISH00043353','WISH00043354']
    tab = FindGoniometerFromUB(UBfiles=",".join(ubList), chi=45, chiTol=5, phiTol=10,
                               phiHand=-1, dOmega=0, omegaHand=1)

.. categories::

.. sourcelink::
