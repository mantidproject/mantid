.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

.. warning::

   This algorithm is being developed for a specific instrument. It might get changed or even 
   removed without a notification, should instrument scientists decide to do so.

Fits the data of every detector pixel using expected Bragg peak positions.

Allows to correct for tiny variations in pixel parameters.

The result of the calibration is accepted by both :ref:`algm-EnginXCalibrate` and 
:ref:`algm-EnginXFocus` to correct the detector positions before focussing.

Expects the *long* calibration run, which provides a decent pattern for every pixel.

.. categories::
