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

Muon Analysis is for reducing and analysing data from muon instruments at ISIS and SMuS.
Muons are implanted into the sample and decay with a lifetime of about 2.2 microseconds.
The decay products (positrons) are detected and the signal measured is the count of positrons over time.

The instruments have many detectors (sometimes up to 600) in different positions, grouped spatially into "groups".
In a typical muon spin resonance experiment, the muon spins precess in a magnetic field, meaning that the positron count seen on a particular detector will be modulated with time. This gives information about the local fields in the sample at the location where the muon is implanted.

Data is loaded into the interface, corrected for dead time, and the detectors grouped into groups.
For example, MuSR (64 detectors) uses two groups, *fwd* (forward, detectors 33-64) and *bwd* (backward, detectors 1-32) in its longitudinal field configuration, corresponding to the two detector rings.
(MuSR is special in that the main field direction can be rotated either longitudinal or transverse). 
In this case scientists will usually look at a "group pair" called *long*, defined as *fwd - bwd*.

The grouping used is taken from the IDF by default, or from the data file if not present.
The user can also specify their own grouping.

The counts or log counts can be plotted, but what scientists usually want to look at is the **asymmetry** (see below).

Processing is done using the workflow algorithm :ref:`algm-MuonProcess` - see there for a fuller explanation and diagram of the steps performed.

The "Data Analysis" tab is then used to fit a function to the data, and tables of the results from such fits can be generated with the "Results Table" tab.

For a full explanation of what the interface does and how to use it, please see the user docs.
Other useful documents are the "Mantid 4 Muons" document and the training school workbook, both of which can be found on the `Muon page <http://www.mantidproject.org/Muon>`_.

Groups and Periods
##################

Groups are explained above, as sets of detectors corresponding to physical arrangements.
For example, the MuSR instrument has two rings of detectors, leading to two detector groups in the longitudinal field arrangement.

*Periods* refers to collecting data in several periods of time - for example you might have two periods "RF on" and "RF off" or "laser on" and "laser off". 
The interface allows users to analyse one period at a time, or a sum/difference (such as "1+2", "1-2" or even "1+2-3+4").

.. warning:: Note that for summed periods (1+2), you sum the periods 1 and 2 first, then calculate the asymmetry of the sum. For subtracted periods (1-2), you calculate the asymmetry of each period separately, then subtract the asymmetry of period 2 from that of period 1. So for 1+2-3+4, you sum 1+2 and 3+4, calculate asymmetry of each sum, then subtract the asymmetry of (3+4) from that of (1+2).

.. note:: We don't support names of periods yet, they are referred to by number. This is something scientists would like to have in the future, though. The names are stored in the NeXus file.

.. topic:: Asymmetry example

    Let's look at a group first - the *fwd* group of ``MUSR00022725.nxs``.
    For a group, you get the asymmetry simply by removing the exponential decay:

    .. image:: ../images/MuonAnalysisDevDocs/MUSR22725-fwd.png
      :align: center

    Now let's look at a group pair - the *long* pair (*fwd - bwd*) of ``MUSR00060625.nxs``.
    For this, the asymmetry is defined as (see :ref:`algm-AsymmetryCalc`)

    .. math:: \textrm{Asymmetry} = \frac{F-\alpha B}{F+\alpha B}

    where :math:`F` is the front spectra, :math:`B` is the back spectra
    and :math:`\alpha` is the balance parameter - see :ref:`algm-AlphaCalc`.

    .. image:: ../images/MuonAnalysisDevDocs/MUSR60625-long.png
      :align: center


Loading data
^^^^^^^^^^^^

(NeXus v0,1,2 - see alg documentation)
(Dead times, grouping, different kinds of plots etc)
(Loader - new - and grouping)
(Load current data - Windows only - where the data is - ISIS)
(Windows - Origin and WiMDA)
(MuonAnalysisHelper - tests)

Plotting data
^^^^^^^^^^^^^
(plotting - Python code)
(Different Plot buttons on grouping tab, auto-update vs plot button)
(Raw data vs rebinning)

Fitting data
^^^^^^^^^^^^
(fitting tab - architecture and testing)

Generating results tables
^^^^^^^^^^^^^^^^^^^^^^^^^
(How the results tables work + testing)
