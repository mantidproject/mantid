.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm allows users to adjust the axes of a workspace by a user
defined math formula. It will NOT adjust or rearrange the data values
(other than in one case the X values) of a workspace. Therefore
alterations that will rearrange the order of the axes are not
recommended. This only works for MatrixWorkspaces, so will not work on
Multi Dimensional Workspaces or Table Workspaces. Like the
:ref:`algm-ConvertSpectrumAxis` algorithm the result of
this algorithm will have custom units defined for the axis you have
altered, and as such may not work in all other algorithms.

The algorithm can operate on the X or Y axis, but cannot alter the
values of a spectrum axis (the axis used as the Y axis on newly loaded
Raw data). If you wish to alter this axis use he
:ref:`algm-ConvertSpectrumAxis` algorithm first.

The formula is defined in a simple math syntax, please look at the usage
examples to some ideas of what is possible, a full list of the functions
available can be found at the muparser website
`1 <http://muparser.beltoforion.de/mup_features.html#idDef2>`__.

.. categories::
