.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

A workflow algorithm to perform a data reduction for indirect **ILL** instruments. Currently only **IN16B** is supported.
This algorithm performs QENS (Quasi-Elastic Neutron Scattering) type of reduction.

SumRuns
~~~~~~~

All runs will be summed while loading, before the reduction. The run number of the first run will be used for naming the single output workspace.

Unmirror Options
~~~~~~~~~~~~~~~~

**IN16B** can record data with mirror sense, where the spectra for the acceleration and
deceleration phase of the Doppler drive are recorded separately, or without.
Technically this is defined in the `Doppler.mirror_sense` entry in the sample logs.
For the data without mirror sense (technically, mirror_sense == 16) only three unmirror options are valid:

0: No x-axis shift.

6: Centering the peaks at the zero energy transfer.

7: Centering the peaks using the corresponding vanadium run.

For the data with mirror sense (i.e. mirror_sense == 14) there are 8 options available:

0: No x-axis shift, no energy conversion can be performed.

1: Left and right wings will summed.

2: Left wing will be returned.

3: Right wing will be returned.

4: Peaks in the right wing will be positioned at peak positions in the left wing, and then they will be summed.

5: Right wing will be shifted according with the offsets of the peak positions in left and right wings in vanadium run.

6: Peaks in both, left and right wings will be centered at zero energy transfer and then they will be summed.

7: Left and right wings will be shifted according to offsets of peak positions of left and right wings in corresponding vanadium run.

Options 5,7 require the ``VanadiumRun``.
The options ``4-7`` rely on :ref:`FindEPP <algm-FindEPP>` algorithm to find the peak positions.
All spectra of the reduced workspaces are converted to scattering angle as y-axis.
Note, that it is forbidden to reduce several runs with different mirror senses.
Technically, the mirror sense will be deduced from the first run, and all the rest will be forced to comply with it, otherwise will be skipped.

Multiple File Reduction
~~~~~~~~~~~~~~~~~~~~~~~
The algorithm is capable of running over multiple files.  
Run needs to be specified following the Mantid conventions in `MultiFileLoading <http://www.mantidproject.org/MultiFileLoading>`_.
When ``SumRuns=True``, all the numors will be merged while loading.
Note, for **Range** and **Stepped Range** (see `MultiFileLoading <http://www.mantidproject.org/MultiFileLoading>`_), ``SumRuns`` will be ignored.
Please use **Added Range** and **Added Stepped Range** instead (see `MultiFileLoading <http://www.mantidproject.org/MultiFileLoading>`_).

CalibrationWorkspace 
~~~~~~~~~~~~~~~~~~~~
Note, that this is not the same as ``VanadiumRun``. This represents a one column workspace containing calibration intensities
computed with :ref:`ILLIN16BCalibration <algm-ILLIN16BCalibration>` algorithm.
It then can be seeded back to :ref:`IndirectILLReduction <algm-IndirectILLReduction>` to use for calibration.
Note, that :ref:`ILLIN16BCalibration <algm-ILLIN16BCalibration>` itself just calls :ref:`IndirectILLReduction <algm-IndirectILLReduction>`
for the given (vanadium) run and performs Integration around the specified peak range.
There is hence no restriction for ``VanadiumRun`` for the alignment be the same as the calibration run.

DebugMode
~~~~~~~~~
This provides a flexibility to monitor the snapshots of workspaces at different intermediate steps.
If enabled, along with the reduced, left and right workspaces, many other workspaces will be created.
They also will be grouped and a tuple of many :ref:`WorkspaceGroup <WorkspaceGroup>` s will be returned, where the
first item would be the group for the final reduced result, and the rest of items will be additional intermediate results.

Output Naming Conventions
~~~~~~~~~~~~~~~~~~~~~~~~~
Note that to avoid confusion when running over multiple files,
the unique run number will be automatically prepended to the output workspace name.
The output workspace will always be grouped and :ref:`WorkspaceGroup <WorkspaceGroup>` will be returned,
containing workspaces for each individual run (i.e. one item for single run, many items for multiple runs).

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

    IndirectILLReduction(Run='146191.nxs')
    print "Reduced workspace has %d spectra" % mtd['146191_red'].getNumberHistograms()
    print "Reduced workspace has %d bins" % mtd['146191_red'].blocksize()

Output:

.. testoutput:: ExIndirectILLReduction

    Reduced workspace has 18 spectra
    Reduced workspace has 1024 bins

**Example - IndirectILLReduction : single run with handler**

.. testcode:: ExIndirectILLReductionSingleRun

    out = IndirectILLReduction(Run='146191.nxs')
    print "out is now refers to a group workspace, which is called %s" % out.getName()
    print "it contains %d item, which is called %s" % (out.size(),out.getItem(0).getName())

Output:

.. testoutput:: ExIndirectILLReductionSingleRun

    out is now refers to a group workspace, which is called out
    it contains 1 item, which is called 146191_out

**Example - IndirectILLReduction : multiple runs**

.. testcode:: ExIndirectILLReductionMultipleRun

    result = IndirectILLReduction(Run='146191,146192.nxs')
    print "result contains %d workspaces, one for each run" % result.size()
    print "first workspace is %s corresponding to run %i" % (result.getItem(0).getName(),result.getItem(0).getRunNumber())

Output:

.. testoutput:: ExIndirectILLReductionMultipleRun

    result contains 2 workspaces, one for each run
    first workspace is 146191_result corresponding to run 146191

.. categories::

.. sourcelink::
