.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

RingProfile sums the counts against a ring.

Below, there is an example of the execution of the RingProfile to a
`Workspace2D <Workspace2D>`__ where the position of the pixels are not
associated to detector positions, but it is derived from the
`Axes <Interacting_with_Matrix_Workspaces#Axes>`__.

.. figure:: /images/ExecuteRingProfile.png 
   :width: 800px
   :align: right

The image below shows a visual interpretation for the inputs of the
algorithm

.. figure:: /images/RingProfileInputsView.png



The algorithm goes through each pixel and find its distance from the
center. If it relies inside the defined ring, it checks the angle
between the pixel position and the center and uses this information to
define the bin where to put the count for that pixel.

The RingProfile is also defined for Workspace2D which has the positions
based on the detectors, as you can see in the picture below.

.. figure:: /images/RingProfileInstrument.png 
   :width:  801px 

In this case, the inputs of the algorithm is like the image below

.. figure:: /images/Ringprofileinstrument1.png

The algorithm does to each spectrum, get the associated detector from
which it get the positions. From the positions it work out if it belongs
or not to the ring and in which bin it must be placed. It finally
accumulate all the spectrum values inside the target bin.

It is possible to setup the *StartAngle* from where to starting the Ring
as well as the Sense, if in clockwise direction or anti-clockwise
direction. But, the resulting workspace will always place the bins in a
relative angle position from the start. Which means that for
anti-clockwise sense, the real 3D angle is:

RealAngle = StartAngle + Angle

While for clockwise sense, the real 3D angle is:

RealAngle = StartAngle - Angle

.. categories::
