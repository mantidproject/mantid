.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The routine varies the width of the resolution file to give a 'stretch
factor' and the area provides an intensity normalisation factor. The
fitted parameters are in the group workspace with suffix \_ResNorm with
additional suffices of Intensity & Stretch. The fitted data are in the
workspace ending in \_ResNorm\_Fit.

This routine was originally part of the MODES package.

Usage
-----
**Example - a basic example using ResNorm.**

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
res = createSampleWorkspace("irs26173_graphite002_res")

ResNorm(VanNumber='26176',
        ResNumber='26173',
        InputType='Workspace',
        ResInputType='Workspace',
        Instrument='irs',
        Analyser='graphite002',
        Plot='None',
        Version=1)


.. categories::

.. sourcelink::
  :cpp: None
  :h: None
