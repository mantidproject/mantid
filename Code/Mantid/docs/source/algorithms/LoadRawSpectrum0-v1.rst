.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The LoadRawSpectrum0 algorithm stores spectrum zero data from the
selected :ref:`RAW <Raw File>` file in a :ref:`Workspace2D <Workspace2D>`.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Only for .Raw Files**  

.. testcode:: raw

    #Spectrum 0 contains any data from detectors not mapped to spectra
    wsOut = LoadRawSpectrum0('HRP39180.RAW')

    wsIntegral = Integration(wsOut)
    print ("Spectrum0 contained %i counts" % wsIntegral.readY(0)[0])

Output:

.. testoutput:: raw

    Spectrum0 contained 0 counts

.. categories::
