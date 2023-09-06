.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm combines pixel calibration results (:math:`prev` subscript below) from the ``PixelCalibration`` workspace with the calibration of focussed data from :ref:`PDCalibration <algm-PDCalibration-v1>` (:math:`pd` subscript below).
The :math:`arb` subscript below denotes values found in the ``CalibrationWorkspace``.
This is part of the larger workflow for :ref:`powder diffraction calibration <Powder Diffraction Calibration>`.

.. warning::

   The ``GroupedCalibration`` table is assumed to not have :math:`DIFA` or :math:`TZERO` set.

Input arguments
---------------

* ``PixelCalibration`` is the previous calibration result

  * Has values for each detector pixel
  * Provides :math:`DIFC_{prev}`, :math:`DIFA_{prev}`, and :math:`TZERO_{prev}`

* ``GroupedCalibration``

  * Has values for each detector pixel, but the values are generally the same within a focused group
  * Provides :math:`DIFC_{pd}`
  * Is the ``OutputCalibrationTable`` from previous :ref:`PDCalibration <algm-PDCalibration>` execution

* ``CalibrationWorkspace``

  * Has values for each spectrum which are grouped pixels
  * Provides :math:`DIFC_{arb}`
  * Is the ``InputWorkspace`` from previous :ref:`PDCalibration <algm-PDCalibration>` execution

How values are combined
-----------------------

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
