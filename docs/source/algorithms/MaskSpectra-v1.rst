
.. algorithm::

.. summary::

.. alias::

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
  print(ws.readY(0), masked.readY(0))
  print(ws.readY(1), masked.readY(1))

Output:

.. testoutput:: MaskSpectraExample

  (array([ 2.]), array([ 2.]))
  (array([ 2.]), array([ 0.]))

.. categories::

.. sourcelink::

