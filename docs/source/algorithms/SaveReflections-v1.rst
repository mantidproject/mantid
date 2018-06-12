
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Save a :ref:`PeaksWorkspace <PeaksWorkspace>` to a variety of different text formats. This algorithm currently provides support for exporting a PeaksWorkspace to GSAS, SHELX, Fullprof, and Jana formats.

Usage
-----
**Example - a simple example of running SaveHKL.**

.. testcode:: ExSaveReflections

    import os

    path = os.path.join(os.path.expanduser("~"), "MyPeaks.hkl")

    #load a peaks workspace from file
    peaks = LoadIsawPeaks(Filename=r'Peaks5637.integrate')

    SaveReflections(peaks, Filename=path)

    print(os.path.isfile(path))

Output:

.. testoutput::  ExSaveReflections

    True

.. testcleanup:: ExSaveHKLSimple

    import os
    def removeFiles(files):
      for ws in files:
        try:
          path = os.path.join(os.path.expanduser("~"), ws)
          os.remove(path)
        except:
          pass

    removeFiles(["MyPeaks.hkl"])

.. categories::

.. sourcelink::
