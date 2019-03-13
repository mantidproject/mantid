.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm performs the data reduction for the D2B instrument at the ILL, and also for D20 when doing a detector
scan.

Input Runs
----------

Provide the list of the input runs, that is the runs corresponding to a single detector scan, following the syntax in
:py:obj:`MultipleFileProperty <mantid.api.MultipleFileProperty>`. Note that for this algorithm ``508093:508095``
and ``508093-508095`` would be equivalent; in either case it will load and process the runs separately and then finally merge them with :ref:`SumOverlappingTubes <algm-SumOverlappingTubes>`.

Calibration
-----------

The NeXus files for D2B contain raw data and pre-calibrated data. Either of these can be used when loading.
Note that, when reading the calibrated data, the even-numbered tubes will not be flipped, since they are flipped in the nexus files already.

Normalisation Options
---------------------

The default is for normalisation to monitor, in which case the counts will be scaled up by 10ˆ6 after dividing by monitor counts.

Output
------

The output from the algorithm is a :py:obj:`WorkspaceGroup <mantid.api.WorkspaceGroup>`, containing the requested
outputs:

* **Output2DTubes** - Outputs a 2D workspace of tube height against scattering angle. In other words with no correction
  for Debye-Scherrer cones. It is expected that this is only used for checking alignment.
* **Output2D** - Outputs a 2D workspace of height along tube against scattering angle for pixel in tube. Here
  there is effectively a correction for the Debye-Scherrer cones.
* **Output1D** - Outputs a 1D workspace of counts against scattering angle. The vertical integration range for this
  is set in the ``HeightRange`` option.

Note for D20 only the ``Output1D`` option will be relevant.

For ``Output2DTubes`` only the negative scattering angles are included, they are excluded for ``Output2D`` and
``Output1D``.


Use :ref:`SaveFocusedXYE <algm-SaveFocusedXYE>` to save in FullProf format #10, or :ref:`SaveGSS <algm-SaveGSS>` for
GSAS format.

Workflow
--------

.. diagram:: PowderILLDetectorScan-v1_wkflw.dot

Related Algorithms
------------------

:ref:`PowderILLParameterScan <algm-PowderILLParameterScan>` can be used for D20 where the detector is static, and
another variable such as temperature is scanned instead.

Usage
-----

**Example - PowderILLDetectorScan**

.. testsetup:: ExPowderILLDetectorScan

   config['default.facility'] = 'ILL'
   config['default.instrument'] = 'D2B'
   config.appendDataSearchSubDir('ILL/D2B/')

.. testcode:: ExPowderILLDetectorScan

   red_ws = PowderILLDetectorScan(Run='508093:508095', Output2DTubes=True, Output2D=True, Output1D=True)
   print("'2DTubes' output workspace has {0} diffractograms having {1} bins each".format(red_ws[0].getNumberHistograms(), red_ws[0].blocksize()))
   print("'2D' output workspace has {0} diffractograms having {1} bins each".format(red_ws[1].getNumberHistograms(), red_ws[1].blocksize()))
   print("'1D' output workspace has {0} diffractograms having {1} bins each".format(red_ws[2].getNumberHistograms(), red_ws[2].blocksize()))

Output:

.. testoutput:: ExPowderILLDetectorScan

    '2DTubes' output workspace has 128 diffractograms having 3024 bins each
    '2D' output workspace has 128 diffractograms having 3024 bins each
    '1D' output workspace has 1 diffractograms having 3024 bins each

.. testcleanup:: ExPowderILLDetectorScan

   mtd.remove('red_ws')

.. categories::

.. sourcelink::
