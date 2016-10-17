============
SANS Changes
============

.. contents:: Table of Contents
   :local:

Bug Fixes
---------

|

X uncertainties (delta-Q)
-------------------------

- X uncertainties (Dx) are now always stored as one value per *bin*, i.e., not as one value per *bin edge*.
- When loading legacy Nexus files (files with Dx data that were stored with previous Mantid versions) the last Dx value is ignored.

`Full list of changes on github <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.9%22+is%3Amerged+label%3A%22Component%3A+SANS%22>`__
