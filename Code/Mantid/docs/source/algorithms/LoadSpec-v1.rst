.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The LoadSpec algorithm reads in spectra data from a text file and stores
it in a :ref:`Workspace2D <Workspace2D>` as data points. 
The data in the file must be organized by set of 3 columns (separated by any number of spaces). The
first column has to be the X values, the second column the Y values and
the third column the error values.

Here are two examples of such text files that can be loaded with
LoadSpec:

*Example 1:*

::

    #F norm: REF_M_2000.nxs
    #F data: REF_M_2001.nxs
    #E 1234567.80
    ...
    #C SCL Version - 1.4.1

    #S 1 Spectrum ID ('bank1',(0,127))
    #L Lambda_T(Angstroms) Intensity(Counts/A) Sigma(Counts/A)
    0.0   0.0   0.0
    1.0   5.0   2.0
    2.0   10.0  3.0
    3.0   15.0  2.0
    4.0   20.0  2.5
    5.0   25.0  3.2
    6.0   30.0  4.2

This will create a :ref:`Workspace2D <Workspace2D>` with 1 spectrum.

*Example 2:*

::

    #F norm: REF_M_2000.nxs
    #F data: REF_M_2001.nxs
    #E 1234567.80
    ...
    #C SCL Version - 1.4.1

    #S 1 Spectrum ID ('bank1',(0,127))
    #L Lambda_T(Angstroms) Intensity(Counts/A) Sigma(Counts/A)
    0.0   0.0   0.0
    1.0   5.0   2.0
    2.0   10.0  3.0
    3.0   15.0  4.0

    #S 1 Spectrum ID ('bank1',(1,127))
    #L Lambda_T(Angstroms) Intensity(Counts/A) Sigma(Counts/A)
    0.0   10.0   0.0
    1.0   15.0   2.0
    2.0   110.0  3.0
    3.0   115.0  4.0

    #S 1 Spectrum ID ('bank1',(3,127))
    #L Lambda_T(Angstroms) Intensity(Counts/A) Sigma(Counts/A)
    0.0   20.0   0.0
    1.0   25.0   2.0
    2.0   210.0  3.0
    3.0   215.0  4.0

This text file will create a :ref:`Workspace2D <Workspace2D>` with 3 spectra.

.. categories::
