.. _RAW File:

RAW_File
========

The RAW file format has been for many years the primary data format of
the ISIS facility.

Structure
---------

The Raw file is a binary formatted file that uses a simple but effective
compression system to achieve 4-1 compression of the data in holds.

Compression
-----------

To be completed later

Alternate Data Streams
----------------------

When the Raw file is archived to the NTFS archive additional information
about it is added as an Alternate Data Stream. This includes a list of
all of the files that were archived with this RAW file and a checksum
for each. This can provide a fast way of finding the log files
associated with a RAW file in a crowded directory.

For example:

::

    d28cb560cdefc765fc8d550b6f335006 *SANS2D00000799.log
    52526cd14652284c2748e905ff74fc4b *SANS2D00000799.nxs
    7d3b5776c32b63aa76b9e36d2e7ec348 *SANS2D00000799.raw
    de230169cda344118d26315dd31c0fb4 *SANS2D00000799.s001
    cbd0685b6ce19781fca13ba4395318d9 *SANS2D00000799.s01
    f9b9aa805179598207d6bf713eacd5a7 *SANS2D00000799_Beam_Shutter.txt
    d10238be18d69d21f8890f73805795bb *SANS2D00000799_Changer.txt
    acc92b1223bb93ad85e03b523639142a *SANS2D00000799_Fast_Shutter.txt
    671d77b4d2d9a1deee3e6dc9c3db9875 *SANS2D00000799_Height.txt
    2eb44a65f79f73e194cdcd48d38385b4 *SANS2D00000799_ICPdebug.txt
    0268e471a0ae4c0382ffb81622e03dde *SANS2D00000799_ICPevent.txt
    3cda6c3f179c6f70fea82e9ad9e75360 *SANS2D00000799_ICPstatus.txt
    19ea5cbe0e57b01db61b01ba24f70bc5 *SANS2D00000799_Julabo.txt
    2e6bb0fe5965527d1cb319fff0df928d *SANS2D00000799_Moderator_Temp.txt
    604946b7385f8d08ff9c9eb47b0803c2 *SANS2D00000799_Sample.txt
    cf5022506705b6ba5c850edd824088be *SANS2D00000799_Status.txt
    54e4a8e82fe00cd5e9263bb14506682f *SANS2D00000799_Table.txt

In the links below you will streams or DIR on Vista can list the streams
on a file. Not many programs can display the contents of an alternate
data stream, but so far I have discovered that the following work, at
least for RAW files.

::

    notepad2.exe SANS2D00000799.raw:checksum
    more < SANS2D00000799.raw:checksum 

If a file with an alternate data stream is copied to an FAT file system
the alternate data stream is lost. It has been reported that if a file
with an ADS is copied using SAMBA to a unix file system the streams are
extracted into seperate files with automatically generates suffixes.

More information about Alternate Data Streams
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

-  `Microsoft support
   article <http://support.microsoft.com/kb/105763>`__
-  `DIR command on Vista can list Alternate
   streams <http://bartdesmet.net/blogs/bart/archive/2006/07/13/4129.aspx>`__
-  `Streams.exe from SysInternals can list alternate
   streams <http://technet.microsoft.com/en-us/sysinternals/bb897440.aspx>`__
-  `Practical Guide to Alternative Data Streams in
   NTFS <http://www.irongeek.com/i.php?page=security/altds>`__

See also
~~~~~~~~

:ref:`Nexus file <Nexus file>` a newer type of data file



.. categories:: Concepts