.. algorithm::

.. summary::

.. alias::

.. properties::

Introduction
------------

To understand the algorithms options, user should clearly understand the difference between *WorkspaceIndex* 
-- the numbers, specified in *WorkspaceIndexList* and *StartWorkspacIndex*, *EndWorkspaceIndex* properties,
the *Spectra Number* or according to other terminology *Spectra ID* -- values of the **SpectraList** property and *Detector ID* -- the numbers to provide for 
*DetectorList* property.

The *WorkspaceIndex* is the number a spectrum has in a workspace, e.g. ::

  sp = ws.getSpectrum(0) 

always returns first spectra present in the workspace.

The *Spectra Number* or  *spectra ID* mean the number, assigned to a spectrum. This number is often equal to *WorkspaceIndex+1*, e.g. ::

  print sp.getSpectrumNo() 

from the sample above will often print 1 but not always. The simplest case when this 
number is different is when you load a second half of a workspace, when the first spectrum number still is **NumTotalSpectraInWorkspace/2+1**,
while *WorkspaceIndex* of this spectra becomes 0, i.e.: ::

	sp = ws.getSpectrum(0)
	print sp.getSpectrumNo()
	
prints number equal to **NumTotalSpectraInWorkspace/2+1**. There are other ways to assign a random number to a spectrum. 

And finally, the *detector ID* is the number assigned to a detector in an instrument definition file. Sometimes, 
a  first detector corresponds to the first spectra of a workspace, but it is not in any way certain. For example
the code: ::

  ws = CreateSimulationWorkspace('MARI','-0.5,0.5,0.5')
  sp=ws.getSpectrum(1)
  print sp.getSpectrumNo(), sp.getDetectorIDs()

Will print: ::

  2 set(1102)
	
but any *ISIS MARI* workspace obtained from experiment will produce different sequence, e.g. something like: ::

  5 set(4101)

  
Description
-----------

The algorithm zeroes the data in the spectra of the input workspace 
defined as masked. The detectors corresponding to the masked spectra are also
flagged as masked (can be verified by the `IDetector::isMasked()` method).

The *Workspace* property specifies the workspace to mask while the other properties
provide various ways to define the spectra and detectors to mask.

If *Workspace* is PeaksWorkspace, only the detectors listed are masked and 
the mask must be specified by a *DetectorList* or *MaskedWorkspace*.

All but the *Workspace* property are optional and at least one of them must be
set. If several are set, the combination of them is used.

The set of spectra and detectors to be masked can be given as a list of either
spectrum numbers, detector IDs, workspace indices, component names, or as a
workspace index range.

The workspace index range (properties *StartWorkspacIndex* and *EndWorkspaceIndex*)
changes its action depending on other masking properties being provided, namely:

- If a workspace index range is provided alone, all spectra within this range are masked.
- If a workspace index range is provided in combination with any other masking
  property, only the indexes in this range are masked.

Mask Detectors According To Instrument & Masking Workspace
##########################################################

If *MaskedWorkspace* is provided, both *MaskedWorkspace* and 
*Workspace* mask have the same instrument. 

The algorithm works differently depending on *MaskedWorkspace* property 
being a *Mask Workspace* (SpecialWorkspace2D object) or 
`Matrix Workspace <http://docs.mantidproject.org/nightly/concepts/MatrixWorkspace.html#matrixworkspace>`_. 

If source *MaskedWorkspace* is a *Mask Workspace* and the number of spectra in the source 
*MaskedWorkspace* is equal to number of spectra in the target *Workspace*, the 
spectra numbers of the *MaskedWorkspace* are used as source 
of masking information for the target workspace. 

If the numbers of spectra in *Workspace* and *MaskedWorkspace* are different,
the algorithm extracts list of masked detector IDS from source workspace and
uses them to mask the corresponding spectra of the target workspace. 

Setting property *ForceInstrumentMasking* to true forces algorithm 
to always use *MaskedWorkspace* detector IDs
as the source of the masking information. 
If a detector is masked, then the corresponding detector
will be masked in the input *Workspace*.

If the input *MaskedWorkspace* is a `Matrix Workspace <http://docs.mantidproject.org/nightly/concepts/MatrixWorkspace.html#matrixworkspace>`_ 
and the number of spectra in the source *MaskedWorkspace* is equal to the number 
of spectra in the target *Workspace*, then workspace indicies of the source are
used.

If the numbers of spectra in *Workspace* and *MaskedWorkspace* are different,
the algorithm extracts list of detector IDS from source workspace and uses them 
to mask the corresponding spectra of the target workspace. 

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
-  If masked workspace with a masked spectrum is applied to a target workspace with grouped detectors, 
   and only one detector in the group of target workspace is masked, all target spectra, 
   containing this detector become masked.
   

About Input Parameters
######################

:ref:`algm-MaskDetectors` supports various format of input to
mask detectors, including

-  Workspace indices
-  Spectrum numbers
-  Detector IDs
-  Instrument components
-  MaskWorkspace
-  General :ref:`MatrixWorkspace <MatrixWorkspace>` other than
   MaskWorkspace (In this case, the mask will be
   extracted from this workspace)
