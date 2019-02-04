.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Remove the prompt pulse tor a time of flight measurement


Usage
-----

.. testcode:: RemovePromptPulse

    #Create a workspace
    from numpy import *
    x=arange(0,100000,10)
    y=0*x+1
    w=CreateWorkspace(x,y[1:],UnitX="TOF")   

    #apply algorithm
    w1=RemovePromptPulse(w,Width=5000,Frequency=50)
    #The prompt pulses at 50Hz are at 0, 2e4, 4e4, 6e4,8e4 microseconds

    #do some checks
    x=w1.dataX(0)
    y=w1.dataY(0)
    
    print("Y( {} ) =  {:.1f}".format(x[100], y[100]))
    print("Y( {} ) =  {:.1f}".format(x[1000], y[1000]))
    print("Y( {} ) =  {:.1f}".format(x[4100], y[4100]))
    print("Y( {} ) =  {:.1f}".format(x[5000], y[5000]))

.. testcleanup:: RemovePromptPulse

    DeleteWorkspace('w')
    DeleteWorkspace('w1')

Output:

.. testoutput:: RemovePromptPulse
    
    Y( 1000.0 ) =  0.0
    Y( 10000.0 ) =  1.0
    Y( 41000.0 ) =  0.0
    Y( 50000.0 ) =  1.0

The spectra should look like

.. figure:: /images/RemovePromptPulse.png
   :alt: RemovePromptPulse.png

.. categories::

.. sourcelink::
