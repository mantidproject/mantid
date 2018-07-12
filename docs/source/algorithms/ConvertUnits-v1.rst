.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Changes the units in which the X values of a :ref:`workspace <Workspace>`
are represented. The available units are those registered with the `Unit
Factory <http://www.mantidproject.org/Units>`__. If the Y data is 'dimensioned' (i.e. has been
divided by the bin width), then this will be correctly handled, but at
present nothing else is done to the Y data. If the sample-detector
distance cannot be calculated then the affected spectrum will be zeroed,
and a warning message will be output on the `logging <http://www.mantidproject.org/logging>`__
service.

If AlignBins is false or left at the default the output workspace may be
a :ref:`ragged workspace <Ragged_Workspace>`. If it is set to true then the
data is automatically `rebinned <http://www.mantidproject.org/Rebin>`__ to a regular grid so that the
maximum and minimum X values will be maintained. It uses the same number
of bins as the input data, and divides them linearly equally between the
minimum and maximum values.

If converting to :math:`\Delta E` any bins which correspond to a
physically inaccessible will be removed, leading to an output workspace
than is smaller than the input one. If the geometry is indirect then the
value of EFixed will be taken, if available, from the instrument
definition file.

If ConvertFromPointData is true, an input workspace
contains Point data will be converted using `ConvertToHistogram <http://www.mantidproject.org/ConvertToHistogram>`__
and then the algorithm will be run on the converted workspace.

Restrictions on the input workspace
###################################

-  Naturally, the X values must have a unit set, and that unit must be
   known to the `Unit Factory <http://www.mantidproject.org/Units>`__.
-  Histograms and Point data can be handled.
-  The algorithm will also fail if the source-sample distance cannot be
   calculated (i.e. the :ref:`instrument <instrument>` has not been
   properly defined).

Available units
---------------

The units currently available to this algorithm are listed
`here <http://www.mantidproject.org/Units>`__, along with equations specifying exactly how the
conversions are done.

Usage
-----

**Example: Convert to wavelength**

.. testcode:: ExConvertUnits

    ws = CreateSampleWorkspace("Histogram",NumBanks=1,BankPixelWidth=1)
    wsOut = ConvertUnits(ws,Target="Wavelength")

    print("Input {}".format(ws.readX(0)[ws.blocksize()-1]))
    print("Output {:.11f}".format(wsOut.readX(0)[wsOut.blocksize()-1]))

Output:

.. testoutput:: ExConvertUnits

    Input 19800.0
    Output 5.22196485301


.. categories::

.. sourcelink::
