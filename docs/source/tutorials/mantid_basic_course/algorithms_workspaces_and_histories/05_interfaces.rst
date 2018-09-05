.. _05_interfaces:

==========
Interfaces 
==========

Custom Interfaces
=================

Some analysis workflows are too complex, need more
user input or need a better way to present the results than a single
algorithm interface can provide. This is where custom interfaces come
in. These are pluggable interfaces for Mantidplot that are loaded at run
time to give a much richer interface to work with for your data
analysis. Several interfaces have now been developed to handle different
aspects of the reduction and analysis workflow for data from the various
scientific techniques.

An Example - The Muon Analysis Interface
========================================

The muon analysis interface is an example that uses both algorithmic
analysis, plotting and curve fitting within one interface to provide a
single interface that covers a large proportion of the analysis required
for basic muon spin resonance.

.. figure:: /images/MuonAnalysisInterface.png
   :alt: centre
   :width: 400px

A simple walkthrough
--------------------

#. Start the interface with the Interfaces->Muon Analysis Menu
#. On the Settings tab set the end value to 10
#. On the Home tab Change the instrument to Emu
#. Enter 20884 in the Load Run box and hit enter
#. You will get a plot of the Asymetry of the data
#. You can change the plot data drop down box to see other plot types
#. With the Asymetry plot visible, go to the Data Analysis Tab
#. Right click the grey Functions bar and "Add Function"
#. Under Muon select ExpDecayOsc and click ok
#. Click Fit under the fit button, the fitted data is displayed on the
   graph
#. On the Results Table tab click the sample_magn_field and sample_temp
   logs and click create table
#. A table with all of the fit parameters and the selected log values is
   created

.. raw:: mediawiki

   {{SlideNavigationLinks|MBC_History|Mantid_Basic_Course|MBC_Exercise_Algorithms_History_EventWorkspace}}

