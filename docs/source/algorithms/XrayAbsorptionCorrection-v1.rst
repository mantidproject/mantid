.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm calculates the correction factors due to the absorption of Xrays 
by sample and applies it to Input Workspace. This is done by determining path of xray from
muon in sample to detector and using the calculated distance in sample to calculate 
correction factor using exp(-absorption coefficient(energy) * distance).


Input Workspace Requirements
############################

The algorithm will compute the correction factors on a bin-by-bin basis for each spectrum within
the input workspace. The following assumptions on the input workspace will are made:

- properties of the sample and optionally its environment have been set with
  :ref:`SetSample <algm-SetSample>`
  
- Xray Attenuation profile data must be provided using :ref:`SetSampleMaterial <algm-SetSampleMaterial>`

- Muon Implantation Profile must have only one spectrum and muon depth should be x data and intensity 
should be y data.


Input Parameters
######

#. Detector Angle, this is the angle between x axis and detector.
#. Detector Distance, this is the distance between sample and detector.
#. Muon Implantation Profile, this is a workspace containing muon implantion profile.
#. Input Workspace,  this is a workspace containing data.


Usage
-----

**Example: A cylindrical sample with no container**

	LoadElementalAnalysisData(Run='2695', GroupWorkspace='ws')
	CreateWorkspace(OutputWorkspace='MuonProfile', DataY = np.arange(1,51), DataX = np.linspace(0.1,0.2))
	
	SetSample(InputWorkspace='ws; Detector 1', 
	Geometry='{"Center":[0.0,0.0,0.0],"Height":2.0,"Radius":1.0,"Shape":"Cylinder"}')
	
	SetSampleMaterial(InputWorkspace='ws; Detector 1', ChemicalFormula='Au', 
	XRayAttenuationProfile='C:/Users/bux39319/Documents/Gold.dat')
	
	XrayAbsorptionCorrections(InputWorkspace='ws; Detector 1', MuonImplantationProfile='MuonProfile',
	OutputWorkspace='corrections')