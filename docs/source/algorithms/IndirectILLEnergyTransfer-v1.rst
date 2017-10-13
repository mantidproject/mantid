.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This is a part of multi-algorithm reduction workflow for **IN16B** indirect geometry instrument at **ILL**.
It handles the first steps of the reduction chain, such as grouping of the detectors, normalizing to monitor dependent on the reduction type.
It performs transformation of the axes; x-axis from channel number to energy transfer, and optionally y-axis to scattering angle or elastic momentum transfer.
It handles **automatically** all three types of data (QENS, EFWS, IFWS) recorded with or without mirror sense.
Note, that following the standard, the ``Unit`` for energy transfer (``DeltaE``) will be mili-elevtron-volts (``mev``).
This algorithm is intended to handle only single file at a time, although if multiple files are given, they will be automatically summed at raw level, i.e. while loading.
In this case ``MergeRuns`` algorithm will be invoked, which will forbid the merges across different types of data
(e.g. different mirror senses, doppler energy or velocity profiles).
Note, that this algorithm is compatible with the data recorded from 03.2014 onwards
(i.e. where Doppler's ``mirror_sense``, ``maximum_delta_energy`` and ``velocity_profile`` entries are defined in ``.nxs`` files).
It returns a :ref:`WorkspaceGroup <WorkspaceGroup>`, containing one (no mirror sense) or two workspaces (with mirror sense) for left and right wings respectively.
This algorithm is **not** intended to be used directly by the end users. Instead it is used as a child algorithm by :ref:`IndirectILLReductionQENS <algm-IndirectILLReductionQENS>`
and :ref:`IndirectILLReductionFWS <algm-IndirectILLReductionFWS>` for QENS and FWS type of reductions correspondingly.


Workflow
--------

.. diagram:: IndirectILLEnergyTransfer-v1_wkflw.dot

Usage
-----

**Example - IndirectILLEnergyTransfer : QENS data without mirror sense**

.. testsetup:: ExIndirectILLEnergyTransfer

   config['default.facility'] = 'ILL'
   config['default.instrument'] = 'IN16B'

.. testcode:: ExIndirectILLEnergyTransfer

    ws = IndirectILLEnergyTransfer(Run='ILL/IN16B/090661.nxs')
    print("Reduced workspace has {:d} wing".format(ws.getNumberOfEntries()))
    print("which has {:d} spectra".format(ws.getItem(0).getNumberHistograms()))
    print("and {:d} bins".format(ws.getItem(0).blocksize()))

Output:

.. testoutput:: ExIndirectILLEnergyTransfer

    Reduced workspace has 1 wing
    which has 18 spectra
    and 1024 bins

.. testcleanup:: ExIndirectILLEnergyTransfer

   DeleteWorkspace('ws')

**Example - IndirectILLEnergyTransfer : QENS data with mirror sense**

.. testsetup:: ExIndirectILLEnergyTransferMirrorSense

   config['default.facility'] = 'ILL'
   config['default.instrument'] = 'IN16B'

.. testcode:: ExIndirectILLEnergyTransferMirrorSense

    ws = IndirectILLEnergyTransfer(Run='ILL/IN16B/136553:136555.nxs', CropDeadMonitorChannels=True)
    print("Reduced workspace has {:d} wings".format(ws.getNumberOfEntries()))
    print("which have {:d} spectra".format(ws.getItem(0).getNumberHistograms()))
    print("and {:d} bins".format(ws.getItem(0).blocksize()))

Output:

.. testoutput:: ExIndirectILLEnergyTransferMirrorSense

    Reduced workspace has 2 wings
    which have 18 spectra
    and 1017 bins

.. testcleanup:: ExIndirectILLEnergyTransferMirrorSense

   DeleteWorkspace('ws')

.. categories::

.. sourcelink::
