.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm calculates asymmetry for a series of Muon workspaces. The
input workspaces must be in Muon Nexus files, the names of which follow the
rule: the filename must begin with at least 1 letter and followed by a
number. The input property FirstRun must be set to the file name with
the smallest number and the LastRun to the one with the highest number.
Alternatively the input property WorkspaceNames can be set as a list of runs.

The output workspace contains asymmetry as the Y values, and the selected
log values for X. The log values can be chosen as the mean, minimum, maximum,
first or last if they are a time series. For start/end times, the values are
in seconds relative to the start time of the first run.

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
| 3                 | 4          | Sum of Red and Green               |
+-------------------+------------+------------------------------------+

If ForwardSpectra and BackwardSpectra are set the Muon workspaces will
be grouped according to the user input, otherwise the Autogroup option
of LoadMuonNexus will be used for grouping.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Calculating asymmetry for a series of MUSR runs using First and Last:**

.. testcode:: ExSimple

   ws = PlotAsymmetryByLogValue(FirstRun = 'MUSR00015189.nxs',
                                LastRun = 'MUSR00015191.nxs',
                                LogValue = 'sample_magn_field',
                                TimeMin = 0.55,
                                TimeMax = 12.0);

   print("Y values (asymmetry): {}".format(ws.readY(0)))
   print("X values (sample magn. field): {}".format(ws.readX(0)))

Output:

.. testoutput:: ExSimple

   Y values (asymmetry): [0.14500665 0.136374   0.11987909]
   X values (sample magn. field): [1350. 1360. 1370.]

**Example - Calculating asymmetry for a series of MUSR runs using a range:**

.. testcode:: ExSimpleRange

   ws_list = ['MUSR00015195','MUSR00015189','MUSR00015192']

   ws = PlotAsymmetryByLogValue(WorkspaceNames = ws_list,
                                LogValue = 'sample_magn_field',
                                TimeMin = 0.55,
                                TimeMax = 12.0);

   print("Y values (asymmetry): [ {:.7f}  {:.7f}  {:.7f} ]".format(
       ws.readY(0)[0],ws.readY(0)[1],ws.readY(0)[2]))
   print("X values (sample magn. field): [ {:.1f}  {:.1f}  {:.1f} ]".format(
       ws.readX(0)[0],ws.readX(0)[1],ws.readX(0)[2]))

Output:

.. testoutput:: ExSimpleRange
  :options: +NORMALIZE_WHITESPACE

   Y values (asymmetry): [ 0.1450066  0.0929052  0.0652143 ]
   X values (sample magn. field): [ 1350.0  1380.0  1410.0 ]

**Example - Using both Red and Green periods:**

.. testcode:: ExRedGreen

   ws = PlotAsymmetryByLogValue(FirstRun = 'MUSR00015189.nxs',
                                LastRun = 'MUSR00015191.nxs',
                                LogValue = 'sample_magn_field',
                                TimeMin = 0.55,
                                TimeMax = 12.0,
                                Red = 1,
                                Green = 2);

   print("Y values (difference): {}".format(ws.readY(0)))
   print("Y values (red): {}".format(ws.readY(1)))
   print("Y values (green): {}".format(ws.readY(2)))
   print("Y values (sum): {}".format(ws.readY(3)))
   print("X values (sample magn. field): {}".format(ws.readX(0)))

Output:

.. testoutput:: ExRedGreen

   Y values (difference): [-0.01593431 -0.02579926 -0.04337762]
   Y values (red): [0.14500665 0.136374   0.11987909]
   Y values (green): [0.16056898 0.16160068 0.16239291]
   Y values (sum): [0.30557563 0.29797468 0.282272  ]
   X values (sample magn. field): [1350. 1360. 1370.]

**Example - Using custom grouping to ignore a few detectors:**

.. testcode:: ExCustomGrouping

   # Skip spectra 35
   fwd_spectra = [x for x in range(33, 65) if x != 35]

   # Skip spectra 1 and 2
   bwd_spectra = range(3, 33)

   ws = PlotAsymmetryByLogValue(FirstRun = 'MUSR00015189.nxs',
                                LastRun = 'MUSR00015191.nxs',
                                LogValue = 'sample_magn_field',
                                TimeMin = 0.55,
                                TimeMax = 12.0,
                                ForwardSpectra = fwd_spectra,
                                BackwardSpectra = bwd_spectra)

   print("No of forward spectra used: {}".format(len(fwd_spectra)))
   print("No of backward spectra used: {}".format(len(bwd_spectra)))
   print("Y values (asymmetry): {}".format(ws.readY(0)))
   print("X values (sample magn. field): {}".format(ws.readX(0)))

Output:

.. testoutput:: ExCustomGrouping

   No of forward spectra used: 31
   No of backward spectra used: 30
   Y values (asymmetry): [0.1628339  0.15440602 0.13743397]
   X values (sample magn. field): [1350. 1360. 1370.]

**Example - Applying dead time correction stored in the run files:**

.. testcode:: ExDeadTimeCorrection

   ws = PlotAsymmetryByLogValue(FirstRun = 'MUSR00015189.nxs',
                                LastRun = 'MUSR00015191.nxs',
                                LogValue = 'sample_magn_field',
                                TimeMin = 0.55,
                                TimeMax = 12.0,
                                DeadTimeCorrType = 'FromRunData');

   print("Y values (asymmetry): {}".format(ws.readY(0)))
   print("X values (sample magn. field): {}".format(ws.readX(0)))

Output:

.. testoutput:: ExDeadTimeCorrection

   Y values (asymmetry): [0.1458422  0.1371184  0.12047788]
   X values (sample magn. field): [1350. 1360. 1370.]

**Example - Calculating asymmetry as a function of the sample mean temperature:**

.. testcode:: ExLogValueFunction

   ws = PlotAsymmetryByLogValue(FirstRun="MUSR00015189",
                                LastRun="MUSR00015191",
                                LogValue="sample_temp",
                                Function="Mean")
   print("Y values (asymmetry): {}".format(ws.readY(0)))
   print("X values (sample magn. field): {}".format(ws.readX(0)))

Output:

.. testoutput:: ExLogValueFunction

   Y values (asymmetry): [0.15004357 0.14289412 0.12837688]
   X values (sample magn. field): [290. 290. 290.]

.. categories::

.. sourcelink::
