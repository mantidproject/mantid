
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Mask (zero) spectra and the underlying detectors in a workspace.
If a spectrum corresponds to more than one detector, all detectors are masked.

Usage
-----

**Example - MaskSpectra**

.. testcode:: MaskSpectraExample

  ws = CreateWorkspace(OutputWorkspace='ws', DataX='1,1', DataY='2,2', NSpec=2)
  masked = MaskSpectra(InputWorkspace=ws, InputWorkspaceIndexType='WorkspaceIndex', InputWorkspaceIndexSet='1')
  print('Spectrum 0 before {} after {}'.format(ws.readY(0)[0], masked.readY(0)[0]))
  print('Spectrum 1 before {} after {}'.format(ws.readY(1)[0], masked.readY(1)[0]))

Output:

.. testoutput:: MaskSpectraExample

  Spectrum 0 before 2.0 after 2.0
  Spectrum 1 before 2.0 after 0.0

.. categories::

.. sourcelink::
