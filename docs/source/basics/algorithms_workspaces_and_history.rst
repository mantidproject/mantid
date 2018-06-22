.. _algorithms workspaces and history:

==================================
Algorithms, workspaces and history
==================================

Introduction
============

Workspaces are the data storage units of Mantid. 

Algorithms load, save and manipulate the data in a workspace.

The current list of available algorithms is very large, but
most algorithms interact with workspaces in the same way, generating a new output workspace as the result.

In this section we will introduce you to some basic types of workspaces, and some key algorithms to operate on those
workspaces.

Workspaces
==========

Workspaces come in several forms, but the most common by far is the MatrixWorkspace which represents X, Y, Error data for one or more spectra. 
The MatrixWorkspace itself can be sub-grouped into EventWorkspaces and Workspace2Ds.

Workspace 2D
############

A Workspace2D consists of a workspace with 1 or more spectra. Typically, each spectrum will be a histogram. For each spectrum X,
Y (counts) and E (error) data is stored as a separate array.

Event Workspace
###############

An EventWorkspace stores information about each individual event observation in detectors. More specifically, at a neutron
spallation source, this means that the time of arrival and detector ID of each individual neutron is recorded.

Event workspaces have two importnat properties:

* Pulse time – when the proton pulse happened in absolute time.
* Time-of-flight – time for the neutron to travel from moderator to the detector

Other Workspace Types
#####################

* Group Workspaces store a collection of other workspaces in a group, these can be created manually and are often used in multi-period data. Either the whole group or individual members can be processed using algorithms.
* Table Workspaces stores data as cells, just like a spreadsheet. Columns determine the type of the data, for example double precision float, while each  entry appears as a new row.
* Peaks workspaces are a special type of table workspaces with additional support for single crystal peaks.
* MD workspaces are for holding multi-dimensional data, these are described in more detail here: ref::`MDWorkspace`.

