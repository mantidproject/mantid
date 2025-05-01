.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

|Example of RAW GEM data focused across the 5 detector banks| Given an
InputWorkspace and a Grouping filename, the algorithm performs the
following:

#. The calibration file is read and a map of corresponding udet-group is
   created.
#. The algorithm determines the X boundaries for each group as the upper
   and lower limits of all contributing detectors. It then calculates
   a logarithmic step that preserves the number of bins in the initial workspace.
   It assumes that the entire data set uses logarithmic binning in the process
   (i.e. it does not check for constant width binning).
#. All histograms are read and rebinned to the new grid for their group.
#. A new workspace with N histograms is created.

Within the :ref:`CalFile <CalFile>` any detectors with the 'select' flag
can be set to zero or with a group number of 0 or -ve groups are not
included in the analysis.

Since the new X boundaries depend on the group and not the entire
workspace, this focusing algorithm does not create overestimated X
ranges for multi-group instruments. However it is important to remember
that this means that this algorithm outputs a :ref:`ragged workspace <Ragged_Workspace>`.
Some 2D and 3D plots will not display the data correctly.

The DiffractionFocussing algorithm uses GroupDetectors algorithm to
combine data from several spectra according to GroupingFileName file
which is a :ref:`CalFile <CalFile>`.

For EventWorkspaces
###################

The algorithm can be used with an :ref:`EventWorkspace <EventWorkspace>`
input, and will create an EventWorkspace output if a different workspace
is specified.

The main difference vs. using a Workspace2D is that the event lists from
all the incoming pixels are simply appended in the grouped spectra; this
means that you can rebin the resulting spectra to finer bins with no
loss of data. In fact, it is unnecessary to bin your incoming data at
all; binning can be performed as the very last step.

Rebin parameters
################

By default ``DiffractionFocussing`` will use the min and max of each
spectra and the number of bins in the input workspace to automatically
determine the output binning. An alternative is to provide the min,
max and delta for each output spectra similar to
:ref:`algm-RebinRagged`. The minimum and maximum values that are
specified are interpreted as one value per spectrum. If there is only
one value overall, it is used for all of the spectra. The ``Delta``
parameter is required and can either be a single number which is
common to all, or one number per spectra. Positive values are
interpreted as constant step-size. Negative are logarithmic.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Diffraction focussing of HRPD data:**

.. testcode:: ExHRPDFocussing

   # Load HRP dataset
   ws = Load("HRP39180.RAW")

   # specify grouping file, here using CalFile format
   cal_file = "hrpd_new_072_01_corr.cal"

   # For HRPD data, perform a unit conversion TOF->d-spacing, taking into account detector position offsets
   ws = ApplyDiffCal(InstrumentWorkspace='ws', CalibrationFile=cal_file)
   ws = ConvertUnits(InputWorkspace='ws', Target="dSpacing")
   # Focus the data
   ws = DiffractionFocussing(InputWorkspace='ws',GroupingFileName=cal_file)

   print("The 51st y-value is: %.3f" % ws.readY(0)[50])

Output:

.. testoutput:: ExHRPDFocussing

   The 51st y-value is: 900.148

**Example - Demonstrating option PreserveEvents:**

.. testcode:: ExEventFocussing

   # create event workspace with one bank of 4 detectors
   ws = CreateSampleWorkspace(WorkspaceType="Event", XUnit="dSpacing", NumBanks=1, BankPixelWidth=2)

   # focus data in 4 spectra to 2 spectra according to .cal file and don't preserve events
   ws = DiffractionFocussing(InputWorkspace='ws', GroupingFileName="4detector_cal_example_file.cal" \
        , PreserveEvents=False)

   print("Number of focussed spectra: " + str(ws.getNumberHistograms()))
   print("What type is the workspace after focussing: " + ws.id())

Output:

.. testoutput:: ExEventFocussing

   Number of focussed spectra: 2
   What type is the workspace after focussing: Workspace2D

**Example - Defining binning parameters:**

.. testcode:: ExHRPDFocussing2

   # Load HRP dataset
   ws = Load("HRP39180.RAW")

   # specify grouping file, here using CalFile format
   cal_file = "hrpd_new_072_01_corr.cal"

   # For HRPD data, perform a unit conversion TOF->d-spacing, taking into account detector position offsets
   ApplyDiffCal(InstrumentWorkspace=ws, CalibrationFile=cal_file)
   ws = ConvertUnits(InputWorkspace=ws, Target="dSpacing")
   ApplyDiffCal(InstrumentWorkspace=ws, ClearCalibration=True)  # use uncalibrated DIFC to convert from d-spacing to TOF
   # Focus the data with defined binning parameters
   ws = DiffractionFocussing(InputWorkspace=ws,GroupingFileName=cal_file, DMin=[0.6,1.0,2.2], DMax=[1.0,1.5,4.0], Delta=0.1)

   print(f"Output has {ws.getNumberHistograms()} spectra with number of bins {len(ws.readY(0))}, {len(ws.readY(1))} and {len(ws.readY(2))}")

Output:

.. testoutput:: ExHRPDFocussing2

   Output has 3 spectra with number of bins 4, 5 and 18

Previous Versions
-----------------

Version 1
#########

Version 1 did not support the use of workspaces to carry grouping
workspaces and only worked with CalFiles.

.. |Example of RAW GEM data focused across the 5 detector banks| image:: /images/GEM_Focused.png

.. categories::

.. sourcelink::
