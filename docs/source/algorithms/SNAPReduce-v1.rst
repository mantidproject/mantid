.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The purpose of this algorithm is to do a full reduction of SNAP
data. This allows several runs, and with all the typical options that
are usually used at the beamline, including calibrate from a cal file
and from Convert Units, mask from file workspace and default masks,
several groupings and save in GSAS or Fullprof format.

AutoConfiguration Creator
=========================
Option `EnableConfigurator` bypasses the autoreduction. Instead, a file containing the names and values
of certain input properties is saved to disk.

- All properties except `RunNumbers`, `EnableConfigurator`, and `ConfigSaveDir` are saved.
- The set of property names and values are cast into a python dictionary, then saved in JSON format.
- The file name follows the format YYYY-MM-DD_HH-MM-SS.json, the timestamp being the time of file creation.
- The file is saved to the location specified in `ConfigSaveDir`. If no location is specified then the file is saved to /SNS/SNAP/IPTS-XXXX/shared/autoreduce/configurations/, where XXXX stands for the IPTS number of the first run passed on to property "RunNumbers".

Usage
-----

.. categories::

.. sourcelink::
