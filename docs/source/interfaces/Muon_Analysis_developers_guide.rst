.. _Muon_Analysis_DevelopersGuide-ref:

Muon Analysis: guide for Mantid developers 
==========================================

.. contents:: Table of Contents
    :local:
    
Preamble
^^^^^^^^^
This document is intended for Mantid developers as a guide to the architecture of the Muon Analysis custom interface.
User documentation for this interface can be found at :ref:`Muon_Analysis-ref`.

There will be a particular focus on the *Data Analysis* tab, which has been significantly changed for Mantid 3.8.

There is also another custom interface for muons: ALC. Development on this is much easier as it has an MVP architecture and is far better tested, so it will not be covered in this document.

What the interface is for
^^^^^^^^^^^^^^^^^^^^^^^^^

(typically, what do scientists do with it?)

Loading data
^^^^^^^^^^^^

(NeXus v0,1,2 - see alg documentation)
(Dead times, grouping, different kinds of plots etc)
(Loader - new - and grouping)
(MuonAnalysisHelper - tests)
(plotting - Python code)
(fitting tab - architecture and testing)
(How the results tables work)
