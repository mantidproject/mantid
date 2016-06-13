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

- :ref:`SNSPowderReduction <algm-SNSPowderReduction>` has changed
  parameters. ``Instrument``, ``RunNumber``, and ``Extension`` have
  been replaced with a single ``Filename`` parameter. This has been
  paired with changes to the Powder Diffraction interface as well. An
  additional parameter, ``LogFilename``, has been added to aid in
  testing files being produced by the "new DAS" at SNS. This parameter
  has not been added to the Powder Diffraction interface and will be
  removed without notice.

Full list of `diffraction <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.8%22+is%3Amerged+label%3A%22Component%3A+Diffraction%22>`_
and
`imaging <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.8%22+is%3Amerged+label%3A%22Component%3A+Imaging%22>`_ changes on GitHub.
