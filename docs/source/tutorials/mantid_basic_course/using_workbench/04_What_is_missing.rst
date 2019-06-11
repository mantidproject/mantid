.. _04_What_is_missing:

==============================
What is missing from Workbench
==============================

While much of MantidPlot's functionality can be replicated through the python scripts the interface on Workbench does lack some aspects that you have 
used in previous parts of this tutorial.

Plotting tools
==============

In the loading_and_displaying_data chapter you learnet how to create 2D colour plots and 3D surface plots. Colour plots in Workbench cannot have 
their colour bar scale changed between linear and logarithmic removing the usefulness of colourfill plots.
Workbench also lacks an interface for producing 3D plots at all, as well as the advanced plot tool that was presant in MantidPlot.
Access to these plots must be done through Matplotlib directly, it may help to consult the `Matplotlib in Mantid tutorial <https://docs.mantidproject.org/nightly/plotting/index.html#simple-plots>`_.



Interfaces
==========

MantidPlot provides a range of different interfaces that are not yet available in Workbench. As of Mantid 4.0, the interfaces available through Workbench are:

* Defraction

  - HFIR 4 Circle Reduction
  
  - Powder Defraction Reduction
  
* Direct

  - DGSPlanner
  
  - DGS Reduction
  
  - MSlice
  
  - PyChop
  
* SANS
  
  - ISIS SANS
  
  - ORNL SANS
  
* Utility

  - FilterEvents
  
  - QECoverage
  
  - TofConverter

