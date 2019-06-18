.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This Loader reads XML data files from HFIR SANS Instruments (BioSANS and GPSANS) created by the SPICE data acquisition.
It places the detector(s) at the right position and sets some properties necessary for posterior data reduction.

For both BioSANS and GPSANS places the main detector at the right distance.
In the case of the BioSANS rotates the wing detector to the right angle.

Finds and sets the logs:

* ``beam-trap-diameter``
* ``wavelength``
* ``wavelength-spread``
* ``wavelength-spread-ratio``
* ``monitor``
* ``timer``
* ``sample-thicknes``
* ``source-aperture-diameter``
* ``sample-aperture-diameter``
* ``number-of-guides``
* ``sample-detector-distance`` and ``sdd``
* ``source-sample-distance``
* ``beam-diameter``

.. categories::

.. sourcelink::
