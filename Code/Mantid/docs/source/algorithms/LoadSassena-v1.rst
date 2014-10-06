.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The Sassena application `1 <http://sassena.org>`__ generates
intermediate scattering factors from molecular dynamics trajectories.
This algorithm reads Sassena output and stores all data in workspaces of
type :ref:`Workspace2D <Workspace2D>`, grouped under a single
:ref:`WorkspaceGroup <WorkspaceGroup>`.

Sassena ouput files are in HDF5 format
`2 <http://www.hdfgroup.org/HDF5>`__, and can be made up of the
following datasets: *qvectors*, *fq*, *fq0*, *fq2*, and *fqt*

Time units: Current Sassena version does not specify the time unit, thus
the user is required to enter the time in between consecutive data
points. Enter the number of picoseconds separating consecutive
datapoints.

The workspace for **qvectors**:

-  X-values for the origin of the vector, default: (0,0,0)
-  Y-values for the tip of the vector
-  one spectra with three bins for each q-vector, one bin per vector
   component. If orientational average was performed by Sassena, then
   only the first component is non-zero.

The workspaces for **fq**, **fq0**, and **fq2** contains two spectra:

-  First spectrum is the real part, second spectrum is the imaginary
   part
-  X-values contain the moduli of the q vector
-  Y-values contain the structure factors

Dataset **fqt** is split into two workspaces, one for the real part and
the other for the imaginary part. The structure of these two workspaces
is the same:

-  X-values contain the time variable
-  Y-values contain the structure factors
-  one spectra for each q-vector

Usage
-----

.. include:: ../usagedata-note.txt

.. testcode:: Ex

    ws = LoadSassena("loadSassenaExample.h5", TimeUnit=1.0)
    print 'workspaces instantiated: ', ', '.join(ws.getNames())
    fqtReal = ws[1] # Real part of F(Q,t)
    # Let's fit it to a Gaussian. We start with an initial guess
    intensity = 0.5
    center = 0.0
    sigma = 200.0
    startX = -900.0
    endX = 900.0 
    myFunc = 'name=Gaussian,Height={0},PeakCentre={1},Sigma={2}'.format(intensity,center,sigma)

    # Call the Fit algorithm and perform the fit
    fitStatus, chiSq, covarianceTable, paramTable, fitWorkspace =\
    Fit(Function=myFunc, InputWorkspace=fqtReal, WorkspaceIndex=0, StartX = startX, EndX=endX, Output='fit')

    print "The fit was: " + fitStatus
    print("Fitted Height value is: %.2f" % paramTable.column(1)[0])
    print("Fitted centre value is: %.2f" % abs(paramTable.column(1)[1]))
    print("Fitted sigma value is: %.1f" % paramTable.column(1)[2])
    # fitWorkspace contains the data, the calculated and the difference patterns
    print "Number of spectra in fitWorkspace is: " +  str(fitWorkspace.getNumberHistograms())
    print("The 989th y-value of the fitted curve: %.3f" % fitWorkspace.readY(1)[989])

Output:

.. testoutput:: Ex

    workspaces instantiated:  ws_qvectors, ws_fqt.Re, ws_fqt.Im
    The fit was: success
    Fitted Height value is: 1.00
    Fitted centre value is: 0.00
    Fitted sigma value is: 100.0
    Number of spectra in fitWorkspace is: 3
    The 989th y-value of the fitted curve: 0.673

.. categories::
