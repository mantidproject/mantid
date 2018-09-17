.. _01_why_analyse_live_data:

======================
 Why Analyse Live Data 
======================

Why might you want to look at live data?
========================================

-  Monitoring the experiment

   -  Raw data (e.g. monitor plots, alignment scans, etc…)

      -  Are neutrons actually getting through? Are the shutters really
         open?
      -  Is my crystal aligned?
      -  Is everything still going ok?

   -  Reduced data (e.g. S(Q,ω), etc…)

      -  Are the statistics good enough yet?
      -  Given this result what should we look at next?

-  This enables better Experimental steering

   -  Provide user with more information to make better use of beam time
   -  Automatic feedback to Data Acquisition System

      -  This has been shown to work in principle and is a future area
         of development for Mantid.

Perhaps the better question is "Why should you have to wait for the run
to finish, and the file to be written to see your data?".

Why do this in Mantid?
======================

This type of thing could be built into each facilities data acquisition
system, indeed Mantid needs the data acquisition system to broadcast the
live data stream, so why choose mantid as the location for live data
processing and visualization?

-  Standard framework for data reduction
-  Can process Event and Histogram data
-  Provides comprehensive visualisation tools

   -  which can update automatically with data changes

-  Easy to customize and extend.

.. raw:: mediawiki

   {{SlideNavigationLinks|MBC_Exercise_Intelligent_Fitting|Mantid_Basic_Course|MBC_Live_Data_Workflow }}
