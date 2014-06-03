.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm calculates asymmetry for a series of muon workspaces. The
input workspaces must be in Muon Nexus files which names follow the
rule: the filename must begin with at least 1 letter and followed by a
number. The input property FirstRun must be set to the file name with
the smalest number and the LastRun to the one with the highest number.
If the "Green" property is not set the output workspace will contain a
single spectrum with asymmetry values. If the "Green" is set the output
workspace will contain four spectra with asymmetries:

+-------------------+------------+------------------------------------+
| Workspace Index   | Spectrum   | Asymmetry                          |
+===================+============+====================================+
| 0                 | 1          | Difference of Red and Green        |
+-------------------+------------+------------------------------------+
| 1                 | 2          | Red only                           |
+-------------------+------------+------------------------------------+
| 2                 | 3          | Green only                         |
+-------------------+------------+------------------------------------+
| 3                 | 4          | Sum of red and green asymmetries   |
+-------------------+------------+------------------------------------+

If ForwardSpectra and BackwardSpectra are set the muon workspaces will
be grouped according to the user input, otherwise the Autogroup option
of LoadMuonNexus will be used for grouping.

There is a python script PlotAsymmetryByLogValue.py which if called in
MantidPlot runs the algorithm and plots the results.

.. categories::
