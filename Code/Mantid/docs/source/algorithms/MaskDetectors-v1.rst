.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm will flag the detectors listed as
masked(\ `IDetector <http://www.mantidproject.org/IDetector>`__::isMasked() method) and will zero the
data in the spectra for MatrixWorkspaces related to those detectors.  For PeaksWorkspaces, only the 
detectors listed are masked and the mask must be specified by a DetectorList or MaskedWorkspace.

All but the first property are optional and at least one of them must be
set. If several are set, the first will be used.

The set of detectors to be masked can be given as a list of either
spectrum numbers, detector IDs or workspace indices. The list should be
set against the appropriate property.

Mask Detectors According To Instrument
######################################

If the input MaskedWorkspace is not a SpecialWorkspace2D object, this
algorithm will check every detectors in input MaskedWorkspace's
Instrument. If the detector is masked, then the corresponding detector
will be masked in Workspace.

Mask Detectors According to Masking Workspace
#############################################

If the input MaskedWorkspace is a `MaskWorkspace <http://www.mantidproject.org/MaskWorkspace>`__
object, i.e., masking workspace, then the algorithm will mask
Workspace's detector according to the histogram data of the
SpecialWorkspace2D object

Definition of Mask
##################

-  If a pixel is masked, it means that the data from this pixel won't be
   used. In the masking workspace (i.e.,
   `SpecialWorkspace2D <http://www.mantidproject.org/SpecialWorkspace2D>`__), the corresponding value
   is 1.
-  If a pixel is NOT masked, it means that the data from this pixel will
   be used. In the masking workspace (i.e.,
   `SpecialWorkspace2D <http://www.mantidproject.org/SpecialWorkspace2D>`__), the corresponding value
   is 0.

About Input Parameters
######################

:ref:`algm-MaskDetectors` supports various format of input to
mask detectors, including

-  Workspace indices
-  Spectra
-  Detectors
-  `MaskWorkspace <http://www.mantidproject.org/MaskWorkspace>`__
-  General :ref:`MatrixWorkspace <MatrixWorkspace>` other than
   `MaskWorkspace <http://www.mantidproject.org/MaskWorkspace>`__ (In this case, the mask will be
   extracted from this workspace)

Rules
#####

Here are the rules for input information for masking

1. At least one of the inputs must be specified.
2. Workspace indices and Spectra cannot be given at the same time.
3. `MaskWorkspace <http://www.mantidproject.org/MaskWorkspace>`__  and general :ref:`MatrixWorkspace <MatrixWorkspace>` cannot be given at the same time.
4. When a general :ref:`MatrixWorkspace <MatrixWorkspace>` is specified, then all detectors in a spectrum are treated as masked if the effective detector of that spectrum is masked.
5. The masks specified from

   a) workspace indices/spectra
   b) detectors
   c) `MaskWorkspace <http://www.mantidproject.org/MaskWorkspace>`__ /general :ref:`MatrixWorkspace <MatrixWorkspace>` will be combined by the *plus* operation.

Operations Involved in Masking
##############################

There are 2 operations to mask a detector and thus spectrum related

1. Set the detector in workspace's instrument's *parameter map* to *masked*.
2. Clear the data associated with the spectrum with detectors that are masked.

Implementation
##############

In the plan, the workflow to mask detectors should be

1. Convert input detectors, workspace indices or spectra, and general :ref:`MatrixWorkspace <MatrixWorkspace>` to a `MaskWorkspace <http://www.mantidproject.org/MaskWorkspace>`__.
2. Mask detectors according to `MaskWorkspace <http://www.mantidproject.org/MaskWorkspace>`__.
3. Clear data on all spectra, which have at least one detector that is masked.

Concern
#######

-  Speed!

Usage
-----

Example 1: specifying spectrum numbers
##########################################

.. testcode:: ExMaskSpec

  import numpy as np

  # Create a workspace containing some data.
  ws = CreateSampleWorkspace()
  # Mask two detectors by specifying numbers 1 and 3
  MaskDetectors(ws,SpectraList=[1,3])

  # Check that spectra with spectrum numbers 1 and 3 are masked

  # Get the 1st spectrum in the workspace
  spec = ws.getSpectrum(0)
  detid = spec.getDetectorIDs()[0]
  print 'Spectrum number is',spec.getSpectrumNo()
  print 'Detector of this spectrum is masked:',ws.getInstrument().getDetector(detid).isMasked()
  y = ws.readY(0)
  print 'All counts in the spectrum are 0:   ',np.all( y == 0.0 )

  # Get the 2nd spectrum in the workspace
  spec = ws.getSpectrum(1)
  detid = spec.getDetectorIDs()[0]
  print 'Spectrum number is',spec.getSpectrumNo()
  print 'Detector of this spectrum is masked:',ws.getInstrument().getDetector(detid).isMasked()
  y = ws.readY(1)
  print 'All counts in the spectrum are 0:   ',np.all( y == 0.0 )

  # Get the 3rd spectrum in the workspace
  spec = ws.getSpectrum(2)
  detid = spec.getDetectorIDs()[0]
  print 'Spectrum number is',spec.getSpectrumNo()
  print 'Detector of this spectrum is masked:',ws.getInstrument().getDetector(detid).isMasked()
  y = ws.readY(2)
  print 'All counts in the spectrum are 0:   ',np.all( y == 0.0 )

  # Get the 4th spectrum in the workspace
  spec = ws.getSpectrum(3)
  detid = spec.getDetectorIDs()[0]
  print 'Spectrum number is',spec.getSpectrumNo()
  print 'Detector of this spectrum is masked:',ws.getInstrument().getDetector(detid).isMasked()
  y = ws.readY(3)
  print 'All counts in the spectrum are 0:   ',np.all( y == 0.0 )

