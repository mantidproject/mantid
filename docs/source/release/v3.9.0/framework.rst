=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

Algorithms
----------

New
###


Improved
########

- :ref:`CalculateFlatBackground <algm-CalculateFlatBackground>` has now a new mode 'Moving Average' which takes the minimum of a moving window average as the flat background.

Deprecated
##########

MD Algorithms (VATES CLI)
#########################

Performance
-----------

CurveFitting
------------

Improved
########

Python
------

Python Algorithms
#################

- :ref:`MatchPeaks <algm-MatchPeaks>` performs circular shift operation (numpy roll) along the x-axis to align the peaks in the spectra.
- :ref:`FindEPP <algm-FindEPP>` is improved to better determine the initial parameters and range for the fitting.

Bug Fixes
---------

- Bin masking information was wrongly saved when saving workspaces into nexus files, which is now fixed.
- :ref:`LoadEventNexus <algm-LoadEventNexus>` should no longer leak memory when the execution is cancelled.
- If a run is aborted and restarted, the ``running`` log in the workspace will correctly reflect this. (``running`` will be false at all times before the abort.)

Full list of
`Framework <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.9%22+is%3Amerged+label%3A%22Component%3A+Framework%22>`__
and
`Python <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.9%22+is%3Amerged+label%3A%22Component%3A+Python%22>`__
changes on GitHub

Bug Fixes
---------

- Fixed several issues with masked detectors and neighbour counts in the nearest-neighbour code used by a few algorithms.
