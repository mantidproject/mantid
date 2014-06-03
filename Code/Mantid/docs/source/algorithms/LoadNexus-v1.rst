.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The algorithm LoadNexus will read the given Nexus file and try to
identify its type so that it can be read into a workspace. The file name
can be an absolute or relative path and should have the extension .nxs
or .nx5. Currently only Nexus Muon Version 1 files are recognised, but
this will be extended as other types are supported such as
:ref:`_algm-LoadNexusProcessed`.

If the file contains data for more than one period, a separate workspace
will be generated for each. After the first period the workspace names
will have "\_2", "\_3", and so on, appended to the given workspace name.
For single period data, the optional parameters can be used to control
which spectra are loaded into the workspace. If spectrum\_min and
spectrum\_max are given, then only that range to data will be loaded. If
a spectrum\_list is given than those values will be loaded.

.. categories::
