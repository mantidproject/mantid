===================
Diffraction Changes
===================

.. contents:: Table of Contents
   :local:

Crystal Improvements
--------------------

Engineering Diffraction
-----------------------

Powder Diffraction
------------------

:ref:`algm-SNSPowderReduction` had an error in logic of subtracting the vanadium background. It was not being subtracted when ``PreserveEvents=True``.

:ref:`algm-PDLoadCharacterizations` and
:ref:`algm-PDDetermineCharacterizations` have been upgraded to support
sample container specific information, as well as additional
information about the empty sample environment and instrument.


Full list of `diffraction <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.9%22+is%3Amerged+label%3A%22Component%3A+Diffraction%22>`_
and
`imaging <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.9%22+is%3Amerged+label%3A%22Component%3A+Imaging%22>`_ changes on GitHub.