-  Workspace index range specified by setting either *StartWorkspacIndex* or *EndWorkspaceIndex* to non-default value.
   **Note:** Setting *EndWorkspaceIndex* to a value exceeding the number of histograms in the target workspace would mask
   the entire workspace.

Rules
#####

Here are the rules for input information for masking

1. At least one of the masking inputs must be specified.
2. Workspace indices and Spectra cannot be given at the same time.
3. MaskWorkspace  and general :ref:`MatrixWorkspace <MatrixWorkspace>` cannot be given at the same time.
4. When a general :ref:`MatrixWorkspace <MatrixWorkspace>` is specified, then all detectors in a spectrum are treated as masked if the effective detector of that spectrum is masked.
5. The detectors found recursively in given instrument components are added to the list of detectors to mask. If multiple components with the same name exist, the first component found is masked.
6. The masks specified from

   a) workspace indices/spectra
   b) detectors
   c) MaskWorkspace /general :ref:`MatrixWorkspace <MatrixWorkspace>` will be combined by the *plus* operation.

Operations Involved in Masking
##############################

There are 2 operations to mask a detector and thus spectrum related

1. Set the detector in workspace's instrument's *parameter map* to *masked*.
2. Zero the data and clear the events associated with the spectrum with detectors that are masked.


Usage
-----

Example 1: specifying spectrum numbers
######################################

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
##################################

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
#######################################

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


Example 4: specifying instrument components
###########################################

.. testcode:: ExMaskComp

  # Create a workspace containing some data.
  ws = CreateSampleWorkspace()
  # Mask the column of detectors named 'bank1(x=3)' in bank1, and bank2 entirely.
  # Unfortunately, individual detectors cannot be masked this way in the
  # workspace created by CreateSampleWorkspace since their
  # names contain a comma ',' which breaks the parsing of the component list.
  MaskDetectors(ws, ComponentList='bank1/bank1(x=3), bank2')
  
  
  # Define a helper function.
  def checkMasked(detsBegin, detsEnd):
      allMasked = True
      for i in range(detsBegin, detsEnd):
          det = ws.getInstrument().getDetector(i)
          if not det.isMasked():
              allMasked = False
              break
      if allMasked:
          print('Detectors from {0} to {1} are masked.'.format(detsBegin, detsEnd))
      else:
          print('Some detectors were unmasked.')
  
  # Check the detector column in bank1
  checkMasked(130, 140)
  
  # Check bank2
  checkMasked(200,300)

.. testoutput:: ExMaskComp

  Detectors from 130 to 140 are masked.
  Detectors from 200 to 300 are masked.

Example 5: specifying a masking workspace
#########################################

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
  
Example 6: specifying a masking range
#####################################

.. testcode:: ExMaskInRange

  # Create a data workspace
  ws = CreateSampleWorkspace()
  # Mask 3 detectors using the masking range
  MaskDetectors(ws, StartWorkspaceIndex=2, EndWorkspaceIndex=4)  

  # Check masking of first 6 detectors
  for ind in xrange(0,6):
    det = ws.getDetector(ind)
    print 'Detector',det.getID(),'is masked:',det.isMasked()


Output
^^^^^^

.. testoutput:: ExMaskInRange

  Detector 100 is masked: False
  Detector 101 is masked: False
  Detector 102 is masked: True
  Detector 103 is masked: True
  Detector 104 is masked: True
  Detector 105 is masked: False
  
Example 7: constraining the masking range
#########################################

.. testcode:: ExMaskConstrainInRange

  # Create a masking workspace

  # Create a intermediate workspace to help create the masking workspace
  tmp = CreateSampleWorkspace()
  # Mask four detectors:
  MaskDetectors(tmp,StartWorkspaceIndex=2, EndWorkspaceIndex=5)
  # Extract created mask into specialised masking workspace
  masking_ws,_ = ExtractMask( tmp )

  for ind in xrange(0,7):
    val = masking_ws.readY(ind)[0]
    if val>0:
        print 'Unmasked spectrum, value=',val    
    else:
        print 'Masked spectrum,   value=',val
  print

  # Create a data workspace
  ws = CreateSampleWorkspace()
  # Mask it using the mask in masking_ws constraining masking range:
  MaskDetectors(ws, MaskedWorkspace=masking_ws,StartWorkspaceIndex=4, EndWorkspaceIndex=5)

  # Check masking of first 7 detectors
  for ind in xrange(0,7):
    det = ws.getDetector(ind)
    print 'Detector',det.getID(),'is masked:',det.isMasked()

Output
^^^^^^

.. testoutput:: ExMaskConstrainInRange

  Masked spectrum,   value= 0.0
  Masked spectrum,   value= 0.0
  Unmasked spectrum, value= 1.0
  Unmasked spectrum, value= 1.0
  Unmasked spectrum, value= 1.0
  Unmasked spectrum, value= 1.0
  Masked spectrum,   value= 0.0

  Detector 100 is masked: False
  Detector 101 is masked: False
  Detector 102 is masked: False
  Detector 103 is masked: False
  Detector 104 is masked: True
  Detector 105 is masked: True
  Detector 106 is masked: False
    
.. categories::

.. sourcelink::
