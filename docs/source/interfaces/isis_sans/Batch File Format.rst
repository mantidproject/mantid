.. _ISIS_SANS_Batch_File-ref:

=================
Batch File Format
=================

Each line of the file specifies the runs to use for a single reduction.
First the role of the run and then a comma and then the run number.
So the following file specifies three runs:

.. code-block:: none

    sample_sans, 992, output_as, 992_sans_nocan
    sample_sans, 992, sample_trans, 988, sample_direct_beam, 987, output_as, 992_sans
    sample_sans,  992, sample_trans, 988, sample_direct_beam, 987, can_sans, 993, can_trans, 989, can_direct_beam, 987, output_as, 992_full

Often it is easier to edit the files in a spreadsheet application, like Excel,
where it is possible to enter large numbers of columns more easily:

.. image:: /images/ISISSansInterface/batch_file_spreadsheet_example.png
   :align: center
   :width: 800px


It is possible to analyse a single period (entry) from a multi-period run file.
Specify the period by appending a 'p' followed by the entry number to the
run number.

If a multi-period run is entered for the "sample_sans" all periods in the
sample are reduced and the result will be a workspace group. If any
multi-period run was included on the same line without a period specification
then the period 1 in the sample will be reduced with period 1 from that run,
period 2 with period 2, etc. An error will be raised if the number of periods
differs from that in the sample. If an individual period is specified for
the "sample_sans" but no period is given for another run then the first period
from that run is used in the analysis by default.

It is also possible to specify individual user files for each row.
If the user file is not specified for a row then the default user file and
potential setting changes which the user made via the GUI are applied.
Manual changes of the settings via the GUI will be ignored for rows which have
an individual user file. So the following file specifies three runs where the
first and the last use individual user files and the second run uses the
default user file which is specified at the top of the "Run Numbers" tab:

.. code-block:: none

    sample_sans, 992, output_as, 992_sans_nocan, user_file, user_file1.txt
    sample_sans, 998, output_as, 998_sans_nocan
    sample_sans, 999, output_as, 999_sans_nocan, user_file, user_file2.txt
