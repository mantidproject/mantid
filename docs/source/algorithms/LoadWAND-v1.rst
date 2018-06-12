.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm uses :ref:`algm-LoadEventNexus` to load a WAND² data
file after which it will integrate out the events, apply a standard
mask, change units to wavelength and set the wavelength, set the
goniometer, and set the proton charge to be the number of monitor
counts.

The standard mask includes the top and bottom 2 rows of pixels and the
last 6 columns for run numbers up to 26600 or the first and last 2
columns for larger run numbers.

After this algorithm loads the workspace it can be correctly converted
to Q sample or HKL using :ref:`algm-ConvertToMD`. If it's a powder
sample it can be reduced with :ref:`algm-WANDPowderReduction`.

If you need to do event filtering don't use this algorithm, simply use
:ref:`algm-LoadEventNexus` and convert to data manually.

You can specify multiple files in the filename property or set the
IPTS and RunNumbers, where RunNumbers can be a range or list of
numbers, see Usage example bellow. If multiple files are loaded they
will be named 'OutputWorkspace'+'_runnumber' and be grouped in
'OutputWorkspace'.

Usage
-----

**Example - LoadWAND**

.. code-block:: python

    silicon = LoadWAND('/HFIR/HB2C/IPTS-7776/nexus/HB2C_26506.nxs.h5')
    print("Workspace has {0} spectrum and {1} point in units {2}".format(silicon.getNumberHistograms(),
                                                                         silicon.blocksize(),
                                                                         silicon.getXDimension().name))

Output:

.. code-block:: none

    Workspace has 1966080 spectrum and 1 point in units Wavelength

**Load using Multiple file**

.. code-block:: python

    silicon = LoadWAND('/HFIR/HB2C/IPTS-7776/nexus/HB2C_26506.nxs.h5,/HFIR/HB2C/IPTS-7776/nexus/HB2C_26507.nxs.h5')
    print("Workspace group {0} has {1} workspaces {2} and {3}".format(silicon.name(),
                                                                      silicon.getNumberOfEntries(),
                                                                      silicon.getNames()[0],
                                                                      silicon.getNames()[1]))

Output:

.. code-block:: none

    Workspace group silicon has 2 workspaces silicon_26506 and silicon_26507

**Load using IPTS and run numbers**

.. code-block:: python

    # Comma-separated list
    silicon = LoadWAND(IPTS=7776,RunNumbers='26506,26507')
    # or range
    silicon = LoadWAND(IPTS=7776,RunNumbers='26506-26507')
    print("Workspace group {0} has {1} workspaces {2} and {3}".format(silicon.name(),
                                                                      silicon.getNumberOfEntries(),
                                                                      silicon.getNames()[0],
                                                                      silicon.getNames()[1]))

Output:

.. code-block:: none

    Workspace group silicon has 2 workspaces silicon_26506 and silicon_26507

.. categories::

.. sourcelink::
