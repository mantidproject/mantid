.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm calculates the detector efficiency corrections for the instrument D20 at the ILL.

It performs as follows:

1. Takes the first cell response as a function of scattering angle as a reference

2. Takes the second cell, and divides the reference to the second cell response in the overlapping region in terms of scattering angle. This results in the array of relative response ratios.

3. Uses one of the 3 suggested options to compute the relative calibration factor from the array of relative response ratios. This factor is saved in the output as the calibration factor for the second cell.

4. The response of the second cell is scaled up with that factor, and then merged (in the overlapping region) with the reference using :ref:`WeightedMean <algm-WeightedMean>`.

5. Repeat from Step 2 for the next cell and so on until the last cell.

For the zero-counting cells, the calibration factor cannot be computed, and it will be set to 1. Cells are treated as zero-counting, if they count zero more than 80% of time.

After the calibration factors are computed for all the cells, they are divided by the median of all the factors (excluding the zero counting cells),
in order to absolutely normalise the calibration curve.

Input
-----

The input must be a single **detector-scan** run in `.nxs` format produced for vanadium.

Optionally the previously derived calibration file can be seeded, and the algorithm will then compute the residual calibration factors on top of that.

Method
------

You can choose the method of how the relative calibration factor is extracted from the relative response ratios (Step 3).
It can be the Median (default), Mean or :ref:`MostLikelyMean <algm-MostLikelyMean>`.

Excluded range
--------------

Provide ranges in scattering angle in degrees, to exclude non-desired regions, e.g. the beam stop.
Multiple regions can be set, **-20,0,10,20** will exclude **[-20,0]** and **[10,20]**.
This exclusion will happen at Step 3.

Pixel range
-----------

Provide the range of detector cells to compute the calibration factors for.
For the rest of the cells, the factor will be set to 1.

Output
------

Output will be a single-column workspace containing the calibration factors for each cell. This should be normally saved with
:ref:`SaveNexusProcessed <algm-SaveNexusProcessed>` to be later used in :ref:`PowderDiffILLReduction <algm-PowderDiffILLReduction>`.

Optionally, the full absolute response resulted from the combination of the data for all the cells (see Step 4 above) can also be output.

Workflow
--------

.. diagram:: PowderDiffILLDetEffCorr-v1_wkflw.dot

Related Algorithms
------------------

:ref:`PowderDiffILLReduction <algm-PowderDiffILLReduction>` performs the data reduction.

Usage
-----

**Example - PowderDiffILLDetEffCorr**

.. code-block:: python

   calib = PowderDiffILLDetEffCorr(CalibrationRun='967076', OutputWorkspace='constants')
   print("Reduced workspace contains {0} constants, one for each cell.".format(calib.getNumberHistograms()))

Output:

.. code-block:: python

   Reduced workspace contains 3072 constants, one for each cell.

.. categories::

.. sourcelink::
