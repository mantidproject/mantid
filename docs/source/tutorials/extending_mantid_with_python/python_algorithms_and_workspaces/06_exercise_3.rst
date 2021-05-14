.. _06_exercise_3:

==========
Exercise 3
==========

The aim of this exercise is to write a small algorithm that wraps a small
script that focuses some powder diffraction data.

Write an algorithm called ``PowderDiffractionReduce``. The algorithm should
have 3 properties:

#. ``Filename``: A FileProperty for a TOF data file to load (ignore extensions)
#. ``CalFilename``: A FileProperty for a cal file (ignore extensions)
#. ``OutputWorkspace``: An output WorkspaceProperty to hold the final result.

The steps the algorithm should perform are:

#. Use the ``Load`` algorithm to load the TOF data
#. Run ``AlignDetectors`` on the TOF data using the file given by the
   ``CalFilename`` property
#. Run ``DiffractionFocussing`` on the previous output & focus the data using
   the same cal file as the previous step (called a grouping file here)
#. Set the output from the ``DiffractionFocussing`` algorithm as the output of
   ``PowderDiffractionReduce``
#. Delete the temporary reference using ``DeleteWorkspace``

To test the algorithm, execute the script that contains the algorithm to
register it with Mantid. It will then show up in the list of algorithms.
Use the following inputs:

* ``Filename``: *HRP39182.RAW*
* ``CalFilename``: *hrpd_new_072_01_corr.cal*
* ``OutputWorkspace``: *focussed*

Once finished check your answer with the provided :ref:`03_emwp_sol`
