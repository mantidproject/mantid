.. _Nexus file:

Nexus File
==========

A **Nexus file** is a type of data file used by various instruments
and by MantidPlot. The format appears to be like an XML file plus some
unprintable characters.

Mantidplot is capable of loading certain types of Nexus files and of saving certain types of
:ref:`workspace <workspace>` as a Nexus file.  It can also save a
:ref:`project <project>` as a mantid file plus Nexus files.

Structure
---------

ISIS uses NEXUS files for both histrogram and event data and SNS uses NEXUS only for event data. 
Also both ISIS and SNS use the same structure for event data. 
Hence there are two principal types of NEXUS files loaded by Mantid 
 - ISIS Nexus file, which is loaded by LoadISISNexus and
 - Event Nexus file, which is loaded by LoadEventNexus.
Also there is a third kind of Nexus file, which is produced by Mantid, when it saves a workspace
to Nexus, which is called a Processed Nexus file and is saved by SaveNexusProcessed.

See also
--------

:ref:`RAW File <RAW File>` an older data file format.



.. categories:: Concepts