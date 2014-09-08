.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This is a variation of the stretched exponential option of
`Quasi <http://www.mantidproject.org/IndirectBayes:Quasi>`__. For each spectrum a fit is performed
for a grid of β and σ values. The distribution of goodness of fit values
is plotted.

This routine was originally part of the MODES package. Note that this algorithm
uses F2Py and is currently only supported on windows.

Usage
-----
**Example - a basic example using Quest to fit a reduced workspace.**

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
			LoadInstrument(ws, InstrumentName='IRIS')
			path = os.path.join(config['instrumentDefinition.directory'], 'IRIS_graphite_002_Parameters.xml')
			LoadParameterFile(ws, Filename=path)
			ws = RenameWorkspace(ws, OutputWorkspace=name)
			return ws


		ws = createSampleWorkspace("irs26173_graphite002_red", random=True)
		res = createSampleWorkspace("irs26173_graphite002_res")

		ResNorm(VanNumber='26176', ResNumber='26173', InputType='Workspace', ResInputType='Workspace', Instrument='irs', Analyser='graphite002', Plot='None')
		Quest(SamNumber='26176', ResNumber='26173', ResNormNumber='26176', InputType='Workspace', ResInputType='Workspace', ResNormInputType='Workspace', Instrument='irs', Analyser='graphite002', )

.. categories::
