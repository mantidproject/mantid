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

Workflow
--------

.. diagram:: IndirectILLReductionFWS-v1_wkflw.dot

Usage
-----

**Example: EFWS+IFWS**

.. testcode:: ExFixedWindowScans

    out = IndirectILLFixedWindowScans(Run='143718')
    print "out now refers to a group workspace, which is called %s" % out.getName()
    print "it contains %d item, one for each energy transfer" % out.size()

Output:

.. testoutput:: ExFixedWindowScans

    out now refers to a group workspace, which is called out
    it contains 1 item, one for each energy transfer

.. categories::

.. sourcelink::
