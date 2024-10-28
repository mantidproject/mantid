============
SANS Changes
============

.. contents:: Table of Contents
   :local:

New Features
------------
- The ILL SANS workflow now adds more metadata to the reduced sample workspace. The sample logs now include information about empty container, empty beam, absorber, etc. plus the workspace names and files that were used to create them.
- Sample transmission is now overwritten after transmission correction is done as sample, transmission, and stitching factors are added to the sample logs of the stitching output workspace as ``stitch_scale_factors``.
- Support loading and reducing data for the new detector for D16 at ILL.

Bugfixes
--------
- Changed :ref:`LoadEventNexus <algm-LoadEventNexus>` to skip pulsetime lookup on 0 event sections, preventing the crash of legacy files.
- The depolarisation columns output by algorithm :ref:`SaveSESANS <algm-SaveSESANS>` are now normalised by the sample thickness. A new parameter, ``OverrideSampleThickness``, allows an alternative value for normalisation to be specified if needed.


:ref:`Release 6.5.0 <v6.5.0>`
