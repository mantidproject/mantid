.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm sorts a group workspace by the qvectors found in the
qvectors file. Workspaces will be tranformed if the qvectors dimension
is in the bins. Used for output from LoadSassena.

Usage
-----

.. include:: ../usagedata-note.txt

.. testcode:: ExSortByQVectors

  
    ws = LoadSassena("outputSassena_1.4.1.h5", TimeUnit=1.0)
    print 'workspaces instantiated: ', ', '.join(ws.getNames())
    SortByQVectors(InputWorkspace='ws')
    ws0 = ws[0]
    for i in range(0, ws0.getNumberHistograms()):
            print ws0.dataY(i)[0]

Output:

.. testoutput:: ExSortByQVectors

    workspaces instantiated:  ws_qvectors, ws_fq, ws_fq0, ws_fq2, ws_fqt.Re, ws_fqt.Im
    0.0
    0.00600600591861
    0.0120120118372
    0.0180180184543
    0.0240240236744

.. categories::
