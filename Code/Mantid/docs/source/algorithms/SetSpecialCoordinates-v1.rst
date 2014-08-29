.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

`MDEventWorkspaces <http://www.mantidproject.org/MDEventWorkspace>`_ and
:ref:`MDHistoWorkspaces <MDHistoWorkspace>` can be used with any type of
coordinate system. On the other hand
`PeaksWorkspaces <http://www.mantidproject.org/PeaksWorkspace>`_ may be plotted either in QLab,
QSample or HKL. There is an inherent link between a PeaksWorkspace and a
MDWorkspace in that an MDWorkspace may utilise the same coordinate
systems as the PeaksWorkspaces. For example, workspaces created via
:ref:`algm-ConvertToMD` or
:ref:`algm-ConvertToDiffractionMDWorkspace`
may be generated in a special set of V3D coordinates, which are the same
as those for the PeaksWorkspace (QLab, QSample, HKL). Amongst other
usages, in order to be able to simultaneously plot MDWorkspaces and
PeaksWorkspaces, it is necessary to be able to determine what (if any)
special coordinates the Workspaces were created in, or are currently
using.

This algorithm is for backwards compatibility. The special coordinates
flags are new, and legacy workspaces will need to be corrected in order
for them to work as expected with the Mantid tools.

.. categories::
