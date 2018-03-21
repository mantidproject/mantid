.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm uses :ref:`algm-LoadEventNexus` to load a WAND² data
file after which it will integrate out the events, apply a standard
mask, change units to wavelength and set the wavelength, set the
goniometer, and set the proton charge to be the number of monitor
counts. The standard mask includes the top and bottom 2 rows of pixels
and the last 6 columns.

After this algorithm loads the workspace it can be correctly converted
to Q sample or HKL using :ref:`algm-ConvertToMD`.

If you need to do event filtering don't use this algorithm, simply use
:ref:`algm-LoadEventNexus` and convert to data manually.

Usage
-----

**Example - LoadWAND**

.. code-block:: python

    ws = LoadWAND('HB2C_7000.nxs.h5')
    print("ws has {0} spectrum and {1} point in units {2}".format(ws.getNumberHistograms(),
                                                                  ws.blocksize(),
                                                                  ws.getXDimension().name))

Output:

.. code-block:: none

    ws has 1966080 spectrum and 1 point in units Wavelength

.. categories::

.. sourcelink::
