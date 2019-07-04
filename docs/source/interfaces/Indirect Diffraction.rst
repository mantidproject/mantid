Indirect Diffraction
====================

.. contents:: Table of Contents
  :local:

Overview
--------

.. interface:: Diffraction
  :align: right
  :width: 350

This interface provides a means of reducing raw data into units of dSpacing, and
is focused on the instruments of the ISIS Molecular Spectroscopy Group. For this
reason, it bears the name "Indirect", to separate it from any future interface
that may be used by other diffraction-based instruments - although the
operations are actually elastic.

Action Buttons
--------------

Settings
  Opens the :ref:`Settings <interface-indirect-settings>` GUI which allows you to
  customize the settings for the Indirect interfaces.

?
  Opens this help page.

Manage Directories
  Opens the Manage Directories dialog allowing you to change your search directories
  and default save directory and enable/disable data archive search.

Options Common to all Instruments
---------------------------------

Instrument
  Used to select the instrument on which the data being reduced was created on.

Reflection
  Used to select the instrument configuration used during the experiment; either
  diffonly if the instrument was only being used for diffraction or diffspec if
  the instrument was being used for diffraction and spectroscopy.

.. tip:: If you need clarification as to the instrument setup you should use
  please speak to the instrument scientist who dealt with your experiment.

Run Numbers
  The raw run files that are to be used in the reduction, multiple files can be
  specified in the same manner as in the Energy Transfer tab on Indirect Data
  Reduction.

Use Container
  This option allows the subtraction of a run for an empty container run.

Scale Container
  This option allows for an optional scale factor to be applied to the container
  data before it is subtracted.

Spectra Min & Spectra Max
  Specify the range of spectra to use in the reduction, the default values of
  this are set based on the instrument and reflection selected. Note that these
  options will be disabled when using OSIRIS in diffonly mode, in which case the
  entire spectra range will be used.

Run
  Runs the processing configured on the current tab.

Plot Type
  Specify the type of plot to be created when the reduction is complete, either
  a contour or spectra plot can be created.

Save Formats
  Select a range of save formats to save the reduced data as, in all cases the
  data is saved in the default save directory. In the case of GSS the data is
  first converted to time of flight.

OSIRIS diffonly
---------------

OSIRIS is supported through the :ref:`OSIRISDiffractionReduction
<algm-OSIRISDiffractionReduction>` algorithm, and as such has a radically
different workflow to IRIS and TOSCA.

The available options are the same, except that a single calibration file and
one or more vanadium files must be specified instead of rebinning values. These
files are remembered by the interface so they only have to be set once per cycle.

There is also the option to manually set the dRange used in all of the sample
runs, note that in this case all sample files must correspond to the same dRange.
The dRanges are numbered as per the `OSIRIS manual
<https://www.isis.stfc.ac.uk/Pages/osiris-user-guide.pdf>`_.
Otherwise the dRange is determined based on the table provided in the manual.

Note: There must be a corresponding vanadium file with the same dRanges for each
of the data files entered, but leaving in a full complement of vanadium files -
even if they are not all used - should not be a problem. Further, mixing up the
order of files should not be problematic either.

Multiple data files with the same dRanges will be "averaged" together.

.. interface:: Diffraction
  :widget: pageCalibration

IRIS, OSIRIS diffspec, TOSCA & VESUVIO
--------------------------------------

All other instruments are supported through the
:ref:`ISISIndirectDiffractionReduction <algm-ISISIndirectDiffractionReduction>`
algorithm and share the same set of options:

.. interface:: Diffraction
  :widget: pageDSpaceRebin

Sum Files
  If selected the raw files will be summed after they are loaded and the
  reduction will treat them as a single run.

Rebin in D-Spacing
  Optionally provide parameters to rebin the data in dSpacing, if no parameters
  are provided then a rebin will not be done.

Use Individual Grouping
  If selected, each detector will be output on its own spectrum in the reduced
  file, this can be useful to verify detector positioning on instruments such
  as VESUVIO.

.. categories:: Interfaces Indirect
