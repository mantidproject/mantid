
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Saves a processed nexus file similar to :ref:`algm-SaveNexusProcessed`, but provides nexus geometry which is an accurate snapshot of the calibrated/transformed instrument geometry in-memory. One current major difference between the two algorithms is that SaveNexusESS does not support the generation of a single processed file based on a :ref:`GroupWorkspace <WorkspaceGroup>` input.

The algorithm writes out spectra-detector mappings and can handle detector groupings.

This algorithm may be deprecated in future in favour of a master :ref:`algm-SaveNexusProcessed` algorithm.

This algorithm currently provides not shape information for component geometry.

Usage
-----
..  Try not to use files in your examples,
    but if you cannot avoid it then the (small) files must be added to
    autotestdata\UsageData and the following tag unindented
    .. include:: ../usagedata-note.txt

**Example - SaveNexusESS**

.. testcode:: SaveNexusESSExample

    from mantid.simpleapi import *
    import os
    import tempfile
    simple_run = CreateSampleWorkspace(NumBanks=2, BankPixelWidth=10)
    destination = os.path.join(tempfile.gettempdir(), "sample_processed.nxs")
    SaveNexusESS(Filename=destination, InputWorkspace=simple_run)
    print("Created: {}".format(os.path.isfile(destination)))
    os.remove(destination)

Output:

.. testoutput:: SaveNexusESSExample

  Created: True

.. categories::

.. sourcelink::
