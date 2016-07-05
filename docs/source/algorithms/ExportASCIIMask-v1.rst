.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The algorithm extracts list of the masked spectra numbers from the input workspace and,
if requested, saves these spectra as legacy *.msk* file of ISIS ASCII format.

The format is described on :ref:`LoadMask <algm-LoadMask>` algorithm pages.

Usage
-----

**Example: Export ASCII Masks**

.. testcode:: ExExportASCIIMask
    
    #prepare the data:
    masks = [1,4,8,10,11,12,12,198,199,200]
    test_ws = CreateSampleWorkspace()
    test_ws.maskDetectors(masks)
    
    f_name = os.path.join(config.getString('defaultsave.directory'),'test_ws.msk')
    
    #extract mask:
    r_masks = ExportASCIIMask(test_ws,Filename=f_name)
    
    # Compare results:
    wmsk = ''
    with open(f_name,'r') as res_file:
        for line in res_file:
            wmsk = line
    
    print "Initial mask: ",masks
    print "Extracted mask: ",r_masks
    print "Saved mask: ",wmsk
    
    os.remove(f_name)
    
Output:

.. testoutput:: ExExportASCIIMask

    Initial mask:  [1, 4, 8, 10, 11, 12, 12, 198, 199, 200]
    Extracted mask:  [  1   4   8  10  11  12 198 199 200]
    Saved mask:  1 4 8 10-12 198-200
    

.. categories::

.. sourcelink::
