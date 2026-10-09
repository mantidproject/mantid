
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm will convert the output of :ref:`algm-LoadWANDSCD` or
the autoreduced data from DEMAND (HB3A) into a :py:obj:`MDEventWorkspace <mantid.api.IMDWorkspace>` in Q-sample,
where every pixel at every scan point is converted to a MDEvent.
This is similar to :ref:`algm-ConvertWANDSCDtoQ` except that it doesn't histogram the
data or do normalization. :ref:`algm-FindPeaksMD` can be run on the
output Q sample space, then the UB can be found and used to then
convert to HKL using :ref:`algm-ConvertWANDSCDtoQ`
. :ref:`algm-IntegratePeaksMD` will also work on the output of this
algorithm.

There is an option to apply the LorentzCorrection using the formula :math:`|\sin(2\theta)\cos(\phi)|/\lambda^3`. This helps lower the sloping background at low :math:`Q`.

Wavelength
##########

The incident wavelength of each input workspace is resolved in the same order as in
:ref:`algm-HB3AAdjustSampleNorm`:

1. the ``wavelength`` sample log of the input workspace, when present. A numeric or string log is accepted, and the
   mean is used for a time series log;
2. otherwise, the value given in the ``Wavelength`` property;
3. otherwise, the algorithm stops with an error.

The ``Wavelength`` property therefore acts as a fallback. For a single input workspace, give at most one value. For a
WorkspaceGroup, give either one value, used for every member that has no ``wavelength`` sample log, or one value per
member, in group order. When a ``wavelength`` sample log is present and differs from the value given in the
``Wavelength`` property, the property value is ignored and a warning is logged. The resolved wavelength is stored in
the ``wavelength`` sample log of the output.

Grouped input
#############

``InputWorkspace`` can also be a WorkspaceGroup of detector-space MDHistoWorkspaces, such as the output of
:ref:`algm-HB3AAdjustSampleNorm` with ``OutputType="Detector"`` for several scans. All members must come from the
same instrument; they may have different numbers of scan points. Each member is converted independently, with its own
goniometer settings and wavelength. Every member is validated before any conversion starts, and an error names the
member that caused it.

- With ``MergeInputs=False`` (the default), the output is a WorkspaceGroup of MDEventWorkspaces in the order of the
  input group. Each member is named ``<OutputWorkspace>_<input member name>``, following the convention of
  :ref:`algm-HB3AAdjustSampleNorm`. A member of an input group that has no name is identified by its position,
  starting at 1. These names are given only when the algorithm stores its output in the Analysis Data Service, as
  when it is run from Python or the GUI. When it is run as a child algorithm, the members are not named; if the
  output group is later added to the Analysis Data Service, its members get the default names of a WorkspaceGroup,
  such as ``<group name>_1``.
- With ``MergeInputs=True``, the converted members are merged with :ref:`algm-MergeMD` into a single MDEventWorkspace,
  and no intermediate workspaces are kept. The ``SplitInto``, ``SplitThreshold`` and ``MaxRecursionDepth`` properties
  of this algorithm are passed to :ref:`algm-MergeMD`. Because :ref:`algm-HB3AAdjustSampleNorm` merges with the
  :ref:`algm-MergeMD` defaults, its merged output contains the same events but a different box structure.

``MergeInputs`` is ignored for a single input workspace.

Usage
-----

**Example - ConvertHFIRSCDtoMDE**

.. code-block:: python

   LoadWANDSCD(IPTS=7776, RunNumbers='26640-27944', OutputWorkspace='data',Grouping='4x4')
   ConvertHFIRSCDtoMDE(InputWorkspace='data',
                       Wavelength=1.488,
                       OutputWorkspace='Q')


Output:

.. figure:: /images/ConvertHFIRSCDtoMDE.png

**Example - ConvertHFIRSCDtoMDE with a group of HB3A scans**

This example converts 68 scans of HB3A (DEMAND). It requires access to the data of IPTS-24855.

.. code-block:: python

   IPTS = 24855
   exp = 755
   scans = range(28, 96)
   filename = "/HFIR/HB3A/IPTS-{}/shared/autoreduce/HB3A_exp{:04}_scan{:04}.nxs"
   files = ",".join(filename.format(IPTS, exp, scan) for scan in scans)

   # WorkspaceGroup with one detector-space MDHistoWorkspace per scan
   HB3AAdjustSampleNorm(Filename=files,
                        OutputType="Detector",
                        NormaliseBy="None",
                        NormalizeData=False,
                        Grouping="4x4",
                        OutputWorkspace="detector")

   # One Q-sample MDEventWorkspace per scan, named Q_<input member name>.
   # Each scan has a "wavelength" sample log, so the Wavelength property is not needed.
   ConvertHFIRSCDtoMDE(InputWorkspace="detector",
                       OutputWorkspace="Q")

   # All scans converted with their own goniometer settings, then merged into one MDEventWorkspace
   ConvertHFIRSCDtoMDE(InputWorkspace="detector",
                       MergeInputs=True,
                       OutputWorkspace="Q_merged")

:ref:`algm-HB3AAdjustSampleNorm` loads each scan at the full detector resolution of 512 x 1536 pixels before grouping.
Building the input group for these 68 scans with ``Grouping="4x4"`` used about 28 GB of memory. The two calls to
ConvertHFIRSCDtoMDE use little additional memory.

.. diagram:: ConvertHFIRSCDtoMDE-v1_wkflw.dot

.. categories::

.. sourcelink::
