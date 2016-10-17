.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm selects fixed window scan data of the IN16B instrument at ILL. An integration will be performed over the elastic peak range or the begin and end ranges of the elastic or inelastic data, respectively.
The integration range will be determined automatically taking into account the relevant peaks as well as the overall number of bins.
An error occurs, if there are no elastic or inelastic scan data found.
All output workspaces are GroupWorkspaces.

See Also
########

-  :ref:`algm-IndirectILLReduction` for reducing quasi-elastic and fixed window scanned data.


Usage
-----

**Example: Appending two workspaces**

.. testcode:: ExFixedWindowScans

Output:

.. testoutput:: ExFixedWindowScans


.. categories::

.. sourcelink::
