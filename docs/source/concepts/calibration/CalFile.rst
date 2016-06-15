.. _CalFile:

CalFile
=======

Summary
-------

The CalFile is a calibration file originally created for use in the
Ariel powder diffraction data reduction package.

Files using this format with the .cal extension are still used by
several algorithms in Mantid.

File Format
-----------

The file is a simple text file with the following format

.. code-block:: none

  #  number  UDET  offset  select  group
     0        611  0.0000000  1    0
     1        612  0.0000000  1    0
     2        601  0.0000000  0    0
              ...
    19     101001 -0.0497075  1    2
    20     101002 -0.3515893  1    2
    21     101003 -0.2803879  0    2
              ...
   349     201001  0.0525040  1    3
   350     201002  0.0538936  1    3
   351     201003  0.0535027  1    3
              ...

The first column is simply an index, the second is a UDET identifier for
the detector, the third column corresponds to an offset in
:math:`\delta d/d` (this is usually applied in Mantid using the
:ref:`AlignDetectors  <algm-AlignDetectors>` algorithm). The fourth column is a
flag to indicate whether the detector is selected and should be used in
:ref:`DiffractionFocussing  <algm-DiffractionFocussing>`. The fifth column
indicates the group this detector belongs to (number >=1), zero is not
considered as a group.

Masking
-------

The most important columns for masking are the **UDET** and the
**select** column. The **number**, **offset** and **group** entries are
ignored.

Algorithms that work with CalFiles
----------------------------------

Using CalFiles
##############

* :ref:`AlignDetectors  <algm-AlignDetectors>` will apply the offset from a CalFile while converting the data from time of flight to wavelength.
* :ref:`DiffractionFocussing  <algm-DiffractionFocussing>` uses the selection and grouping columns of a CalFile to focus the diffraction data.

Loading and saving CalFiles
###########################

* :ref:`LoadCalFile <algm-LoadCalFile>` will load the CalFile into separate offset, masking and grouping workspaces.
* :ref:`SaveCalFile <algm-SaveCalFile>` will save a CalFile from separate offset, masking and grouping workspaces.
* :ref:`MaskWorkspaceToCalFile<algm-MaskWorkspaceToCalFile>` will save a mask workspace as a CalFile.
* :ref:`ReadGroupsFromFile<algm-ReadGroupsFromFile>` Reads the groups from a CalFile, and output a 2D workspace containing on the Y-axis the values of the Group each detector belongs to. This is used to visualize the grouping scheme for powder diffractometers.


Creating CalFiles
#################

* :ref:`CalibrateRectangularDetectors<algm-CalibrateRectangularDetectors>` will output a CalFile if the SaveAs property is set to "Calibration".
* :ref:`GetDetectorOffsets<algm-GetDetectorOffsets>` will output a CalFile if the GroupingFilename is set.
* :ref:`CreateCalFileByNames<algm-CreateCalFileByNames>` will create a CalFile with the grouping column set according to a list of bank names.
* :ref:`CreateDummyCalFile<algm-CreateDummyCalFile>` creates a CalFile from a workspace. All of the offsets will be zero, and the pixels will be all grouped into one group.

Merging CalFiles
################

* :ref:`MergeCalFiles<algm-MergeCalFiles>` combines the data contained in two CalFiles, based on the selections offsets, selections and groups can be merged. The matching rows are determined by UDET. Any unmatched records are added at the end of the file.


.. categories:: Calibration
