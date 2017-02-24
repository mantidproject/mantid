======================
UI & Usability Changes
======================

.. contents:: Table of Contents
   :local:

Installation
------------

Windows
#######

OS X
####

User Interface
--------------


Instrument View
###############
 - Fixed a bug preventing the some of the banks from being visible when using a U correction.
 - Fixed a bug where pressing delete would delete a workspace even when the dock was not focused.

Plotting Improvements
#####################

Algorithm Toolbox
#################

- The Algorithm Progress bar has been improved to handle reporting the progress of multiple algorithms much better.  Now it will correctly show the progress of the most recently started algorithms, and correctly move onto the next most recent should  the first finish sooner.  In addition the "Details" button now shows whether Mantid is Idle or how many algorithms it is running.
  
.. figure:: ../../images/Progress_running.png
   :class: screenshot
   :width: 396px

Scripting Window
################

Documentation
#############

Bugs Resolved
-------------

SliceViewer Improvements
------------------------

VSI Improvments
---------------
Update ParaView to v5.3.0-RC1

|

Full list of
`GUI <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.10%22+is%3Amerged+label%3A%22Component%3A+GUI%22>`_
and
`Documentation <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.10%22+is%3Amerged+label%3A%22Component%3A+Documentation%22>`_
changes on GitHub
