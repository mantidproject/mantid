.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm performs Fixed Window Scan (FWS) data reduction (both Elastic and Inelastic) for IN16B indirect geometry instrument at ILL.
Scanning observable can be in principle any sample parameter. For the list of valid options check the Sample Logs of a loaded IN16B data file.
It is able to treat both two-wing and one-wing data, although in one call, they should not be mixed (see the `mirror_sense` discussion in :ref:`algm-IndirectILLReduction`)

Input
-----
Multiple files following the syntax given in `MultiFileLoading <http://www.mantidproject.org/MultiFileLoading>`_.

Output
------
A GroupWorkspace that contains the results, one per each of energy value (including 0 for EFWS).
Each Workspace in the group will have the given observable as the x-axis, and scattering angle for each detector as y-axis.

See Also
########

-  :ref:`algm-IndirectILLReduction` for performing the actual energy transfer reduction for IN16B instrument.

Usage
-----

**Example: EFWS+IFWS**

.. testcode:: ExFixedWindowScans

Output:

.. testoutput:: ExFixedWindowScans


.. categories::

.. sourcelink::
