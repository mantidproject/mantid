.. _func-CrystalFieldFunction:

====================
CrystalFieldFunction
====================

.. index:: CrystalFieldFunction


Description
-----------

This function calculates spectra of a crystal electric field acting upon one or more rare earth ions. It is a part of crystal field computation
in Mantid and under active development. More documentation will follow as the development progresses.

Here is an example of how to fit function's parameters to spectra.

.. code::

    import numpy as np

    # This creates a (empty) workspace to use with EvaluateFunction
    x = np.linspace(0, 50, 100)
    y = x * 0
    ws = CreateWorkspace(x, y)

    # Define two single site multi-spectrum functions to generate some data to fit to
    fun1 = "name=CrystalFieldMultiSpectrum,Ion=Ce,Temperatures=(4, 10), FWHMs=2,Background=\"name=LinearBackground,A0=20,A1=-0.11\","\
           "ToleranceIntensity=0.001,B20=0.37737,B22=3.9770,B40=-0.031787,B42=-0.11611,B44=-0.12544,f1.f0.A0=10,f1.f0.A1=0.09"
           
    fun2 = "name=CrystalFieldMultiSpectrum,Ion=Pr, Symmetry=D4h,Temperatures=(4, 10), FWHMs=2,"\
           "ToleranceIntensity=0.001,B20=0.4268, B40=0.001031, B44=-0.01996, B60=0.00005, B64=0.001563"

    # Generate the data
    EvaluateFunction(fun1 + ';' + fun2, InputWorkspace=ws, InputWorkspace_1=ws, OutputWorkspace='data')

    cf = "name=CrystalFieldFunction,Ions=(Ce, Pr),Symmetries=(C2v, D4h), FixAllPeaks=1,"\
         "Temperatures=(4, 10),FWHMs=2.0,ToleranceIntensity=0.001,Background=\"name=LinearBackground,A0=20,\","\
         "ion0.B20=0.37737,ion0.B22=3.9770,ion0.B40=-0.031787,ion0.B42=-0.11611,ion0.B44=-0.12544,"\
         "ion1.B20=0.4268, ion1.B40=0.001031, ion1.B44=-0.01996, ion1.B60=0.00005, ion1.B64=0.001563,"\
         "ties=(ion0.BmolX=0,ion0.BmolY=0,ion0.BmolZ=0,ion0.BextX=0,ion0.BextY=0,ion0.BextZ=0, ion0.B60=0,ion0.B62=0,ion0.B64=0,ion0.B66=0, ion0.IntensityScaling=1),"\
         "ties=(ion1.BmolX=0,ion1.BmolY=0,ion1.BmolZ=0,ion1.BextX=0,ion1.BextY=0,ion1.BextZ=0, ion1.IntensityScaling=1),"\
         "ties=(sp0.IntensityScaling=1, sp1.IntensityScaling=1),ties=(sp1.bg.A1 = -sp0.bg.A1)"

    Fit(cf, InputWorkspace='Workspace_0', WorkspaceIndex=1, InputWorkspace_1='Workspace_1', WorkspaceIndex_1=1, Output='fit')


.. categories::

.. sourcelink::
