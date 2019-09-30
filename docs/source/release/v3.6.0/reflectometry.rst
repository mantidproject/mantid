=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:

ReflectometryReductionOne
-------------------------

A new algorithm, :ref:`RotateSource <algm-RotateSource>`,
to move the source to the correct position, was added `#14707 <https://github.com/mantidproject/mantid/pull/14707>`

:ref:`RotateSource <algm-RotateSource>` algorithm has been implemented so that source position is
rotated before the conversion from IvsLambda to IvsQ to ensure that the
correct Q range is calculated. `#14942 <https://github.com/mantidproject/mantid/pull/14942>`_

To sustain continuity in the instrument setup the :ref:`RotateSource <algm-RotateSource>` algorithm
is applied again but in the opposite direction by the same rotation
theta. This fixed some bugs we were seeing when other algorithms that
relied on the source position were producing the wrong results `#15139 <https://github.com/mantidproject/mantid/pull/15139>`_

ConvertToReflectometryQ
-----------------------

:ref:`ConvertToReflectometryQ <algm-ConvertToReflectometryQ>` can now output any transformation (Centre point
or NormalisedPolygon) in the form of an :ref:`MDHistoWorkspace <MDHistoWorkspace>`. See
``OutputAsMDWorkspace property`` in the :ref:`documentation <algm-ConvertToReflectometryQ>`.

A bug that was causing :ref:`ConvertToReflectometryQ <algm-ConvertToReflectometryQ>` to output wrong values
around Qx ~ 0 was fixed `#14201 <https://github.com/mantidproject/mantid/pull/14201>`_

A bug that was causing :ref:`ConvertToReflectometryQ <algm-ConvertToReflectometryQ>` to output NaN's at one of
the edges was fixed `#14283 <https://github.com/mantidproject/mantid/pull/14283>`_ (this fix
has also been applied to the PiPf and KiKf transformations
`#14455 <https://github.com/mantidproject/mantid/pull/14455>`_)

A bug in PolygonIntersection was fixed. It was affecting
:ref:`ConvertToReflectometryQ <algm-ConvertToReflectometryQ>` when the number of pixels was large (~3000x3000)
`#14477 <https://github.com/mantidproject/mantid/issues/14477>`_

Reflectometry Reduction Interface
---------------------------------

ISIS Reflectometry (Polref)
###########################

The Reflectometry Polref Interface can now import a `.tbl` file straight
into the processing table instead of the two-step process of importing
and then opening the table that was previously implemented. This change
mimics the behaviour of the old Reflectometry reduction Interface
`#14709 <https://github.com/mantidproject/mantid/pull/14709>`__

The name of the table that is currently in the table should appear in
the title of the interface window i.e "ISIS Reflectometry (Polref) -
table1"

A bug has been fixed that appeared when the user tried to open a new
table when the table currently in the interface had been edited. This
bug caused a pop-up saying "You have unsaved changes in your table,
would you like to discard these changes?" to appear multiple times if
the user clicked the "No" button.
`#14966 <https://github.com/mantidproject/mantid/pull/14966>`_

ISIS Reflectometry
##################

Another bug that would reset your table back to original values if you
chose 'Cancel' when asked if you would like to discard your changes.
`#15083 <https://github.com/mantidproject/mantid/pull/15083>`_

Measurement Based Reduction Grouping
####################################

Transferring of runs will now automatically assemble the batch
processing table using the measurement logs in the isis nexus files.

If the measurement log is invalid it result in a failed transfer and the
row in the search table corresponding to this run will be highlighted.
Hovering the cursor over failed transfers will show you why the transfer
failed i.e "Invalid measurement ID".

Momentum Transfer calculations
##############################

-  Some algorithms either returning theta or two theta, but labelling
   the output incorrectly. One candidate being
   :ref:`SpecularReflectionCalculateTheta <algm-SpecularReflectionCalculateTheta>`.
   **This change could affect results when using** :ref:`ReflectometryReductionOne <algm-ReflectometryReductionOne>`.
-  :math:`|Q|` created via :ref:`ConvertUnits <algm-ConvertUnits>`
   and Qx, Qz maps created via
   :ref:`ConvertToReflectometryQ <algm-ConvertToReflectometryQ>`
   incommensurate. Source component not being located correctly.

SlitCalculator
##############

The SlitCalculator tool for this interface will now retrieve values for:
Distance between first and second slit and distance between the second
slit and the sample container, using the Instrument Definition file of
the processing/search instrument. This functionality will work even if
SlitCalculator is open when you change the instrument associated with
the interface.
`#14077 <https://github.com/mantidproject/mantid/pull/14077>`_

Other Changes
-------------

A bug has been fixed in :ref:`SpecularReflectionPositionCorrect <algm-SpecularReflectionPositionCorrect>` whereby the Y
shift was not being correctly calculated
`#14376 <https://github.com/mantidproject/mantid/pull/14376>`_

`Full list of changes on github <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.6%22+is%3Amerged+label%3A%22Component%3A+Reflectometry%22>`__
