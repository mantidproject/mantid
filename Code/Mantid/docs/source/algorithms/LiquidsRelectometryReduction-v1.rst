.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The workflow proceeds as follows:

1. Load the data to be reduced.

2. Crop to the specified TOF range.

3. Subtract the background and integrate over the low-resolution axis.

4. Normalize by the integrated current.

5. Crop to the reflectivity peak using the specified range.

6. Repeat steps 1 to 6 for the direct beam normalization run.

7. Sum up the pixels contained in the peak of the normalization run to
   obtain a TOF distribution.

8. Divide the TOF distribution of each signal pixel by the normalization distribution.

9. Apply the scaling factor.

10. Sum up the pixels within the reflectivity peak of the data.

11. Convert to Q.

12. Rebin the Q axis to the specified binning and crop out the first and last Q point.

.. categories::