Output
^^^^^^

.. testoutput:: ExMaskSpec

  Spectrum number is 1
  Detector of this spectrum is masked: True
  All counts in the spectrum are 0:    True
  Spectrum number is 2
  Detector of this spectrum is masked: False
  All counts in the spectrum are 0:    False
  Spectrum number is 3
  Detector of this spectrum is masked: True
  All counts in the spectrum are 0:    True
  Spectrum number is 4
  Detector of this spectrum is masked: False
  All counts in the spectrum are 0:    False


Example 2: specifying detector IDs
######################################

.. testcode:: ExMaskIDs

  # Create a workspace containing some data.
  ws = CreateSampleWorkspace()
  # Mask two detectors by specifying detector IDs 101 and 103
  MaskDetectors(ws,DetectorList=[101,103])

  # Check that spectra with spectrum numbers 1 and 3 are masked

  # Check the 1st detector
  det = ws.getInstrument().getDetector(101)
  print 'Detector ',det.getID(),' is masked:',det.isMasked()

  # Check the 2nd detector
  det = ws.getInstrument().getDetector(103)
  print 'Detector ',det.getID(),' is masked:',det.isMasked()

  # Check some other detectors
  det = ws.getInstrument().getDetector(100)
  print 'Detector ',det.getID(),' is masked:',det.isMasked()
  det = ws.getInstrument().getDetector(102)
  print 'Detector ',det.getID(),' is masked:',det.isMasked()
  det = ws.getInstrument().getDetector(105)
  print 'Detector ',det.getID(),' is masked:',det.isMasked()

Output
^^^^^^

.. testoutput:: ExMaskIDs

  Detector  101  is masked: True
  Detector  103  is masked: True
  Detector  100  is masked: False
  Detector  102  is masked: False
  Detector  105  is masked: False


Example 3: specifying workspace indices
###########################################

.. testcode:: ExMaskWI

  # Create a workspace containing some data.
  ws = CreateSampleWorkspace()
  # Mask two detectors by specifying workspace indices 0 and 2
  MaskDetectors(ws,WorkspaceIndexList=[0,2])

  # Check that spectra with workspace indices 0 and 2 are masked

  # Check the 1st spectrum
  workspaceIndex = 0
  det = ws.getDetector( workspaceIndex )
  print 'Detector in spectrum with workspace index ',workspaceIndex,' is masked:',det.isMasked()

  # Check the 2nd spectrum
  workspaceIndex = 2
  det = ws.getDetector( workspaceIndex )
  print 'Detector in spectrum with workspace index ',workspaceIndex,' is masked:',det.isMasked()

  # Check some other spectra
  workspaceIndex = 1
  det = ws.getDetector( workspaceIndex )
  print 'Detector in spectrum with workspace index ',workspaceIndex,' is masked:',det.isMasked()
  workspaceIndex = 3
  det = ws.getDetector( workspaceIndex )
  print 'Detector in spectrum with workspace index ',workspaceIndex,' is masked:',det.isMasked()
  workspaceIndex = 4
  det = ws.getDetector( workspaceIndex )
  print 'Detector in spectrum with workspace index ',workspaceIndex,' is masked:',det.isMasked()

Output
^^^^^^

.. testoutput:: ExMaskWI

  Detector in spectrum with workspace index  0  is masked: True
  Detector in spectrum with workspace index  2  is masked: True
  Detector in spectrum with workspace index  1  is masked: False
  Detector in spectrum with workspace index  3  is masked: False
  Detector in spectrum with workspace index  4  is masked: False


Example 4: specifying a masking workspace
##################################################

.. testcode:: ExMaskMask

  # Create a masking workspace

  # Create a intermediate workspace to help create the masking workspace
  tmp = CreateSampleWorkspace()
  # Mask two detectors
  MaskDetectors(tmp,WorkspaceIndexList=[1,3])
  # Extract created mask into specialised masking workspace
  masking_ws,dummy = ExtractMask( tmp )

  print 'A masking workspace has',masking_ws.blocksize(),'spectrum'
  print 'Unmasked spectrum, value=',masking_ws.readY(0)[0]
  print 'Masked spectrum,   value=',masking_ws.readY(1)[0]
  print 'Unmasked spectrum, value=',masking_ws.readY(2)[0]
  print 'Masked spectrum,   value=',masking_ws.readY(3)[0]
  print 'Unmasked spectrum, value=',masking_ws.readY(4)[0]
  print

  # Create a data workspace
  ws = CreateSampleWorkspace()
  # Mask it using the mask in masking_ws
  MaskDetectors(ws, MaskedWorkspace=masking_ws)

  # Check masking of first 5 detectors
  det = ws.getDetector(0)
  print 'Detector',det.getID(),'is masked:',det.isMasked()
  det = ws.getDetector(1)
  print 'Detector',det.getID(),'is masked:',det.isMasked()
  det = ws.getDetector(2)
  print 'Detector',det.getID(),'is masked:',det.isMasked()
  det = ws.getDetector(3)
  print 'Detector',det.getID(),'is masked:',det.isMasked()
  det = ws.getDetector(4)
  print 'Detector',det.getID(),'is masked:',det.isMasked()


Output
^^^^^^

.. testoutput:: ExMaskMask

  A masking workspace has 1 spectrum
  Unmasked spectrum, value= 0.0
  Masked spectrum,   value= 1.0
  Unmasked spectrum, value= 0.0
  Masked spectrum,   value= 1.0
  Unmasked spectrum, value= 0.0

  Detector 100 is masked: False
  Detector 101 is masked: True
  Detector 102 is masked: False
  Detector 103 is masked: True
  Detector 104 is masked: False

.. categories::
