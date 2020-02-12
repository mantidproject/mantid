============
MuSR Changes
============

.. contents:: Table of Contents
   :local:

New
###

Improvements
############

Algorithms
-------------

Muon Analysis 2 and Frequency Domain Interfaces
---------------------------------------------------
- The plotting window within the Muon Analysis 2 and Frequency Domain Interfaces has been converted into a dockable window,
  which can be docked and undocked at will. This change improves the user experience for those working on laptops.
- Improved the plotting within the Muon Analysis 2 and Frequency Domain interfaces by introducing tiled plotting.
  The user can choose whether they would like tiled plotting over the groups/pairs or by run. Within these plotting improvements
  a number of additional changes have been made, most notably the removal of the grouping options from the home tab. Instead,
  plots are controlled through the grouping tab, which is also used to determine which groups/pairs to undertake fitting on.
  In the fitting tab, improvements have been made to make the process of simultaneous fitting over groups/pairs easier, which in
  part due to the new tiled plotting feature.
- Added a plotting options toolbar to the docked plotting in the Muon Analysis 2 and Frequency Domain interfaces. This toolbar
  can be used to autoscale the axes, change the axes limits, and control the plotting of errors.

Bug Fixes
#########

- The elemental analysis GUI can now handle legacy data which is missing a response dataset, e.g Delayed.
- Fixed a bug with constraints in the Muon Analysis 2 GUI which would cause Mantid to crash.
- Fixed a bug in Muon Analysis Old that prevented the muon fitting functions from appearing in the data analysis tab.
- Data sets can now be reloaded while the Instrument View is open without crashing Mantid.

:ref:`Release 4.3.0 <v4.3.0>`