.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Calculates Multiple Scattering based on the Monte Carlo program MINUS.
It calculates :math:`S(Q,w)` from specified functions (such as those
used in JumpFit) and supports both Flat and Cylindrical geometries. More
information on the multiple scattering can be procedure can be found in
the `modes
manual <http://www.isis.stfc.ac.uk/instruments/iris/data-analysis/modes-v3-user-guide-6962.pdf>`__.

References
##########

#. M W Johnson, AERE Report R7682 (1974)


Usage
-----
**Example - a basic example using MuscatFunc.**

.. code-block:: python

		def createSampleWorkspace(name, random=False):
			""" Creates a sample workspace with a single lorentzian that looks like IRIS data"""
			import os
			function = "name=Lorentzian,Amplitude=8,PeakCentre=5,FWHM=0.7"
			ws = CreateSampleWorkspace("Histogram", Function="User Defined", UserDefinedFunction=function, XUnit="DeltaE", Random=True, XMin=0, XMax=10, BinWidth=0.01)
			ws = CropWorkspace(ws, StartWorkspaceIndex=0, EndWorkspaceIndex=9)
			ws = ScaleX(ws, -5, "Add")
			ws = ScaleX(ws, 0.1, "Multiply")

			#load instrument and instrument parameters
			LoadInstrument(ws, InstrumentName='IRIS', RewriteSpectraMap=True)
			path = os.path.join(config['instrumentDefinition.directory'], 'IRIS_graphite_002_Parameters.xml')
			LoadParameterFile(ws, Filename=path)
			ws = RenameWorkspace(ws, OutputWorkspace=name)
			return ws


		ws = createSampleWorkspace("irs26173_graphite002_red", random=True)
		SaveNexus(ws, "irs26173_graphite002_red.nxs")

		MuscatFunc(SamNumber='26173', Thick='0.5', Width='0.5', Instrument='irs')


.. categories::

.. sourcelink::
