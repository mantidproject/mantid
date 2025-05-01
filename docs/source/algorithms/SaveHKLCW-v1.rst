.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Input
#####

This algorithm will save a peaks workspace to a SHELX76 formatted file
which is made to be compatible with Fullprof and WINGX. This is a
simplified version compared to :ref:`algm-SaveHKL` and it assumes a
constant wavelength. It also assume integer HKL and will round to the
nearest integer value.

For output with direction cosines the format is.

+----+----+----+------+------------+----+------+------+------+------+------+------+
| h  | k  | l  | F^2  | sigma(F^2) | N  | IX   | DX   | IY   | DY   | IZ   | DZ   |
+====+====+====+======+============+====+======+======+======+======+======+======+
| I4 | I4 | I4 | F8.2 | F8.2       | I4 | F8.5 | F8.5 | F8.5 | F8.5 | F8.5 | F8.5 |
+----+----+----+------+------------+----+------+------+------+------+------+------+

The corresponding format without direction cosines is.

+----+----+----+------+------------+----+
| h  | k  | l  | F^2  | sigma(F^2) | N  |
+====+====+====+======+============+====+
| I4 | I4 | I4 | F8.2 | F8.2       | I4 |
+----+----+----+------+------------+----+

The output file contains a short header containing the default title,
format string and wavelength. See the documentation for :ref:`algm-SaveHKL`
for additional information about conventions in direction cosines.

Usage
-----

**Example: Output with direction cosines**

.. testcode:: SaveHKLCW

    # create a temporary workspace
    ws = CreateSampleWorkspace()
    # create the peaks workspace
    peaks = CreatePeaksWorkspace(ws, NumberOfPeaks=0)
    # set the UB, require for direction cosine calculation
    SetUB(peaks)
    # add some peaks
    peaks.addPeak(peaks.createPeakHKL([1, 1, 1]))
    peaks.addPeak(peaks.createPeakHKL([1, -1, 1]))

    # Save the output
    file_name = "Usage_Example.hkl"
    path = os.path.join(os.path.expanduser("~"), file_name)
    SaveHKLCW(peaks, path, DirectionCosines=True)

    # Does the file exist? If it exists, print it!
    if os.path.isfile(path):
        with open(path, 'r') as f:
             data = f.readlines()
        for entry in data:
            print(entry[:-1]) # leave out the last character to remove unnecessary newlines

.. testcleanup:: SaveHKLCW

    os.remove(path)

Output:
The resulting output file (Usage_Example.hkl) looks like this

.. testoutput:: SaveHKLCW

    Single crystal data
    (3i4,2f8.2,i4,6f8.5)
    0.66667  0   0
       1   1   1    0.00    0.00   1-1.00000 0.33333-0.00000-0.66667-0.00000-0.66667
       1  -1   1    0.00    0.00   1-1.00000 0.33333-0.00000 0.66667-0.00000-0.66667


.. categories::

.. sourcelink::
