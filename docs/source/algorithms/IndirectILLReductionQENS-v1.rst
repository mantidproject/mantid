.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Performs a multiple-file QENS (Quasi-Elastic Neutron Scattering) data reduction for indirect geometry **ILL** instrument **IN16B**.
It uses internally the :ref:`IndirectILLEnergyTransfer <algm-IndirectILLEnergyTransfer>` algorithm.

Multiple File Reduction
#######################

The algorithm is capable of running over multiple files.  Run property
needs to be specified following the syntax in :py:obj:`MultipleFileProperty <mantid.api.MultipleFileProperty>`.
When ``SumRuns=True``, all the runs will be merged while loading.
Note, for **Range** and **Stepped Range**, ``SumRuns`` will be
ignored.  Use **Added Range** and **Added Stepped Range** instead (see
:py:obj:`MultipleFileProperty <mantid.api.MultipleFileProperty>`).  For ``BackgroundRun``,
``CalibrationRun``, ``CalibrationBackgroundRun`` and ``AlignmentRun`` all the runs will be automatically summed.

Unmirror Options
################

**IN16B** can record data with mirror sense, where the spectra for the acceleration and
deceleration phase of the Doppler drive are recorded separately, or without.
Technically this is defined in the ``Doppler.mirror_sense`` entry in the sample logs.
For the data without mirror sense (i.e. mirror_sense = 16) only three unmirror options are valid:

0: No x-axis shift. (Options 0-5 will fall back to 0).

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

:ref:`MatchPeaks <algm-MatchPeaks>` algorithm is invoked for aligning the peaks with different options.

Note, that both detector calibration and background subtraction are performed wing-by-wing, i.e. unmirroring is the very final step.

Vanadium Calibration
####################

Integration range can be specified to integrate over spectra in ``CalibrationRun``. Note, that before integration, the spectra will be
centered at 0-energy transfer (see Unmirror Option 6 above) for the calibration run.


Output
------

A :ref:`WorkspaceGroup <WorkspaceGroup>` will be returned, containing workspaces for each individual (unsummed) run.

Workflow
--------

.. diagram:: IndirectILLReductionQENS-v1_wkflw.dot

Usage
-----

**Example - IndirectILLReduction : default options**

.. testsetup:: ExIndirectILLReductionQENS

   config['default.facility'] = 'ILL'
   config['default.instrument'] = 'IN16B'

.. testcode:: ExIndirectILLReductionQENS

    ws = IndirectILLReductionQENS(Run='ILL/IN16B/136553:136555.nxs')
    print("Result is a WorkspaceGroup, that contains {:d} workspaces".format(ws.getNumberOfEntries()))
    print("the name of the first one is {} corresponding to run 136553".format(ws.getItem(0).name()))
    print("it has {:d} spectra and {:d} bins".format(ws.getItem(0).getNumberHistograms(),ws.getItem(0).blocksize()))

Output:

.. testoutput:: ExIndirectILLReductionQENS

    Result is a WorkspaceGroup, that contains 3 workspaces
    the name of the first one is 136553_ws_red corresponding to run 136553
    it has 18 spectra and 1024 bins

.. testcleanup:: ExIndirectILLReductionQENS

   DeleteWorkspace(ws)

.. categories::

.. sourcelink::
