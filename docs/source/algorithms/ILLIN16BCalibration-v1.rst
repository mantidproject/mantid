.. algorithm::

.. summary::

.. alias::

.. properties::

.. warning::

   This algorithm is deprecated (20-Nov-2016). Please, use :ref:`IndirectILLReductionQENS <algm-IndirectILLReductionQENS>` instead.

Description
-----------

A workflow algorithm to generate a calibration workspace for the IN16B
spectrometer at the ILL.

This first reduces the data using the :ref:`IndirectILLReduction
<algm-IndirectILLReduction>` algorithm and then integrates each spectrum within
the peak range.

Workflow
--------

.. diagram:: ILLIN16BCalibration-v1_wkflw.dot

Usage
-----

**Example - Creating a calibration workspace**

.. testcode:: ExILLIN16BCalibration

    calibration_ws = ILLIN16BCalibration(Run='ILLIN16B_034745.nxs',
                                         PeakRange=[-0.001,0.002],
                                         MirrorMode=True)

    print 'Calibration workspace has %d spectra and %d bin(s)' % (
           calibration_ws.getNumberHistograms(), calibration_ws.blocksize())

Output:

.. testoutput:: ExILLIN16BCalibration

    Calibration workspace has 18 spectra and 1 bin(s)

.. categories::

.. sourcelink::
