.. _Texture_Planner-ref:

Texture Experiment Planner
==========================

.. contents:: Table of Contents
    :local:

Interface Overview
------------------

This custom interface integrates the tasks involved in planning and executing
texture experiments (see :ref:`TextureAnalysis` and :ref:`Texture_Reduction` for
more context). It is primarily intended to serve ENGINX and IMAT; however, it is
compatible with all Mantid instruments.

The general workflow of the interface is:

#. Define an experimental sample with the correct shape, material properties, and
   initial position on the beamline.

#. Define the sample directions (see :ref:`TextureAnalysis` for more details).

#. Set the instrument and virtual detector grouping (this can be updated
   throughout the process).

#. Optionally define a gauge volume (otherwise the whole sample is assumed to be
   illuminated).

#. Either load or manually add the experimental orientations.

#. Observe the pole figure coverage that your current experimental setup will
   yield.

#. Iterate on the orientations until the desired coverage is achieved.

#. Optionally, view and optimise the experiment based on the estimated
   transmission values for each run and each virtual detector.

#. Export the desired information. This can be the experimental orientations in a
   variety of formats, the definition of the sample on the beamline to be used as
   a reference, or the relative weightings of the estimated transmission values.

.. _Texture_Planner_general-info-ref:

General Information
^^^^^^^^^^^^^^^^^^^

Red Stars
    Red stars next to browse boxes and other fields indicate that the file could
    not be found. Hover over the star to see more information.

Settings
    The cog icon opens an additional settings menu, which exposes some of the
    interface behaviour that might otherwise clutter the main window. See
    :ref:`Settings Menu <Texture_Planner_settings-ref>` for the full set of options.

.. _Texture_Planner_sample-setup-ref:

Sample Setup Options
^^^^^^^^^^^^^^^^^^^^^

Load Sample Shape
    Options to load a shape which has been defined either as a shape mesh
    (``.stl`` file) or using Constructive Solid Geometry (CSG - ``.xml`` file). See
    :ref:`HowToDefineGeometricShape` for more information on how to format a CSG
    shape definition.

Set Sample Material
    Set the material of the sample using the :ref:`algm-SetSampleMaterial`
    algorithm.

Update Initial Shape Orientation
    Adjust the starting orientation of the shape relative to the components of the
    selected instrument. This should match how the sample will be (or is) loaded
    onto the beamline, before the sample positioner has repositioned it for a given
    orientation.

Update Initial Shape Position
    Adjust the starting position of the shape relative to the components of the
    selected instrument. This should match how the sample will be (or is) loaded
    onto the beamline, before the sample positioner has repositioned it for a given
    orientation.

Update Sample Directions
    Set the three intrinsic sample directions, relative to which all of the texture
    information is calculated. The pole figure projection is always taken into the
    plane of the first and the last directions.

    Note that these are defined in terms of the instrument reference frame, *not*
    the sample shape that has been loaded and potentially repositioned.

    ``Update Texture Directions`` must be clicked to apply the changes.

Experimental Setup
^^^^^^^^^^^^^^^^^^

Instrument
    Select from the fully supported instruments (ENGINX or IMAT) or try a different
    Mantid instrument (``Custom``). If ``Custom`` is selected, ``Group`` is fixed to
    ``Custom`` as well and the relevant dialog boxes will appear.

Group
    Select the desired virtual detector grouping. Some pre-set options are available
    for both ENGINX and IMAT, but custom XML definitions can be loaded in addition.

Custom Instrument
    Type the name of the Mantid instrument you would like to use for the experiment.
    The interface checks the input text against the instrument definitions it can
    find. If the text does not match a known instrument the box will appear red and
    ``Update Instrument`` will remain disabled.

Grouping File
    Select a detector grouping file (see :ref:`algm-SaveDetectorsGrouping-v1` and
    :ref:`algm-LoadDetectorsGroupingFile-v1`) to be used. If the file is
    incompatible with the selected instrument, ``Update Instrument`` will remain
    disabled.

Update Instrument
    Update the experiment with the current set of instrument options.

Gauge Volume Options
    Set an experimental gauge volume either from the pre-set definitions or from a
    custom CSG definition (see :ref:`HowToDefineGeometricShape`).

Set Gauge Volume
    Apply the current gauge volume definition to the experiment.

Clear Gauge Volume
    Remove the gauge volume definition - a shortcut for setting ``No Gauge Volume``.

Load Orientation File
    Load a set of orientations from a ``.txt`` file of either Euler angles or
    flattened rotation matrices (see the
    :ref:`orientation file section <OrientationSection>` of the
    :ref:`Texture Analysis Concept doc <TextureAnalysis>`). The file type is
    automatically determined from the number of entries in each row.

    The axes and senses of the Euler angles can be set in the settings menu, and
    these axes also define how rotation matrices will be displayed as decomposed
    axes.

    Once loaded, these orientations populate the Orientation Table.

Goniometers
^^^^^^^^^^^

Num axes
    Number of active goniometer axes for the sample positioner.

Step size
    Number of degrees that a single arrow click for the ``Angle`` fields will
    increment or decrement.

Current Orientation Index
    The row of the Orientation Table currently being shown in the Axes below and
    highlighted in the Pole Figure Display (when not in the transmission estimation
    view).

