.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Mask pixels in an Workspace close to peak positions from a
PeaksWorkspace. Peaks could come from ISAW diamond stripping routine for
SNAP data. Only works on Workspaces and for instruments with
RectangularDetector's.

Usage
-----

**Example:**

.. To usage test this properly you need a matching dataset and peaks workspace,
   the unit tests do this, but that functionality is not available in algorithms.
   You could load files, but the basic data would be too big to download sensisbly.
   Therefore this is an untested code block just showing usage

.. code-block:: python

    #the files for this example are not available for download
    #This is to illustrate usage only
    ws = Load("TOPAZ_3007.nxs")
    wsPeaks=LoadIsawPeaks("TOPAZ_3007.peaks")
    #This will mask out the data in the event workspace relating to the peaks
    MaskPeaksWorkspace(ws,wsPeaks)


.. categories::

.. sourcelink::
