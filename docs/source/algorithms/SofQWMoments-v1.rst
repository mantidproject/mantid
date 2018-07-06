.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Calculates the :math:`n^{th}` moment :math:`M_n` of :math:`y(Q,w)` where
:math:`M_n` is the integral of :math:`w^n*y(Q,w)` over all w for
:math:`n=0` to 4.

Workflow
--------

.. diagram:: SofQWMoments-v1_wkflw.dot

Usage
-----

**Example - Running SofQWMoments from with an SofQW workspace.**

.. testcode:: ExSofQWMomentsSimple
    
    #create a dummy workspace
    function = "name=Lorentzian,Amplitude=1,PeakCentre=5,FWHM=1"
    ws = CreateSampleWorkspace("Histogram", Function="User Defined", UserDefinedFunction=function, XMin=0, XMax=10, BinWidth=0.01, XUnit="DeltaE")
    ws = ScaleX(ws, -5, "Add") #shift to center on 0
    ws = ScaleX(ws, 0.1) #scale to size
    ws = RenameWorkspace(ws, OutputWorkspace="irs21760_graphite002_red")
    LoadInstrument(ws, InstrumentName='IRIS', RewriteSpectraMap=True)

    #Run SofQW and then SofQWMoments
    ws = SofQW(ws, '0.4, 0.1, 1.8', EMode='Indirect', EFixed='1.845', OutputWorkspace="irs00001_graphite002_red")
    SofQWMoments(ws, OutputWorkspace='Test')

.. categories::

.. sourcelink::
