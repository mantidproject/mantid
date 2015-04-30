.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm Integrates over the range specified, converts the
spectrum axis into units of Q and Q^{2} and transposes the result
workspaces.

There are two output workspaces.

Subalgorithms used
##################

This algorithm uses the :ref:`algm-Integration`, :ref:`algm-ConvertSpectrumAxis`
and :ref:`algm-Transpose` algorithms.

Usage
-----

.. testcode:: exElasticWindowSimple

  # Prepare a workspace that has all necessary settings to work with ElasticWindow
  ws = CreateSampleWorkspace(Function='User Defined',
                             UserDefinedFunction='name=Lorentzian,Amplitude=100,PeakCentre=27500,FWHM=20',
                             XMin=27000,
                             XMax=28000,
                             BinWidth=10,
                             NumBanks=1)

  ws = ConvertUnits(ws, 'DeltaE',
                    EMode='Indirect',
                    EFixed=1.555)

  ws = Rebin(ws, [-0.2,0.004,0.2])

  SetInstrumentParameter(ws, 'Efixed',
                         DetectorList=range(100,200),
                         ParameterType='Number',
                         Value='1.555')

  # Run the algorithm
  q, q2 = ElasticWindow(ws, -0.1, 0.1)

  print q.getAxis(0).getUnit().caption()
  print q2.getAxis(0).getUnit().caption()

.. testoutput:: exElasticWindowSimple

    q
    Q2

.. categories::
