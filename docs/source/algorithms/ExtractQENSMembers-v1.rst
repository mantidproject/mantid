.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Extracts the fit members from a QENS fit and stores each of them in a group workspace.

Usage
-----

**Example**

.. testcode:: ExExtractQENSMembers

  from __future__ import print_function

  # Load sample and resolution files
  sample = Load('irs26176_graphite002_red.nxs')
  resolution = Load('irs26173_graphite002_red.nxs')

  # Set up fit algorithm parameters
  function = """name=LinearBackground,A0=0,A1=0,ties=(A0=0.000000,A1=0.0);
  (composite=Convolution,FixResolution=true,NumDeriv=true;
  name=Resolution,Workspace=resolution,WorkspaceIndex=0;
  name=Lorentzian,Amplitude=1,PeakCentre=0,FWHM=0.0175)"""
  bgType = "Fixed Flat"
  startX = -0.547608
  endX = 0.543217
  specMin = 0
  specMax = sample.getNumberHistograms() - 1
  convolve = True  # Convolve the fitted model components with the resolution
  minimizer = "Levenberg-Marquardt"
  maxIt = 500
  output_ws_name = "irs26176_graphite002"

  # Run ConvolutionFitSequential algorithm
  result_ws = ConvolutionFitSequential(InputWorkspace=sample, Function=function,
                                       PassWSIndexToFunction=True, BackgroundType=bgType,
                                       StartX=startX, EndX=endX, SpecMin=specMin, SpecMax=specMax,
                                       Convolve=convolve, Minimizer=minimizer, MaxIterations=maxIt,
                                       OutputWorkspace=output_ws_name)

  # Extract members from the output of the ConvolutionFitSequential algorithm
  members_ws = ExtractQENSMembers(InputWorkspace=sample, ResultWorkspace=output_ws_name + "_conv_1LFixF_s0_to_9",
                                  RenameConvolvedMembers=True, ConvolvedMembers=["Lorentzian"])

  for member_ws in members_ws:
      print(member_ws.getName())

.. testcleanup:: ExExtractQENSMembers

  DeleteWorkspace(output_ws_name + "_Workspaces")
  DeleteWorkspace(output_ws_name + "_Parameters")
  DeleteWorkspace(members_ws)
  DeleteWorkspace(result_ws)
  DeleteWorkspace(sample)
  DeleteWorkspace(resolution)

Output:

.. testoutput:: ExExtractQENSMembers

  irs26176_graphite002_Members_Data
  irs26176_graphite002_Members_Calc
  irs26176_graphite002_Members_Fit
  irs26176_graphite002_Members_Resolution
  irs26176_graphite002_Members_Lorentzian

.. testcode:: ExExtractQENSMembersProperty

  from __future__ import print_function

  # Load sample and resolution files
  sample = Load('irs26176_graphite002_red.nxs')
  resolution = Load('irs26173_graphite002_red.nxs')

  # Set up fit algorithm parameters
  function = """name=LinearBackground,A0=0,A1=0,ties=(A0=0.000000,A1=0.0);
  (composite=Convolution,FixResolution=true,NumDeriv=true;
  name=Resolution,Workspace=resolution,WorkspaceIndex=0;
  name=Lorentzian,Amplitude=1,PeakCentre=0,FWHM=0.0175)"""
  bgType = "Fixed Flat"
  startX = -0.547608
  endX = 0.543217
  specMin = 0
  specMax = sample.getNumberHistograms() - 1
  convolve = True  # Convolve the fitted model components with the resolution
  minimizer = "Levenberg-Marquardt"
  maxIt = 500
  output_ws_name = "irs26176_graphite002"

  # Run ConvolutionFitSequential algorithm with ExtractMembers property
  result_ws = ConvolutionFitSequential(InputWorkspace=sample, Function=function,
                                       PassWSIndexToFunction=True, BackgroundType=bgType,
                                       StartX=startX, EndX=endX, SpecMin=specMin, SpecMax=specMax,
                                       Convolve=convolve, Minimizer=minimizer, MaxIterations=maxIt,
                                       ExtractMembers=True, OutputWorkspace=output_ws_name)

  members_ws = mtd[output_ws_name + "_Members"]

  for member_ws in members_ws:
      print(member_ws.getName())

.. testcleanup:: ExExtractQENSMembersProperty

  DeleteWorkspace(output_ws_name + "_Workspaces")
  DeleteWorkspace(output_ws_name + "_Parameters")
  DeleteWorkspace(members_ws)
  DeleteWorkspace(result_ws)
  DeleteWorkspace(sample)
  DeleteWorkspace(resolution)

.. testoutput:: ExExtractQENSMembersProperty

  irs26176_graphite002_Members_Data
  irs26176_graphite002_Members_Calc
  irs26176_graphite002_Members_Fit
  irs26176_graphite002_Members_Resolution
  irs26176_graphite002_Members_Lorentzian
