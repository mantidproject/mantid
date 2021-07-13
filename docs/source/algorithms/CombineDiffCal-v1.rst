.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm combines pixel calibration results (:math:`prev` subscript below) from the ``PixelCalibration`` workspace with the calibration of focussed data from :ref:`PDCalibration <algm-PDCalibration-v1>` (:math:`pd` subscript below).
The :math:`arb` subscript below denotes values found in the ``CalibrationWorkspace``.

.. note::

   The ``GroupedCalibration`` table is assumed to not have :math:`DIFA` or :math:`TZERO` set.
   It is also assumed to have calibration values for **all** detector pixels.

The effective calibration values are calculated using the following equations

.. math::

    DIFC_{eff} = \frac{DIFC_{pd}}{DIFC_{arb}} * DIFC_{prev}

where :math:`DIFC_{pd}`  is from :ref:`PDCalibration <algm-PDCalibration-v1>`,  :math:`DIFC_{arb}` is found in the parameter ``CalibrationWorkspace`` that was the input workspace to ``PDCalibration``, and :math:`DIFC_{prev}` is from the previous calibration.
The value of :math:`TZERO` is unchanged

.. math::

   TZERO_{eff} = TZERO_{prev}

The value of :math:`DIFA` is updated by

.. math::

    DIFA_{eff} = \left( \frac{DIFC_{pd}}{DIFC_{arb}} \right)^2 * DIFA_{prev}


If pixels are masked in ``MaskWorkspace``, missing from ``PixelCalibration``, or missing from the list of contributing pixels in ``CalibrationWorkspace``, then the calibration constants found in ``GroupedCalibration`` will be copied.

.. categories::

.. sourcelink::
