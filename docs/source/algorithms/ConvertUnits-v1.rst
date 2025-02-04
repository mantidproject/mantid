.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Changes the units in which the X values of a :ref:`workspace <Workspace>`
are represented. The available units are those registered with the :ref:`Unit
Factory <Unit Factory>`. If the Y data is 'dimensioned' (i.e. has been
divided by the bin width), then this will be correctly handled, but at
present nothing else is done to the Y data. If the sample-detector
distance cannot be calculated then the affected spectrum will be zeroed,
and a warning message will be output on the :ref:`logging <05_logging>`
service.

If AlignBins is false or left at the default the output workspace may be
a :ref:`ragged workspace <Ragged_Workspace>`. If it is set to true then the
data is automatically :ref:`rebinned <algm-Rebin>` to a regular grid so that the
maximum and minimum X values will be maintained. It uses the same number
of bins as the input data, and divides them linearly equally between the
minimum and maximum values.

If converting to :math:`\Delta E` any bins which correspond to a
physically inaccessible will be removed, leading to an output workspace
than is smaller than the input one. If the geometry is indirect then the
value of EFixed will be taken, if available, from the instrument
definition file.

If ConvertFromPointData is true, an input workspace
contains Point data will be converted using :ref:`ConvertToHistogram <algm-ConvertToHistogram>`
and then the algorithm will be run on the converted workspace.

Applying instrument calibration
###############################

Some workflows involve applying a calibration in the form of updated detector positions. In the case of
time-of-flight (TOF) diffraction it is also possible to provide detector offsets
(from :ref:`GetDetectorOffsets <algm-GetDetectorOffsets>`) or diffractometer constants
(from :ref:`PDCalibration <algm-PDCalibration>`) for the conversion between TOF and d-spacing.
For more details on diffractometer constants see :ref:`Unit Factory`, for the relation between offsets and diffractometer
constants see ref:`ConvertDiffCal <algm-ConvertDiffCal>`.

:ref:`ApplyDiffCal <algm-ApplyDiffCal>` is used to apply a detector calibration in the form of a ``CalibrationFile``,
``CalibrationWorkspace`` or ``OffsetsWorkspace``. Note this calibration will be used to do the conversion from d-spacing
to TOF unless the user calls :ref:`ApplyDiffCal <algm-ApplyDiffCal>` again with ``ClearCalibration=True`` after the call
to :ref:`ConvertUnits <algm-ConvertUnits>` (to reproduce the behaviour of the deprecated ``AlignDetectors`` algorithm).


Restrictions on the input workspace
###################################

-  Naturally, the X values must have a unit set, and that unit must be
   known to the :ref:`Unit Factory <Unit Factory>`.
-  Histograms and Point data can be handled.
-  The algorithm will also fail if the source-sample distance cannot be
   calculated (i.e. the :ref:`instrument <instrument>` has not been
   properly defined).

Available units
---------------

The units currently available to this algorithm are listed
:ref:`here <Unit Factory>`, along with equations specifying exactly how the
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
