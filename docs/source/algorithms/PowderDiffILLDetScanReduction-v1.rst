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
:py:obj:`MultipleFileProperty <mantid.api.MultipleFileProperty>`. For specifying multiple runs it is best to use ``:``
as the separator - e.g. ``508093:508095``. Using ``-`` is also possible, this will call
:ref:`MergeRuns <algm-MergeRuns>`, but there is a performance penalty for this.

Calibration
-----------

The NeXus files for D2B contain raw data and pre-calibrated data. Either of these can be used when loading.

Normalisation Options
---------------------

The default is for normalisation to monitor, but this can be skipped.

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

.. diagram:: PowderDiffILLDetScanReduction-v1_wkflw.dot

Related Algorithms
------------------

:ref:`PowderDiffILLReduction <algm-PowderDiffILLReduction>` can be used for D20 where the detector is static, and
another variable such as temperature is scanned instead.

Usage
-----

**Example - PowderDiffDetScanILLReduction**

.. testsetup:: ExPowderDiffDetScanILLReduction

   config['default.facility'] = 'ILL'
   config['default.instrument'] = 'D2B'
   config.appendDataSearchSubDir('ILL/D2B/')

.. testcode:: ExPowderDiffDetScanILLReduction

   red_ws = PowderDiffILLDetScanReduction(Run='508093:508095', Output2DTubes=True, Output2D=True, Output1D=True)
   print("'2DTubes' output workspace has {0} diffractograms having {1} bins each".format(red_ws[0].getNumberHistograms(), red_ws[0].blocksize()))
   print("'2D' output workspace has {0} diffractograms having {1} bins each".format(red_ws[1].getNumberHistograms(), red_ws[1].blocksize()))
   print("'1D' output workspace has {0} diffractograms having {1} bins each".format(red_ws[2].getNumberHistograms(), red_ws[2].blocksize()))

Output:

.. testoutput:: ExPowderDiffDetScanILLReduction

    '2DTubes' output workspace has 128 diffractograms having 3025 bins each
    '2D' output workspace has 128 diffractograms having 3025 bins each
    '1D' output workspace has 1 diffractograms having 3025 bins each

.. testcleanup:: ExPowderDiffDetScanILLReduction

   mtd.remove('red_ws')

.. categories::

.. sourcelink::
