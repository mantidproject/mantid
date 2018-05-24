.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Normalise detector counts by the sample thickness.


Usage
-----

.. testcode:: NormaliseByThicness

    #create a workspace
    raw=CreateSampleWorkspace()
        
    #apply algorithm
    norm=NormaliseByThickness(raw,SampleThickness=10)

    #do a quick check
    print(norm[1])
    print("Min(raw)= {}".format(raw.dataY(0).min()))
    print("Min(norm)= {}".format(norm[0].dataY(0).min()))
    print("Max(raw)= {}".format(raw.dataY(0).max()))
    print("Max(norm)= {}".format(norm[0].dataY(0).max()))
    
    
.. testcleanup:: NormaliseByThicness

    DeleteWorkspace('raw')
    DeleteWorkspace('norm')


Output:

.. testoutput:: NormaliseByThicness

    Normalised by thickness [10 cm]
    Min(raw)= 0.3
    Min(norm)= 0.03
    Max(raw)= 10.3
    Max(norm)= 1.03

.. categories::

.. sourcelink::
