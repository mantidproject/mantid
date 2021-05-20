.. _06_exercise_3:

==========
Exercise 3
==========

The aim of this exercise is to write a small algorithm that wraps a small
script that focuses some powder diffraction data.

Write an algorithm called ``PowderDiffractionReduce``. The algorithm should
have 3 properties:

* ``Filename``: A FileProperty for a TOF data file to load (ignore extensions)
* ``CalFilename``: A FileProperty for a cal file (ignore extensions)
* ``OutputWorkspace``: An output WorkspaceProperty to hold the final result.

The steps the algorithm should perform are:

#. Use the :ref:`Load <algm-Load-v1>` algorithm to load the TOF
   data
#. Apply the calibration file using
   :ref:`ApplyDiffCal <algm-ApplyDiffCal-v1>`
#. Run :ref:`ConvertUnits <algm-ConvertUnits-v1>` on the TOF data
   to convert to ``dSpacing``
#. Run :ref:`DiffractionFocussing <algm-DiffractionFocussing-v2>`
   on the previous output & focus the data using
   the same cal file from the earlier step (called a grouping file here)
#. Set the output from the ``DiffractionFocussing`` algorithm as the output of
   ``PowderDiffractionReduce``
#. Delete the temporary reference using
   :ref:`DeleteWorkspace <algm-DeleteWorkspace-v1>`

To test the algorithm, execute the script that contains the algorithm to
register it with Mantid. It will then show up in the list of algorithms.
Use the following inputs:

* ``Filename``: *HRP39182.RAW*
* ``CalFilename``: *hrpd_new_072_01_corr.cal*
* ``OutputWorkspace``: *focussed*

When plotting the output workspace it should look like this:

.. figure:: /images/Training/ExtendingMantidWithPython/exercise_3_solution_plot.png
   :alt: Plot with 3 spectra
   :align: center
   :width: 750
   :height: 500

Once finished check your answer with the provided :ref:`03_emwp_sol`
