.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Performs a multiple-file QENS (Quasi-Elastic Neutron Scattering) data reduction for indirect geometry **ILL** instrument **IN16B**.
It uses internally the :ref:`IndirectILLEnergyTransfer <algm-IndirectILLEnergyTransfer>` algorithm.

Multiple File Reduction
~~~~~~~~~~~~~~~~~~~~~~~
The algorithm is capable of running over multiple files.
Run property needs to be specified following the syntax in `MultiFileLoading <http://www.mantidproject.org/MultiFileLoading>`_.
When ``SumRuns=True``, all the runs will be merged while loading.
Note, for **Range** and **Stepped Range**, ``SumRuns`` will be ignored.
Use **Added Range** and **Added Stepped Range** instead (see `MultiFileLoading <http://www.mantidproject.org/MultiFileLoading>`_).
For ``BackgroundRun``, ``CalibrationRun`` and ``AlignmentRun`` all the runs will be automatically summed.

Unmirror Options
~~~~~~~~~~~~~~~~

**IN16B** can record data with mirror sense, where the spectra for the acceleration and
deceleration phase of the Doppler drive are recorded separately, or without.
Technically this is defined in the ``Doppler.mirror_sense`` entry in the sample logs.
For the data without mirror sense (i.e. mirror_sense = 16) only three unmirror options are valid:

0: No x-axis shift.

6: Centering the peaks at the zero energy transfer.

7: Centering the peaks using the corresponding vanadium alignment run.

For the data with mirror sense (i.e. mirror_sense = 14) there are 8 options available:

0: Left and right wings are returned separately.

1: Left and right wings will summed.

2: Left wing will be returned.

3: Right wing will be returned.

4: Peaks in the right wing will be positioned at peak positions in the left wing, and then they will be summed.

5: Right wing will be shifted according with the offsets of the peak positions in left and right wings in vanadium alignment run.

6: Peaks in both, left and right wings will be centered at zero energy transfer and then they will be summed.

7: Left and right wings will be shifted according to offsets of peak positions of left and right wings in corresponding vanadium alignment run.

Options 5 and 7 require the ``AlignmentRun`` (vanadium) to determine the peak positions to align with.

Output
------

A :ref:`WorkspaceGroup <WorkspaceGroup>` will be returned, containing workspaces for each individual (unsummed) run.

Workflow
--------

.. diagram:: IndirectILLReductionQENS-v1_wkflw.dot

Usage
-----

**Example - IndirectILLReduction : default options**

.. testcode:: ExIndirectILLReductionQENS

    ws = IndirectILLReductionQENS(Run='136553:136555')
    print "Result is a WorkspaceGroup, that contains %d workspaces" % ws.getNumberOfEntries()
    print "the name of the first one is %s corresponding to run 136553" % ws.getItem(0).getName()
    print "it has %d spectra and %d bins" % (ws.getItem(0).getNumberHistograms(),ws.getItem(0).blocksize())

Output:

.. testoutput:: ExIndirectILLReductionQENS

    Result is WorkspaceGroup, that contains 3 workspaces
    the name of the first one is 136553_ws corresponding to run 136553
    it has 18 spectra and 1024 bins

.. categories::

.. sourcelink::
