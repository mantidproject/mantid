=============
Muon Analysis
=============

.. contents:: Table of Contents
   :local:

Interfaces
----------

ALC
###

Muon Analysis
#############


- Fixed a bug where the "load next/previous" buttons appended runs in certain cases where this was not the intended behaviour.
- The *Grouping Options* and *Data Analysis* tabs now only become enabled once data has been loaded on the *Home* tab. Data should first be loaded on the *Home* tab, then it can be grouped/fitted. After the first dataset has been loaded, additional datasets can of course be added (for a multi-dataset fit) using the *Data Analysis* tab.
- The "compatibility mode" option has been renamed "Enable multiple fitting", and defaults to off. The interface will therefore look the same as it did in Mantid 3.7 by default. To enable the new multi-fitting functionality, just tick the box on the settings tab.
- The layout of the new fitting tab UI has been improved to give more space for the fitting function, and enable the relative space given to each section to be adjusted by the user.
- Fixed a bug where stale plot guesses would be left on the graph in some situations.
- Fixed a bug with load current run that meant it would be actually loading old data due to caching. Data from current run files is no longer cached behind the scenes.

Algorithms
----------

Fit Functions
-------------

|

`Full list of changes <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.9%22+is%3Amerged+label%3A%22Component%3A+Muon%22>`_
on GitHub.
