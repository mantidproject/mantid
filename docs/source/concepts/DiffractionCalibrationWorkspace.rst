.. _DiffractionCalibrationWorkspace:

Diffraction Calibration Workspace
=================================

The calibration workspace contains information to convert data to/from
time-of-flight and d-space using the `GSAS
<https://subversion.xor.aps.anl.gov/trac/pyGSAS>`_ parameters
``DIFC``, ``DIFA``, ``TZERO``.  The workspace itself is a
:ref:`TableWorkspace <Table Workspaces>` with the columns

+-------+-----------+
| name  | type      |
+=======+===========+
| detid | int32     |
+-------+-----------+
| difc  | double    |
+-------+-----------+
| difa  | double    |
+-------+-----------+
| tzero | double    |
+-------+-----------+


The order of the rows and columns will not matter to algorithms that
use the workspace. Algorithms that create the :ref:`TableWorkspace
<Table Workspaces>` will use this column order with the rows sorted by
``detid`` (smallest first). Any missing column, other than ``detid``,
will be assumed to be all zeros.

Diffraction Calibration File
============================

Stored in `HDF5 <http://www.hdfgroup.org/>`_ using as simple a
`nexus-style <Nexus file>`_ format to allow for external programs to
read/write them without excessive effort. Missing values will be
assumed to be zero. The data will be stored as multiple parallel
1-dimensional arrays (of length ``n``) as described below. In addition
there will be sufficient information to denote which instrument
geometry file to use. This geometry will only be used for plotting the
various parameters on an instrument view.

* ``calibration`` [with attribute ``NX_class=NXentry``]

  * ``instrument`` [with attribute ``NX_class=NXinstrument``]

    * ``name`` (e.g. NOMAD)
    * ``instrument_source`` (e.g. NOMAD_Definition.xml or NOMAD_Definition_20120701-20120731.xml)

  * ``difc`` (double array)
  * ``difa`` (double array)
  * ``tzero`` (double array)
  * ``detid`` (int32 array)
  * ``dasid`` (int32 array) not used. The pixel number in prenexus files.
  * ``group`` (int32 array) 1 being smallest number. 0 will set to not use. This can be used in addition to
  * ``use`` (int32 array) ``0=false`` and ``1=true``
  * ``offset`` (double array) not used. Value of the legacy calibration file.

The `group` information will still be extracted into separate `GroupingWorkspace` and `MaskWorkspace`.

.. categories:: Concepts
