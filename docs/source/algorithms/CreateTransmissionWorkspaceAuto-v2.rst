.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm is a facade over :ref:`algm-CreateTransmissionWorkspace`. It pulls numeric parameters
out of the instrument parameter file where possible. You can override any of these automatically applied
defaults by providing your own values.

The first transmission run is the only mandatory property. If :literal:`ProcessingInstructions` is not set its value is inferred from other properties:

* If :literal:`AnalysisMode = PointDetectorAnalaysis` and :literal:`PointDetectorStart = PointDetectorStop` then :literal:`ProcessingInstructions` is set to :literal:`PointDetectorStart`.
* If :literal:`AnalysisMode = PointDetectorAnalaysis` and :literal:`PointDetectorStart != PointDetectorStop` then :literal:`ProcessingInstructions` is set to :literal:`PointDetectorStart:PointDetectorStop`.
* If :literal:`AnalysisMode = MultiDetectorAnalaysis` then the spectrum range from :literal:`MultiDetectorStart` to the last spectrum in the input workspace is used.

When :literal:`WavelengthMin` and/or :literal:`WavelengthMax` are not provided, the algorithm will
search for :literal:`LambdaMin` and :literal:`LambdaMax` in the parameter file. If monitor properties,
:literal:`I0MonitorIndex`, :literal:`MonitorBackgroundWavelengthMin`, :literal:`MonitorBackgroundWavelengthMax`,
:literal:`MonitorIntegrationWavelengthMin` and :literal:`MonitorIntegrationWavelengthMax`, are not set, the
algorithm will search for :literal:`I0MonitorIndex`, :literal:`MonitorBackgroundMin`, :literal:`MonitorBackgroundMax`,
:literal:`MonitorIntegralMin` and :literal:`MonitorIntegralMax` respectively.

Finally, if a second transmission run is given, the algorithm will run :ref:`algm-CreateTransmissionWorkspace` using
:literal:`StartOverlap`, :literal:`EndOverlap` and :literal:`Params` if those have been set.

See :ref:`algm-CreateTransmissionWorkspace` for more information on the wrapped algorithm.

Previous Versions
-----------------

This is version 2 of the algorithm. For version 1, please see `CreateTransmissionWorkspaceAuto-v1. <CreateTransmissionWorkspaceAuto-v1.html>`_

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Create a transmission run**

.. testcode:: ExCreateTransWSAutoSimple

    # Create a transmission run with input properties read from the parameter file
    trans = Load(Filename='INTER00013463.nxs')
    transWS = CreateTransmissionWorkspaceAuto(FirstTransmissionRun=trans)

    print "%.4f" % (transWS.readY(0)[26])
    print "%.4f" % (transWS.readY(0)[27])
    print "%.4f" % (transWS.readY(0)[28])
    print "%.4f" % (transWS.readY(0)[29])

Output:

.. testoutput:: ExCreateTransWSAutoSimple

    0.0827
    0.0859
    0.0861
    0.0869

    
**Example - Create a transmission run, overriding some default parameters**

.. testcode:: ExCreateTransWSAutoOverload

    # Override the default values for MonitorBackgroundWavelengthMin and MonitorBackgroundWavelengthMax
    trans = Load(Filename='INTER00013463.nxs')
    transWS = CreateTransmissionWorkspaceAuto(FirstTransmissionRun=trans, MonitorBackgroundWavelengthMin=0.0, MonitorBackgroundWavelengthMax=1.0)

    print "%.3f" % (transWS.readY(0)[26])
    print "%.3f" % (transWS.readY(0)[27])
    print "%.3f" % (transWS.readY(0)[28])
    print "%.3f" % (transWS.readY(0)[29])

Output:

.. testoutput:: ExCreateTransWSAutoOverload

    0.083
    0.086
    0.086
    0.087

    
**Example - Create a transmission run from two runs**

.. testcode:: ExCreateTransWSAutoTwo

    trans1 = Load(Filename='INTER00013463.nxs')
    trans2 = Load(Filename='INTER00013464.nxs')
    transWS = CreateTransmissionWorkspaceAuto(FirstTransmissionRun=trans1, SecondTransmissionRun=trans2, Params=[1.5,0.02,17], StartOverlap=10.0, EndOverlap=12.0)

    print "%.4f" % (transWS.readY(0)[26])
    print "%.4f" % (transWS.readY(0)[27])
    print "%.4f" % (transWS.readY(0)[28])
    print "%.4f" % (transWS.readY(0)[29])

Output:

.. testoutput:: ExCreateTransWSAutoTwo

    0.0816
    0.0841
    0.0857
    0.0862

.. categories::

.. sourcelink::
