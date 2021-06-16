.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Mask pixels in an Workspace close to peak positions from a
PeaksWorkspace.
Peaks could come from ISAW diamond stripping routine for
SNAP data.
This algorithm was originally developed for instruments with
RectangularDetector's, but is now adapted to work for tube type
detector of the two instruments (WISH and CORELLI).

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

Note
-----

Running the algorithm will trigger a deprecation warning regarding the `SpectraList`.
This is a known issue and will be addressed at a later date.

.. categories::

.. sourcelink::
