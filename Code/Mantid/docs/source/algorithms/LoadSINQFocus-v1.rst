.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The LoadSINQFocus algorithm loads a file containing data from the Instrument `FOCUS`_ at SINQ. The data is stored in a MatrixWorkspace. The algorithm automatically loads the instrument definition.

Please note that this algorithm has been declared obsolete. The data may be loaded with the more recent :ref:`algm-LoadSINQFile` algorithm.

.. _FOCUS: http://www.psi.ch/sinq/focus

Usage
-----

.. include:: ../usagedata-note.txt

The following example script loads a data file obtained at the FOCUS instrument and prints some information about the obtained workspace.

.. testcode :: ExFocus

    # Load FOCUS data
    focus_2906 = LoadSINQFocus('focus2014n002906.hdf')
    
    # Print out some information
    print "Sample title:", focus_2906.getTitle()
    print "Number of spectra:", focus_2906.getNumberHistograms()
    
Output:

.. testoutput:: ExFocus

    Sample title: water at 320K
    Number of spectra: 375


Creating a color fill plot of the resulting workspace should result in an image similar to the one below.

.. figure:: /images/LoadSINQFocus_H2O.png
   :figwidth: 15 cm
   :align: center
   :alt: FOCUS data of water at 320 K.
   
   FOCUS data of water at 320 K.

.. categories::
