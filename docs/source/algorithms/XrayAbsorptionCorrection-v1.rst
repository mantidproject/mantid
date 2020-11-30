.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm calculates the correction factors due to the absorption of Xrays 
by sample and applies it to Input Workspace. This is done by determining path of xray from
muon in sample to detector and using the calculated distance in sample to calculate 
correction factor using exp(-absorptioncoefficient(energy) * distance).

Input Workspace Requirements
############################

The algorithm will compute the correction factors on a bin-by-bin basis for each spectrum within
the input workspace. The following assumptions on the input workspace will are made:
   
     - properties of the sample and optionally its environment have been set with :ref:`SetSample <algm-SetSample>`
	 
     - Xray Attenuation profile data must be provided using :ref:`SetSampleMaterial <algm-SetSampleMaterial>`

     - Muon Implantation Profile must have only one spectrum and muon depth should be x data and intensity should be y data.
   
Input Parameters
################

#. Detector Angle, this is the angle between detector.

#. Detector Distance, this is the distance between sample and detector.

#. Muon Implantation Profile, this is a workspace containing muon implantion profile.

#. Input Workspace,  this is a workspace containing data.

Usage
-----

**Example: A cylindrical sample **

.. testcode:: ExCylinderSample

	CreateWorkspace(OutputWorkspace='Input', DataY = np.arange(0,8001), DataX = np.arange(0,8001))
	CreateWorkspace(OutputWorkspace='MuonProfile', DataY = np.arange(1,51), DataX = np.linspace(0.1,0.2))
	SetSample(InputWorkspace='Input', Geometry='{"Center":[0.0,0.0,0.0],"Height":2.0,"Radius":1.0,"Shape":"Cylinder"}')
	SetSampleMaterial(InputWorkspace='Input', ChemicalFormula='Au',XRayAttenuationProfile=path_to_gold_absorption_correction)
	XrayAbsorptionCorrection(InputWorkspace='Input', MuonImplantationProfile='MuonProfile',OutputWorkspace='corrections')
	xdata = mtd['corrections'].readX(0)
	ydata = mtd['corrections'].readY(0)
	ylen = len(ydata)
	print('The middle 5 correction factors in workspace are:')
	for x in range(5):
		index = int(ylen/2) -3 + x
		print("Energy(Kev) : {0:.4f},Factor : {1:.4f}".format(xdata[index],ydata[index]))

		
Output:

.. testoutput:: ExCylinderSample

	The middle 5 correction factors in workspace are:
	Energy(Kev) : 3997.0000,Factor : 0.9418
	Energy(Kev) : 3998.0000,Factor : 0.9418
	Energy(Kev) : 3999.0000,Factor : 0.9418
	Energy(Kev) : 4000.0000,Factor : 0.9418
	Energy(Kev) : 4001.0000,Factor : 0.9418

.. categories::

.. sourcelink::
