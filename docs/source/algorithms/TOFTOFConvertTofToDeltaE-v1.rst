.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------


Converts X-axis units of the given workspace or group of workspaces from time-of-flight to energy transfer. Conversion is performed in a following way. The new X-axis data (:math:`\Delta E`, meV) are calculated as

:math:`\Delta E = \frac{1}{2} C m_n L^2\cdot\left(\frac{1}{t_{el}^2} - \frac{1}{t^2}\right)`

where :math:`L` is the sample-detector distance, :math:`m_n` is the neutron mass, :math:`t_{el}` is the time-of-flight corresponding to the elastic peak and :math:`t` is the time-of-flight from sample to detector.

:math:`t=t_0 - t_1`

where :math:`t_0` is the total TOF corresponding to the bin boundaries of the X data in the input workspace and :math:`t_1` is the TOF from source to sample. Coefficient :math:`C` is related to the unit conversion and calculated as

:math:`C = \frac{10^{12}\cdot 10^3}{1.6\cdot 10^{-19}}`

where :math:`1.6\cdot 10^{-19}` is the coefficient to convert energy from eV to Joule, :math:`10^3` is the coefficient to convert eV to meV and :math:`10^{12}` is the coefficient to convert :math:`\mu\mathrm{sec}^2` to :math:`\mathrm{seconds}^2`.

In contrast to the :ref:`algm-ConvertUnits`, this algorithm calculates the new Y values :math:`I_E(\Delta E)` as 

:math:`I_E (\Delta E) = \frac{t_c^3}{C\cdot m_n\cdot L^2\cdot\Delta t}\cdot I(t)`

where :math:`t_c` is the time-of-flight corresponding to the bin centre in the X data in the input workspace, :math:`\Delta t` is the time channel width, and :math:`I(t)` are the Y data in the given input workspace.


This algorithm offers two options for calculation of the elastic peak position :math:`t_{el}`:

    1. If *EPPTable* is not given,  position of the elastic peak will be taken from the *EPP* sample log.
           
    2. If *EPPTable* is given, position of the elastic peak will be taken from the *PeakCentre* column.
           

If position of the elastic peak cannot be determined or :math:`t_{el} = 0` for a particular detector, this detector will be masked in the output workspace and warning will be produced. 

Restrictions on the input workspaces
####################################

-  The unit of the X-axis must be **Time-of-flight**.

-  Workspace must contain following sample logs: *channel_width*, *EPP*, *TOF1*.

-  Workpace must have an instrument set.

-  If table *EPPTable* is given:

    1. number of rows of the table must match to the number of histograms of the input workspace.

    2. table must have the *PeakCentre* column.

.. note::
    The input *EPPTable* can be produced using the :ref:`algm-FindEPP` algorithm.

Usage
-----

**Example 1: Convert using the default option.**

.. testcode:: ExTOFTOFConvertTofToDeltaE
    
    import numpy

    # create workspace with appropriate sample logs
    ws_tof = CreateSampleWorkspace(Function="User Defined", UserDefinedFunction="name=LinearBackground, \
                A0=0.3;name=Gaussian, PeakCentre=8000, Height=5, Sigma=75", NumBanks=2, BankPixelWidth=1,
                XMin=6005.75, XMax=9995.75, BinWidth=10.5, BankDistanceFromSample=4.0, SourceDistanceFromSample=1.4)

    lognames="channel_width,EPP,TOF1"
    logvalues="10.5,190.0,2123.34"
    AddSampleLogMultiple(ws_tof, lognames, logvalues)

    ws_dE=TOFTOFConvertTofToDeltaE(ws_tof)

    print "Unit of X-axis before conversion: ", ws_tof.getAxis(0).getUnit().unitID()
    print "Unit of X-axis after conversion: ",  ws_dE.getAxis(0).getUnit().unitID()
    print "First 5 X values before conversion: ", ws_tof.readX(0)[:5]
    print "First 5 X values after conversion: ", numpy.round(ws_dE.readX(0)[:5], 2)

Output:

.. testoutput:: ExTOFTOFConvertTofToDeltaE

    Unit of X-axis before conversion:  TOF
    Unit of X-axis after conversion:  DeltaE
    First 5 X values before conversion:  [ 6005.75  6016.25  6026.75  6037.25  6047.75]
    First 5 X values after conversion:  [-3.13 -3.1  -3.07 -3.04 -3.01]


**Example 2: Convert using the elastic peak positions from table**

.. testcode:: Ex2TOFTOFConvertTofToDeltaE
    
    import numpy

    # create workspace with appropriate sample logs
    ws_tof = CreateSampleWorkspace(Function="User Defined", UserDefinedFunction="name=LinearBackground, \
                A0=0.3;name=Gaussian, PeakCentre=8000, Height=5, Sigma=75", NumBanks=2, BankPixelWidth=1,
                XMin=6005.75, XMax=9995.75, BinWidth=10.5, BankDistanceFromSample=4.0, SourceDistanceFromSample=1.4)

    lognames="channel_width,EPP,TOF1"
    logvalues="10.5,190.0,2100.34"
    AddSampleLogMultiple(ws_tof, lognames, logvalues)

    # create a table with elastic peak positions
    table=FindEPP(ws_tof)
    # run unit conversion
    ws_dE=TOFTOFConvertTofToDeltaE(ws_tof, EPPTable=table)

    print "Unit of X-axis before conversion: ", ws_tof.getAxis(0).getUnit().unitID()
    print "Unit of X-axis after conversion: ",  ws_dE.getAxis(0).getUnit().unitID()
    print "First 5 X values before conversion: ", ws_tof.readX(0)[:5]
    print "First 5 X values after conversion: ", numpy.round(ws_dE.readX(0)[:5], 2)

Output:

.. testoutput:: Ex2TOFTOFConvertTofToDeltaE

    Unit of X-axis before conversion:  TOF
    Unit of X-axis after conversion:  DeltaE
    First 5 X values before conversion:  [ 6005.75  6016.25  6026.75  6037.25  6047.75]
    First 5 X values after conversion:  [-3.08 -3.06 -3.03 -3.   -2.97]

.. categories::

.. sourcelink::
