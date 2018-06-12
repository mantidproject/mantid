.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Takes an input TOF spectrum from :ref:`LoadVesuvio <algm-LoadVesuvio>` and
converts it to Y-space using the
:ref:`ConvertToYSpace <algm-ConvertToYSpace>` algorithm. The result is then
fitted using the ComptonPeakProfile function and the given mass to
produce an estimate of the peak area. The input data is normalised by
this value.

The input workspace is required to be a point data workspace, see
:ref:`ConvertToPointData <algm-ConvertToPointData>`, and each detector is required to have
an instrument parameter named *t0* that specifies the detector delay time in :math:`\mu s`, see
:ref:`SetInstrumentParameter <algm-SetInstrumentParameter>`.

The algorithm has 4 outputs:

-  the input data normalised by the fitted peak area;
-  the input data (without normalisation) converted Y-space;
-  the fitted peak in Y-space;
-  the input data converted to Y and then symmetrised about Y=0.

If the sum option is requested then all input spectra are rebinned, in
steps of 0.5 :math:`\AA^{-1}`, to a common Y grid and then summed to give a
single spectrum.

Usage
-----

**Example - Normalise without summation:**

.. testcode:: NormaliseNoSumOutput

    ###### Simulates LoadVesuvio #################
    tof_ws = CreateSimulationWorkspace(Instrument='Vesuvio',BinParams=[50,0.5,562],UnitX='TOF')
    tof_ws = CropWorkspace(tof_ws,StartWorkspaceIndex=0,EndWorkspaceIndex=4) # index one less than spectrum number
    tof_ws = ConvertToPointData(tof_ws)
    SetInstrumentParameter(tof_ws, ParameterName='t0',ParameterType='Number',Value='0.5')
    SetInstrumentParameter(tof_ws, ParameterName='sigma_l1', ParameterType='Number', Value='0.021')
    SetInstrumentParameter(tof_ws, ParameterName='sigma_l2', ParameterType='Number', Value='0.023')
    SetInstrumentParameter(tof_ws, ParameterName='sigma_tof', ParameterType='Number', Value='0.3')
    SetInstrumentParameter(tof_ws, ParameterName='sigma_theta', ParameterType='Number', Value='0.028')
    SetInstrumentParameter(tof_ws, ParameterName='hwhm_lorentz', ParameterType='Number', Value='24.0')
    SetInstrumentParameter(tof_ws, ParameterName='sigma_gauss', ParameterType='Number', Value='73.0')
    ##############################################

    normalised, yspace, fitted, symmetrised = \
      NormaliseByPeakArea(InputWorkspace=tof_ws, Mass=1.0079,Sum=False)

    print("Number of normalised spectra is: {}".format(normalised.getNumberHistograms()))
    print("Number of Y-space spectra is: {}".format(yspace.getNumberHistograms()))
    print("Number of fitted spectra is: {}".format(fitted.getNumberHistograms()))
    print("Number of symmetrised spectra is: {}".format(symmetrised.getNumberHistograms()))

.. testoutput:: NormaliseNoSumOutput

    Number of normalised spectra is: 5
    Number of Y-space spectra is: 5
    Number of fitted spectra is: 5
    Number of symmetrised spectra is: 5

**Example - Normalise with summation:**

.. testcode:: NormaliseWithSummedOutput

    ###### Simulates LoadVesuvio ################
    tof_ws = CreateSimulationWorkspace(Instrument='Vesuvio',BinParams=[50,0.5,562],UnitX='TOF')
    tof_ws = CropWorkspace(tof_ws,StartWorkspaceIndex=0,EndWorkspaceIndex=4) # index one less than spectrum number
    tof_ws = ConvertToPointData(tof_ws)
    SetInstrumentParameter(tof_ws, ParameterName='t0',ParameterType='Number',Value='0.5')
    SetInstrumentParameter(tof_ws, ParameterName='sigma_l1', ParameterType='Number', Value='0.021')
    SetInstrumentParameter(tof_ws, ParameterName='sigma_l2', ParameterType='Number', Value='0.023')
    SetInstrumentParameter(tof_ws, ParameterName='sigma_tof', ParameterType='Number', Value='0.3')
    SetInstrumentParameter(tof_ws, ParameterName='sigma_theta', ParameterType='Number', Value='0.028')
    SetInstrumentParameter(tof_ws, ParameterName='hwhm_lorentz', ParameterType='Number', Value='24.0')
    SetInstrumentParameter(tof_ws, ParameterName='sigma_gauss', ParameterType='Number', Value='73.0')
    ##############################################

    normalised, yspace, fitted, symmetrised = \
      NormaliseByPeakArea(InputWorkspace=tof_ws, Mass=1.0079,Sum=True)

    print("Number of normalised spectra is: {}".format(normalised.getNumberHistograms()))
    print("Number of Y-space spectra is: {}".format(yspace.getNumberHistograms()))
    print("Number of fitted spectra is: {}".format(fitted.getNumberHistograms()))
    print("Number of symmetrised spectra is: {}".format(symmetrised.getNumberHistograms()))

.. testoutput:: NormaliseWithSummedOutput

    Number of normalised spectra is: 5
    Number of Y-space spectra is: 1
    Number of fitted spectra is: 1
    Number of symmetrised spectra is: 1

.. categories::

.. sourcelink::
