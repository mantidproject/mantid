.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm outputs the data in ASCII as a 3 column X, Y ,E format
for use in subsequent analysis by other programs. The output files can
be read for example into FullProf with format instrument=10.

For data where the focusing routine has generated several spectra (for
example, multi-bank instruments), the option is provided for saving all
spectra into a single file, separated by headers, or into several files
that will be named "workspaceName-"+spectra\_number

Current Issues
--------------

Fullprof expects the data to be in TOF, however at present the
:ref:`algm-DiffractionFocussing` algorithm in Mantid
leaves the data in d-spacing.

If the written file is to be loaded into TOPAS, then headers should be
omitted (set the IncludeHeader property to false);

.. categories::
