.. _Nexus file:

Nexus File
==========

A **Nexus file** is a common data exchange format for neutron, X-ray,
and muon experiments.

Mantid is capable of loading certain types of Nexus files and of saving certain types of
:ref:`workspace <workspace>` as a Nexus file.  It can also save a
:ref:`project <project>` as a mantid file plus Nexus files.

Structure
---------

The general structure of Nexus files is explained in http://download.nexusformat.org/doc/html/user_manual.html .

Here are some specific details:

ISIS uses Nexus files for both histrogram and event data and SNS uses NEXUS for event data only.
Also both ISIS and SNS use the same structure for event data.
Hence there are two principal types of NEXUS files loaded by Mantid

 - ISIS histogram Nexus file, which is loaded by :ref:`LoadISISNexus <algm-LoadISISNexus>` and
 - Event Nexus file, which is loaded by :ref:`LoadEventNexus <algm-LoadEventNexus>`.

ISIS uses two versions of Nexus files for muon histogram data:

 - Muon Nexus file v1, which is loaded by :ref:`LoadMuonNexus v1<algm-LoadMuonNexus-v1>` and
 - Muon Nexus file v2, which is loaded by :ref:`LoadMuonNexus v2<algm-LoadMuonNexus-v2>`.


As well as Nexus files loaded by Mantid, there is a kind of Nexus file,
which is produced by Mantid, when it saves a workspace
to Nexus, which is called a Processed Nexus file and is saved by
:ref:`SaveNexusProcessed <algm-SaveNexusProcessed>`.

.. seealso:: :ref:`RAW File <RAW File>` an older data file format.

.. categories:: Concepts
