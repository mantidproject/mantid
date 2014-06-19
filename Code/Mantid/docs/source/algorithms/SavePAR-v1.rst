.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Saves the geometry information of the detectors in a workspace into a
PAR format ASCII file. The angular positions and linear sizes of the
detectors are calculated using :ref:`algm-FindDetectorsPar`
algorithm.

Tobyfit PAR file is an ASCII file consisting of the header and 5 or 6
text columns. Mantid generates 6-column files. Header contains the
number of the rows in the phx file excluding the header. (number of
detectors). The column has the following information about a detector:

| `` *``
| `` *         1st column      sample-detector distance (secondary flight path)``
| `` *         2nd  "          scattering angle (deg)``
| `` *         3rd  "          azimuthal angle (deg)``
| `` *                         (west bank = 0 deg, north bank = -90 deg etc.)``
| `` *                         (Note the reversed sign convention wrt.  **.phx** files (:ref:`algm-SavePHX` files)
| `` *         4th  "          width (m)``
| `` *         5th  "          height (m)``
| `` *         6th  "          detector ID    -- Mantid specific. ``
| `` *---``

You should expect to find column 6 to be the detector ID in
Mantid-generated par files only.

.. categories::