Add Orientation
    Add a duplicate of the current orientation to the end of the Orientation Table.

Axes
    Display a ``Vector``, ``Sense`` and ``Angle`` for up to 6 dependent goniometer
    axes. Axis 0 is the outermost, and each subsequent rotation is intrinsically
    linked to the rotations of the outer axes. The goniometers can be visualised by
    enabling ``Show Goniometers`` in the settings.

Lab View
^^^^^^^^

Figure
    Displays the sample oriented on the chosen instrument. From the settings it is
    possible to toggle the visibility of:

    - The sample direction vectors
    - The goniometers
    - The incident neutron beam
    - The diffraction vectors (Ks)
    - The scattered detector vectors

Pole Figure Display
^^^^^^^^^^^^^^^^^^^

Figure
    Shows the coverage for the current set of orientations with ``Include`` selected
    in the Orientation Table.

    The ``Current Orientation Index`` from the ``Goniometers`` panel is displayed as
    solid blue points (if included, otherwise as grey point outlines), and the other
    orientations as blue point outlines.

    The goniometer axes are displayed as coloured points which match their colour in
    the Lab View display (the goniometers can be visualised by enabling
    ``Show Goniometers`` in the settings). Goniometer axes which lie within the plane
    of the projection are displayed as lines. The most recently updated goniometer is
    displayed as a solid point or line, with the others as outlines or dashed lines.

Estimate Transmission Values
    When enabled, the figure instead displays the estimated transmission values for
    each virtual detector, based on a Monte Carlo simulation at a specified
    TOF/d-spacing/wavelength (these can be modified in the settings menu).

Orientation Table
^^^^^^^^^^^^^^^^^

Orientations
    Contains the axis information for all of the experimental orientations, as well
    as which ones should be included in the Pole Figure Display and any outputs (the
    ``Include`` toggle). Additionally there is a ``Select`` toggle which allows
    several orientations to be removed at once.

Select All / Deselect All
    Tick or clear the ``Select`` box on every orientation in the table.

Delete Selected
    Remove all orientations whose ``Select`` box is ticked.

.. _Texture_Planner_settings-ref:

Settings Menu
^^^^^^^^^^^^^

Opened from the cog icon, the settings menu gathers options that fine-tune how
shapes are loaded, how orientations are interpreted, and how the transmission
estimate is calculated and displayed. It also provides the visibility toggles for
the Lab View and Pole Figure Display (for example ``Show Goniometers``).

STL Loading
    Controls applied when a shape is loaded from an ``.stl`` mesh file:

    - **Scale** - the units of the STL file's coordinates.
    - **Rotate X / Y / Z** - rotation about each axis applied as the file is loaded
      (degrees).
    - **Translation** - a comma-separated translation vector applied as the file is
      loaded, e.g. ``0,0,0``.

Orientation File
    Defines how the Euler angles in a loaded orientation file are interpreted (and
    how rotation matrices are decomposed for display):

    - **Axes** - the lab-frame axes the Euler angles are defined along, e.g.
      ``YXY``.
    - **Senses** - the sense of rotation about each axis (``1`` = counterclockwise,
      ``-1`` = clockwise), one comma-separated value per axis, e.g. ``-1,-1,-1``.

Monte Carlo Simulation
    Controls the Monte Carlo simulation used for the transmission estimate:

    - **Events** - number of Monte Carlo events per detector point (higher is more
      accurate but slower).
    - **Max scatter attempts** - maximum number of attempts to find a valid
      scattering point inside the sample.
    - **Simulate in** - the region in which the scattering point is simulated.
    - **Resimulate per wavelength** - whether to resimulate scattering tracks for
      different wavelengths (more accurate but significantly slower).

Attenuation
    Controls how the attenuation used by the transmission estimate is read and
    displayed:

    - **Point** and **Unit** - the value (and its unit) at which the attenuation
      coefficient is read from the absorption workspace.
    - **Use data range** - if enabled, the estimated transmission plot colour scale
      spans the data range; otherwise it is fixed between 0 and 1.

Output Options
^^^^^^^^^^^^^^

Save Directory
    The directory to save the output files into.

Filename
    The file name for the output file.

Export
    The type of file to write (see the options below).

SScanSS-2 angles
    Write the included orientations into a ``.angles`` file which can be loaded into
    SScanSS-2.

Euler Orientation file
    Write the included orientations into an Euler angle ``.txt`` file. The axes used
    are the axes defined in the settings.

Matrix Orientation file
    Write the included orientations into a flattened rotation matrix ``.txt`` file.

Reference Workspace
    Write the defined sample onto a reference workspace ``.nxs`` file. This can be
    loaded directly into the Absorption Correction tab of the
    :ref:`Engineering Diffraction interface <Engineering_Diffraction-ref>` to start
    processing results.

Transmission Weighting
    Write a ``.txt`` file with one line per included orientation. For each line the
    value given is the transmission value of the most absorbing virtual detector,
    normalised against the orientation with the highest of these values. This gives
    an estimate of the relative counting time required to match the scattered
    intensity of the least absorbing orientation.

.. categories:: Interfaces Diffraction
