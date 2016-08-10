.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

A workflow algorithm to perform a data reduction for indirect **ILL** instruments. Currently only **IN16B** is supported.

Unmirror Options
~~~~~~~~~~~~~~~~

When **IN16B** records data in mirror sense the spectra for the acceleration and
deceleration phase of the Doppler drive are recorded separately, the result is
each spectra containing two regions for the same energy range.

Enabling ``MirrorSense=True`` on this algorithm will split the data for each spectrum into
two separate spectra, these form the **left** and **right** workspaces that are
reduced independently and then summed according to ``UnmirrorOption`` as follows:

0: No summing of left and right will be performed. The reduced workspace will containt both wings and x-axis will not be converted to energy transfer.
``MirrorSense=False`` will fall back to this option.

1: Left wing will be returned as reduced workspace.

2: Right wing will be returned as reduced workspace.

3: Left and right wings will be simply summed and returned as reduced workspace.

4: Peaks in the right wing will be positioned at peak positions in the left wing, and then they will be summed.

5: Right wing will be shifted with the offset of the peak positions of the right wing of the corresponding vanadium run.
It will then be summed with left wing. ``VanadiumRun`` needs to be specified.

6: Peaks in both, left and right wings will be centered at zero energy transfer and then they will be summed.

7: Left and right wings will be shifted according to offsets of peak positions in corresponding vanadium run.
They will then be summed and returned. ``VanadiumRun`` needs to be specified.

The options ``4-6`` rely on :ref:`FindEPP <algm-FindEPP>` algorithm to find the peak positions.

These options are inherited identically from (and validated against) previous **LAMP** software, to enable smooth transition for the users.

Multiple File Reduction
~~~~~~~~~~~~~~~~~~~~~~~
The algorithm is capable of running over multiple files.
Run needs to be specified following the Mantid conventions in `MultiFileLoading <http://www.mantidproject.org/MultiFileLoading>`_.
When ``SumRuns=True``, all the numors will be merged while loading.
Note, for **Range** and **Stepped Range** (see `MultiFileLoading <http://www.mantidproject.org/MultiFileLoading>`_), ``SumRuns`` will be ignored.
Please use **Added Range** and **Added Stepped Range** instead (see `MultiFileLoading <http://www.mantidproject.org/MultiFileLoading>`_).
In case of multiple files specified, the output will be :ref:`WorkspaceGroup <WorkspaceGroup>`
containing :ref:`MatrixWorkspace <MatrixWorkspace>` for each
individual run in the input files list.

CalibrationWorkspace
~~~~~~~~~~~~~~~~~~~~
Note, that this is not the same as ``VanadiumRun``. This represents a one column workspace containing calibration intensities
computed with :ref:`ILLIN16BCalibration <algm-ILLIN16BCalibration>` algorithm.
It then can be seeded back to :ref:`IndirectILLReduction <algm-IndirectILLReduction>` to use for calibration.
Note, that :ref:`ILLIN16BCalibration <algm-ILLIN16BCalibration>` itself just calls :ref:`IndirectILLReduction <algm-IndirectILLReduction>`
for the given (vanadium) run and performs Integration around the specified peak range.

ControlMode
~~~~~~~~~~~
This provides a flexibility to monitor the snapshots of workspaces at different intermediate steps.
If enabled, along with the reduced, left and right workspaces, many other workspaces will be created.

Output Naming Conventions
~~~~~~~~~~~~~~~~~~~~~~~~~
Note that to avoid confusion when running over multiple files,
the unique run number will be automatically prepended to the output workspace name.

For multiple runs, the output workspace will be grouped and
:ref:`WorkspaceGroup <WorkspaceGroup>` will be returned,
containing workspaces for each individual run.

Energy Transfer Unit
~~~~~~~~~~~~~~~~~~~~
Note, that following Mantid standard, the ``Unit`` for energy transfer (``DeltaE``) will be mili-elevtron-volts (``mev``).

Workflow
--------

.. diagram:: IndirectILLReduction-v1_wkflw.dot

Usage
-----

**Example - IndirectILLReduction : minimal run**

.. testcode:: ExIndirectILLReduction

    IndirectILLReduction(Run='146190.nxs')

    print "Reduced workspace has %d spectra" % mtd['146190_red'].getNumberHistograms()
    print "Reduced workspace has %d bins" % mtd['146190_red'].blocksize()

Output:

.. testoutput:: ExIndirectILLReduction

    Reduced workspace has 18 spectra
    Reduced workspace has 1024 bins

**Example - IndirectILLReduction : single run with handler**

.. testcode:: ExIndirectILLReductionSingleRun

    out = IndirectILLReduction(Run='146190.nxs')
    print "out is now a reference to workspace, which is called %s" % out.getName()

Output:

.. testoutput:: ExIndirectILLReductionSingleRun

    out is now a reference to workspace, which is called 146190_out

**Example - IndirectILLReduction : multiple runs**

.. testcode:: ExIndirectILLReductionMultipleRun

    result = IndirectILLReduction(Run='146190:146191.nxs',UnmirrorOption=3)
    print "result is now the reduced workspace group called %s" % result.getName()
    print "it contains %d workspaces, one for each run" % result.size()
    print "first workspace is %s corresponding to run %i" % (result.getItem(0).getName(),result.getItem(0).getRunNumber())

Output:

.. testoutput:: ExIndirectILLReductionMultipleRun

    result is now the reduced workspace group called result
    it contains 2 workspaces, one for each run
    first workspace is 146190_result corresponding to run 146190

.. categories::

.. sourcelink::
