.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm calculates the correction factors due to the absorption of Xrays
by the sample. This is done by determining path of an xray from a
muon in sample to detector and using the calculated distance travelled by emitted xray in sample to
calculate correction factor using

.. math::

   \exp(-absorptionCoefficient(energy) \cdot distance) .

Input Workspace Requirements
############################

The algorithm will compute the correction factors on a bin-by-bin basis for each spectrum within
the input workspace. The following assumptions on the input workspace are made:

- properties of the sample and optionally its environment have been set with :ref:`SetSample <algm-SetSample>`

- Xray Attenuation profile data is provided by using :ref:`SetSampleMaterial <algm-SetSampleMaterial>`

- Muon Implantation Profile has a single spectrum. The x data is the muon depth in cm and the y data is intensity in counts.

Usage
-----

**Example: A cylindrical sample**

.. testcode:: ExCylinderSample

	CreateWorkspace(OutputWorkspace="Input", DataY=np.arange(0,8001), DataX=np.arange(0,8001))
	CreateWorkspace(OutputWorkspace="MuonProfile", DataY=np.arange(1,51), DataX=np.linspace(0.1,0.2))
	SetSample("Input", Geometry={"Shape": "Cylinder", "Height": 2.0, "Radius": 1.0,
                     "Center": [0.0,0.0,0.0]})
	SetSampleMaterial(InputWorkspace="Input", ChemicalFormula="Au",XRayAttenuationProfile="Gold_Xray_Absorption_Coefficient.dat")
	XrayAbsorptionCorrection(InputWorkspace="Input", MuonImplantationProfile="MuonProfile",OutputWorkspace="corrections")
	xdata = mtd["corrections"].readX(0)
	ydata = mtd["corrections"].readY(0)
	ylen = len(ydata)
	print("The middle 5 correction factors in workspace are:")
	for x in range(5):
		index = int(ylen/2) -3 + x
		print("Energy(Kev) : {0:.4f},Factor : {1:.4f}".format(xdata[index],ydata[index]))


Output:

.. testoutput:: ExCylinderSample

	The middle 5 correction factors in workspace are:
	Energy(Kev) : 3997.0000,Factor : 0.9466
	Energy(Kev) : 3998.0000,Factor : 0.9466
	Energy(Kev) : 3999.0000,Factor : 0.9466
	Energy(Kev) : 4000.0000,Factor : 0.9466
	Energy(Kev) : 4001.0000,Factor : 0.9466

.. categories::

.. sourcelink::
