.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

*Logarithm* function calculates the logarithm of the data, held in a
workspace and tries to estimate the errors of this data, by calculating
logarithmic transformation of the errors. The errors are assumed to be
small and Gaussian so they are calculated on the basis of Tailor
decomposition e.g. if :math:`S` and :math:`Err` are the signal and
errors for the initial signal, the logarithm would provide
:math:`S_{ln}=ln(S)` and :math:`Err_{ln}=Err/S` accordingly. If the base
10 logarithm is used the errors are calculated as
:math:`Err_{log10}=0.434Err/S`

Some values in a workspace can normally be equal to zero. Logarithm is
not calculated for values which are less or equal to 0, but the value of
*Filler* is used instead. The errors for such cells set to zeros

When acting on an event workspace, the output will be a Workspace2D,
with the default binning from the original workspace.

Usage
-----

**Example - logarithm of a histogram workspace:**

.. testcode:: ExLogarithm

    dataX = list(range(0,10))*10 # define 10 x-spectra
    dataY =([1]*10)*10     # with values 1
    dataY[0]=-10           # make first value not suitable for logarithm
    dataY[1]=10            # make second value different
    dataE =([1]*10)*10     # define 10 error spectra with value 1
    # create test workspace
    ws = CreateWorkspace(dataX, dataY, dataE, NSpec=10)
    # Calculate log10
    ws = Logarithm(ws,Filler=-1,Natural='0')
    #
    # check results:

    print('Log10 for spectra 0:  {}'.format(ws.readY(0)))
    print('Log10 for Err spectra 0:  {}'.format(ws.readE(0)[0:4]))

.. testcleanup:: ExLogarithm

   DeleteWorkspace(ws)

**Output:**

.. testoutput:: ExLogarithm

   Log10 for spectra 0:  [-1.  1.  0.  0.  0.  0.  0.  0.  0.  0.]
   Log10 for Err spectra 0:  [0.     0.0434 0.434  0.434 ]

.. categories::

.. sourcelink::
