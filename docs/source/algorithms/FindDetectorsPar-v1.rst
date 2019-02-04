.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Identifies geometrical parameters of detectors and groups of detectors
after the workspaces were grouped using ASCII or XML map file. Located
in DataHandling\\Instrument\\Detectors group and intended to be used as
Child Algorithm of :ref:`SaveNXSPE <algm-SaveNXSPE>`
algorithm, though can be deployed independently. Dynamic casting from iAlgorithm
and accessors functions
return calculated parameters to :ref:`SaveNXSPE <algm-SaveNXSPE>`
when FindDetectorsPar used as
the Child Algorithm of :ref:`SaveNXSPE <algm-SaveNXSPE>`
procedure;

Internal Child Algorithm identifies the group topology, namely if a
group of detectors is arranged into a rectangular shape or in a ring.
The algorithm calculates the geometrical centre of the detectors group
and 6 points, located within +-1/4 width of the first detector of the
group. If the centre or any of these points belong to the group of the
detectors itself, the group assumed to have a rectangular topology, and
if not -- the cylindrical one (ring).

Single detector defined to have the rectangular shape.

After identifying the topology, the parameters are calculated using
formulas for angles in Cartesian or Cylindrical coordinate systems
accordingly

:ref:`par <algm-SavePAR>` and :ref:`phx <algm-SavePHX>` files
-------------------------------------------------------------

These files are ascii files which are used to describe the combined
detectors geometry defined by map files. There are no reasons for you to
use it unless this Mantid algorithm is working unsatisfactory for you.
In this case you can quickly modify and use par file until this
algorithm is modified. It is your responsibility then to assure the
correspondence between mapped detectors and parameters in the par file.

The par files are simple ASCII files with the following columns::

    1st column      sample-detector distance (m)
    2nd  "          scattering angle (deg)
    3rd  "          azimuthal angle (deg)   (west bank = 0 deg, north bank = -90 deg etc.)   (Note the reversed sign convention cf .phx files)
    4th  "          width  (m)
    5th  "          height (m)

When processed by this algorithm, 4th and 5th column are transformed
into angular values.

:ref:`Phx <algm-SavePHX>`
files are Mslice phx files, which do not contain
secondary flight path. This path is calculated by the algorithm from the
data in the instrument description and the angular values are calculated
as in nxspe file. There are no reason to use phx files to build nxspe
files at the moment unless you already have one and need to repeat your
previous results with Mantid.

Usage
-----

.. caution::

    This algorithm is meant to be run as a child algorithm to one of the previously
    mentioned algorithms. However, it can be used alone.

.. include:: ../usagedata-note.txt

**Example - Pars with (dPolar, dAzimuthal)**

.. testcode:: ExDpolDazi

    ws = Load("MAR11001.raw")
    # Output workspace is None if OutputParTable is not used)
    pars = FindDetectorsPar(ws, OutputParTable="pars")
    # pars is a TableWorkspace
    print("Workspace type = {}".format(pars.id()))
    # Show width column headers
    print("Width headers = ( {} , {} )".format(pars.getColumnNames()[3], pars.getColumnNames()[4]))

Output:

.. testoutput:: ExDpolDazi

    Workspace type = TableWorkspace
    Width headers = ( polar_width , azimuthal_width )

**Example - Pars with (dX (width), dY (height))**

.. testcode:: ExDxDy

    ws = Load("MAR11001.raw")
    # Output workspace is None if OutputParTable is not used)
    pars = FindDetectorsPar(ws, ReturnLinearRanges=True, OutputParTable="pars")
    # pars is a TableWorkspace
    print("Workspace type = {}".format(pars.id()))
    # Show width column headers
    print("Width headers = ( {} , {} )".format(pars.getColumnNames()[3], pars.getColumnNames()[4]))

Output:

.. testoutput:: ExDxDy

    Workspace type = TableWorkspace
    Width headers = ( det_width , det_height )


.. categories::

.. sourcelink::
