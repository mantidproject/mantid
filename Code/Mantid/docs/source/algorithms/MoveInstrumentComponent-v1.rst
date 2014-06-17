.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This moves an instrument component, e.g. a bank or a pixel.

You can specify a pathname as the name of a non-unique component (e.g.
"WISH/panel03/WISHpanel03/tube005") and one can skip parts not needed
for uniqueness (e.g. "panel03/tube005"). For a unique component, you can
just specify the name (e.g. "panel03").

You can either specify an absolute position or a relative position. The
relative position will be applied to the current position, so applying
this twice will move the detector twice.

.. categories::
