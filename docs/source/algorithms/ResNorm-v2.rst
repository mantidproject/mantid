.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm fits the variation of the width of the resolution file to give a
stretch factor and fits the peak intensities by normalising the peak area to
unity and performing a linear fit with the vanadium.

The outpout workspace is a WorkspaceGroup containing two MatrixWorkspaces named
*_Intensity* and *_Stretch* with the fitted parameters.

Workflow
--------

.. diagram:: ResNorm-v2_wkflw.dot

Usage
-----

**Example - a basic example using ResNorm.**

.. testcode:: ExIRISResNorm
    
    def createSampleWorkspace(name, num_spec=10, random=False):
      """
      Creates a sample workspace with a single lorentzian that looks like IRIS data
      """
      import os

      # Create sample data
      function = "name=Lorentzian,Amplitude=8,PeakCentre=5,FWHM=0.7"
      ws = CreateSampleWorkspace("Histogram",
                                 Function="User Defined",
                                 UserDefinedFunction=function,
                                 XUnit="DeltaE",
                                 Random=random,
                                 XMin=0,
                                 XMax=10,
                                 BinWidth=0.01)

      # Reduce number of spectra
      ws = CropWorkspace(ws,
                         StartWorkspaceIndex=0,
                         EndWorkspaceIndex=num_spec-1)

      ws = ScaleX(ws, -5, "Add")
      ws = ScaleX(ws, 0.1, "Multiply")

      # Load instrument and instrument parameters
      LoadInstrument(ws, InstrumentName='IRIS', RewriteSpectraMap=True)
      path = os.path.join(config['instrumentDefinition.directory'], 'IRIS_graphite_002_Parameters.xml')
      LoadParameterFile(ws, Filename=path)
      ws = RenameWorkspace(ws, OutputWorkspace=name)

      return ws


    van = createSampleWorkspace("irs26173_graphite002_red", random=True)
    res = createSampleWorkspace("irs26173_graphite002_res", num_spec=1)

    res_norm = ResNorm(ResolutionWorkspace=res,
                       VanadiumWorkspace=van,
                       Version=2)


.. categories::

.. sourcelink::
