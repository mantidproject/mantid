=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

Algorithms
----------

New features
############
- Add support for 1D MDHisto workspace to SaveASCII
- Add the ability for :ref:`algm-CompressEvents` to combine events together in a logarithmic increasing size groups.
- Add ability to specify input workspace order in `PolarizationCorrectionWildes` algorithm.
- Abins/Abins2D can now import phonopy .yaml data where the force
  constants are stored in a file named ``FORCE_CONSTANTS`` or
  ``force_constants.hdf5`` in the same directory as the YAML file.
  This is recommended when using large force constants arrays as the
  YAML loader can be very slow.
- Abins and Abins2D algorithms now support frozen atoms in data from GAUSSIAN

  - Similarly to the frozen-atoms ("selective dynamics") support for
    VASP outputs, frozen atoms will be disregarded in the calculation
    of inelastic scattering structure factor.
- Abins and Abins2D now support JSON file import. Supported formats are:

  - AbinsData (dump of internal object, intended for development and testing)
  - euphonic.QpointPhononModes (an equivalent set of data dumped from
    the Euphonic library)
  - euphonic.ForceConstants (force constants which may be manipulated
    in Euphonic, and will be converted to phonon modes on a q-point
    mesh when Abins(2D) is run)

  The Euphonic JSON formats are convenient to create with Python
  scripts, and recommended for users who wish to somehow customise or
  manipulate their data before using it with Abins(2D).
- New algorithm :ref:`LoadErrorEventsNexus <algm-LoadErrorEventsNexus>` to load events from the `bank_error_events` bank of a NeXus file
- Re-implemtation of :ref:`LoadEventNexus <algm-LoadEventNexus>` when specifying the ``CompressTolerance``. This uses significantly less memory to create fewer events overall. However, the execution time of ``LoadEventNexus`` itself is generally longer; workflows that benefit from ``CompressEvents`` generally run faster.
- Added a new peak finding strategy named AllPeaksNSigma to FindSXPeaks :ref:`FindSXPeaks <algm-FindSXPeaks-v1>` algorithm. Credits to the author of SXD2001 for the idea of using NSigma as a threshold (albeit in SXD2001 the peak finding is done in 3D).
  Gutmann, M. J. (2005). SXD2001. ISIS Facility, Rutherford Appleton Laboratory, Oxfordshire, England.
- Validation rules added to FindSXPeaks :ref:`FindSXPeaks <algm-FindSXPeaks-v1>` algorithm to remove spurious peaks due to noise by allowing user to provide additional arguements as below
- `MinNBinsPerPeak`, the Minimum number of bins contributing to a peak in an individual spectrum
- `MinNSpectraPerPeak`, `MaxNSpectraPerPeak` Minimum & Maximum number of spectra contributing to a peak after they are grouped

Bugfixes
############
- A performance improvement when using the :ref:`LoadNexusProcessed <algm-LoadNexusProcessed>` algorithm to load a NeXus file has been achieved.
- Algorithm DSFinterp, which was deprecated, has been removed
- Remove unwanted interaction between Abins and Abins2D

  - Abins algorithm sets the value
    ``abins.parameters.sampling["bin_width"]`` while running. This
    would override the default sampling of Abins2D instruments if set.

  - This would not cause results to be incorrect, but would sample
    them on a different mesh to the expected one and could limit
    resolution.

  - The value is now saved and restored after use by Abins; it can
    still be modified by users who wish to fiddle with the Abins2D
    behaviour.
- A performance improvement when using the :ref:`Load <algm-Load>` algorithm to load a single file has been achieved.
- Fix doctest strings for :ref:`GenerateGroupingPowder <algm-GenerateGroupingPowder>`
- Fixed a crash in :ref:`FindPeaks <algm-FindPeaks>` when the number of bins in the workspace are not sufficient to run SmoothData:ref:`algm-SmoothData` algorithm and raise the error instead.

Fit Functions
-------------

New features
############


Bugfixes
############
- Updated search box for fitting functions so that function suggestions do not show repeated functions
- Fit Function DSFinterp1DFit, which deprecated, has been removed
- CompositeFunction will now throw an exception if getNumberDomains() is called and there is an inconsistent number of domains in any of the member functions.


Data Handling
-------------

New features
############
- Introduced file search/loading from instrument data cache on IDAaaS.
  The instrument data cache is the directory ``/data/instrument/`` present on IDAaaS, and contains a local copy of the data archive.
  The instrument data cache is used to search for files before looking in the data archive.
  This new feature fixes the problem of loading files for users who do not have access to the data archive on IDAaaS.
  Please note that if you are not on IDAaaS, avoid creating the directory ``/data/instrument/`` as this will trigger a search for files inside that directory.
- Added new boolean parameter ``LoadNexusInstrumentXML`` to :ref:`LoadEventAsWorkspace2D <algm-LoadEventAsWorkspace2D>`. Default is *true*.

Bugfixes
############
- The properties ``LoaderName`` and ``LoaderVersion`` of :ref:`Load <algm-Load>` are now guaranteed to be set by end of algorithm.
- The properties ``GroupingWorkspace`` and ``GroupingFilename`` are now optional in :ref:`GenerateGroupingPowder <algm-GenerateGroupingPowder>`. At least one must be specified.


Data Objects
------------

New features
############
- Speedup processing of IDF XML during loading when side-by-side-view-location parameter is not used

Bugfixes
############



Python
------

New features
############
- The :ref:`Peak Shapes <the-peak-shape>` (NoShape, PeakShapeSpherical, PeakShapeEllipsoid) and :meth:`mantid.api.IPeak.setPeakShape` have been exposed to Python allowing you to manually create and set the peak shapes.

Bugfixes
############
- Input validation errors have been corrected for numerical line edits of the Filter Events interface.
- Fix crash occurring when an incorrect `TOF Correction To Sample` value is selected for the input workspace. Now error message is displayed instead of crash.
- Fix bug that allowed the two sliders to cross each other for certain values of the line edits controlling the sliders.


Dependencies
------------------

New features
############
- Drop support for end-of-life numpy 1.22 and 1.23, and extend support to 1.25 and 1.26.

Bugfixes
############



MantidWorkbench
---------------

See :doc:`mantidworkbench`.
:ref:`Release 6.10.0 <v6.10.0>`
