.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm calculates asymmetry for a series of Muon workspaces. The
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
| 3                 | 4          | Sum of Red and Green               |
+-------------------+------------+------------------------------------+

If ForwardSpectra and BackwardSpectra are set the Muon workspaces will
be grouped according to the user input, otherwise the Autogroup option
of LoadMuonNexus will be used for grouping.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Calculating assymetry for a series of MUSR runs:**

.. testcode:: ExSimple

   ws = PlotAsymmetryByLogValue(FirstRun = 'MUSR00015189.nxs',
                                LastRun = 'MUSR00015191.nxs',
                                LogValue = 'sample_magn_field',
                                TimeMin = 0.55,
                                TimeMax = 12.0);

   print "Y values (asymmetry):", ws.readY(0)
   print "X values (sample magn. field):", ws.readX(0)

Output:

.. testoutput:: ExSimple

   Y values (asymmetry): [ 0.14500665  0.136374    0.11987909]
   X values (sample magn. field): [ 1350.  1360.  1370.]

**Example - Using both Red and Green period:**

.. testcode:: ExRedGreen

   ws = PlotAsymmetryByLogValue(FirstRun = 'MUSR00015189.nxs',
                                LastRun = 'MUSR00015191.nxs',
                                LogValue = 'sample_magn_field',
                                TimeMin = 0.55,
                                TimeMax = 12.0,
                                Red = 1,
                                Green = 2);

   print "Y values (difference):", ws.readY(0)
   print "Y values (red):", ws.readY(1)
   print "Y values (green):", ws.readY(2)
   print "Y values (sum):", ws.readY(3)
   print "X values (sample magn. field):", ws.readX(0)

Output:

.. testoutput:: ExRedGreen

   Y values (difference): [-0.01593431 -0.02579926 -0.04337762]
   Y values (red): [ 0.14500665  0.136374    0.11987909]
   Y values (green): [ 0.16056898  0.16160068  0.16239291]
   Y values (sum): [ 0.30557563  0.29797468  0.282272  ]
   X values (sample magn. field): [ 1350.  1360.  1370.]

**Example - Using custom grouping to ignore a few detectors:**

.. testcode:: ExCustomGrouping

   # Skip spectra 35
   fwd_spectra = range(33,35) + range(36,65)

   # Skip spectra 1 and 2
   bwd_spectra = range(3, 33)

   ws = PlotAsymmetryByLogValue(FirstRun = 'MUSR00015189.nxs',
                                LastRun = 'MUSR00015191.nxs',
                                LogValue = 'sample_magn_field',
                                TimeMin = 0.55,
                                TimeMax = 12.0,
                                ForwardSpectra = fwd_spectra,
                                BackwardSpectra = bwd_spectra)

   print "No of forward spectra used:", len(fwd_spectra)
   print "No of backward spectra used:", len(bwd_spectra)
   print "Y values (asymmetry):", ws.readY(0)
   print "X values (sample magn. field):", ws.readX(0)

Output:

.. testoutput:: ExCustomGrouping

   No of forward spectra used: 31
   No of backward spectra used: 30
   Y values (asymmetry): [ 0.1628339   0.15440602  0.13743397]
   X values (sample magn. field): [ 1350.  1360.  1370.]

**Example - Applying dead time correction stored in the run files:**

.. testcode:: ExDeadTimeCorrection

   ws = PlotAsymmetryByLogValue(FirstRun = 'MUSR00015189.nxs',
                                LastRun = 'MUSR00015191.nxs',
                                LogValue = 'sample_magn_field',
                                TimeMin = 0.55,
                                TimeMax = 12.0,
                                DeadTimeCorrType = 'FromRunData');

   print "Y values (asymmetry):", ws.readY(0)
   print "X values (sample magn. field):", ws.readX(0)

Output:

.. testoutput:: ExDeadTimeCorrection

   Y values (asymmetry): [ 0.14542059  0.13674275  0.12017568]
   X values (sample magn. field): [ 1350.  1360.  1370.]

**Example - Calculating asymmetry as a function of the sample mean temperature:**

.. testcode:: ExLogValueFunction

   ws = PlotAsymmetryByLogValue(FirstRun="MUSR00015189",
                                LastRun="MUSR00015191",
                                LogValue="sample_temp",
                                Function="Mean")
   print "Y values (asymmetry):", ws.readY(0)
   print "X values (sample magn. field):", ws.readX(0)

Output:

.. testoutput:: ExLogValueFunction

   Y values (asymmetry): [ 0.15004357  0.14289412  0.12837688]
   X values (sample magn. field): [ 290.  290.  290.]

.. categories::
