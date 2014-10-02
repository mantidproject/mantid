.. _func-TabulatedFunction:

=================
TabulatedFunction
=================

.. index:: TabulatedFunction

Description
-----------

A function which takes its values from a file or a workspace. The values are tabulated as
x,y pairs. Liear interpolation is used for points between the tabulated values. The function
returns zero for points outside the tabulated values.

The files can be either ascii text files or nexus files. The ascii files must contain two column
of real numbers separated by spaces. The first column are the x-values and the second one is for y.

If a nexus file is used its first spectrum provides the data for the function. The same is true for 
a workspace which must be a MatrixWorkspace.

.. attributes::

.. properties::

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Fit two structure factors that are shifted and scaled with respect to each other:**

.. testcode:: Ex

    ws1=LoadNexus('tabulatedFunctionExample.nxs')
    
    # Clone the workspace by rescaling and shift 
    ws2=CloneWorkspace(ws1)
    ws2=Scale(ws2, Factor=1.34, Operation='Multiply')
    ws2=ScaleX(ws2, Factor=0.002, Operation='Add')
    
    # Call the Fit algorithm and perform the fit
    myFunc='name=TabulatedFunction,Workspace=ws1,WorkspaceIndex=0,Scaling=1.0,Shift=0.0'
    fitStatus, chiSq, covarianceTable, paramTable, fitWorkspace =\
    Fit(Function=myFunc, InputWorkspace=ws2, Output='fit')

    print "The fit was: " + fitStatus
    print("Fitted Scaling value is: %.2f" % paramTable.column(1)[0])
    print("Fitted Shift value is: %.3f" % abs(paramTable.column(1)[1]))
    # fitWorkspace contains the data, the calculated and the difference patterns
    print "Number of spectra in fitWorkspace is: " +  str(fitWorkspace.getNumberHistograms())

Output:

.. testoutput:: Ex

    The fit was: success
    Fitted Scaling value is: 1.34
    Fitted Shift value is: 0.002
    Number of spectra in fitWorkspace is: 3

.. categories::
