.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm reduces data acquired by the diffraction detectors of **IN16B** indirect geometry instrument at **ILL**.

Ultimately, it will handle both BATS and Doppler data, but the later is the only one implemented as of now.

Doppler
-------

To reduce data acquired in Doppler mode, this algorithms sums data over all Doppler channels and normalize by monitor counts.

Usage
-----

**Example - IndirectILLReductionDIFF**

.. testsetup:: ExIndirectILLReductionDIFF

   config['default.facility'] = 'ILL'
   config['default.instrument'] = 'IN16B'

.. testcode:: ExIndirectILLReductionDIFF

    ws = IndirectILLEnergyTransfer(Run='ILL/IN16B/276047.nxs')
    print("Reduced workspace has {:d} spectra".format(ws.getItem(0).getNumberHistograms()))
    print("and {:d} bins.".format(ws.getItem(0).blocksize()))

Output:

.. testoutput:: ExIndirectILLReductionDIFF

    Reduced workspace has 18 spectra
    and 512 bins.

.. testcleanup:: ExIndirectILLReductionDIFF

   DeleteWorkspace('ws')

.. categories::

.. sourcelink::
