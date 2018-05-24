.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Combines the data contained in two cal files, based on the selections
offsets, selections and groups can be merged. The matching rows are
determined by UDET. Any unmatched records are added at the end of the
file.


Usage
-----

**Example - Merging and appending files**  

.. testcode:: merge

    import os

    #find a suitable directory to save the files
    file_dir = config["defaultsave.directory"]
    if not os.path.isdir(file_dir):
        #use the users home directory if default save is not set
        file_dir = os.path.expanduser('~')

    def writeCalFile(file_name,num_dets,inc_group=True,inc_offset=True,inc_selected=True):
        file_path = os.path.join(file_dir,file_name+".cal")
        #write the cal file out
        with open(file_path, "w") as file:
            for line_no in range(num_dets):
                file.write("%i\t%i\t%f\t%i\t%i\n" %
                (line_no,
                 line_no + 100,
                 (line_no / 1000.0) if inc_offset else 0.0,
                 (line_no % 2) if inc_selected else 0, 
                 (line_no % 3) if inc_group else 1) )
        return file_path  
 
    calFile_master = writeCalFile("masterCal",6)
    calFile_updates = writeCalFile("updateCal",3,
        inc_group=False, inc_offset=False, inc_selected=False)
    output_file = os.path.join(file_dir,"output.cal")

    MergeCalFiles(
        UpdateFile = calFile_updates,
        MasterFile = calFile_master,
        OutputFile = output_file,
        MergeOffsets = True,
        MergeSelections = True,
        MergeGroups = True)

    #Load and print the resulting file
    print("The result file has been updated with 0 for group, select and offset\nwhere the detector id's match")
    with open(output_file, "r") as file:
        print(file.read().rstrip())

    #cleanup
    os.remove(calFile_master)
    os.remove(calFile_updates)
    os.remove(output_file)

Output:

.. testoutput:: merge

    The result file has been updated with 0 for group, select and offset
    where the detector id's match
            0             100       0.0000000        0        1
            1             101       0.0000000        0        1
            2             102       0.0000000        0        1
            3             103       0.0030000        1        0
            4             104       0.0040000        0        1
            5             105       0.0050000        1        2


**Example - Appending files**  

.. testcode:: append

    import os

    #find a suitable directory to save the files
    file_dir = config["defaultsave.directory"]
    if not os.path.isdir(file_dir):
        #use the users home directory if default save is not set
        file_dir = os.path.expanduser('~')

    def writeCalFile(file_name,num_dets,inc_group=True,inc_offset=True,inc_selected=True):
        file_path = os.path.join(file_dir,file_name+".cal")
        #write the cal file out
        with open(file_path, "w") as file:
            for line_no in range(num_dets):
                file.write("%i\t%i\t%f\t%i\t%i\n" %
                (line_no,
                 line_no + 100,
                 (line_no / 1000.0) if inc_offset else 0.0,
                 (line_no % 2) if inc_selected else 0, 
                 (line_no % 3) if inc_group else 1) )
        return file_path  
 
    calFile_master = writeCalFile("masterCal",6)
    calFile_updates = writeCalFile("updateCal",8,
        inc_group=False, inc_offset=False, inc_selected=False)
    output_file = os.path.join(file_dir,"output.cal")

    MergeCalFiles(
        UpdateFile = calFile_updates,
        MasterFile = calFile_master,
        OutputFile = output_file,
        MergeOffsets = False,
        MergeSelections = False,
        MergeGroups = False)

    #Load and print the resulting file
    print("Any additional rows in the update file will be added,\nset the Merge options to False if you don't want to affect existing values")
    with open(output_file, "r") as file:
        print(file.read().rstrip())

    #cleanup
    os.remove(calFile_master)
    os.remove(calFile_updates)
    os.remove(output_file)

Output:

.. testoutput:: append

    Any additional rows in the update file will be added,
    set the Merge options to False if you don't want to affect existing values
            0             100       0.0000000        0        0
            1             101       0.0010000        1        1
            2             102       0.0020000        0        2
            3             103       0.0030000        1        0
            4             104       0.0040000        0        1
            5             105       0.0050000        1        2
            6             106       0.0000000        0        1
            7             107       0.0000000        0        1


.. categories::

.. sourcelink::
