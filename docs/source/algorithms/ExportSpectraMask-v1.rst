.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The algorithm extracts list of the spectra numbers which are masked on the input workspace and,
if requested, saves these spectra as legacy *.msk* file in ISIS ASCII format.

The format is described on :ref:`LoadMask <algm-LoadMask>` algorithm pages.

Usage
-----

**Example: Export Spectra Masks**

.. testcode:: ExExportSpectraMask

    import sys
    #prepare the data:
    masks = [1,4,8,10,11,12,12,198,199,200]
    test_ws = CreateSampleWorkspace()
    test_ws.maskDetectors(masks)
    
    f_name = 'test_ws_mask.msk'
    
    #extract mask:
    r_masks = ExportSpectraMask(test_ws,Filename=f_name)
    
    # Compare results:
    wmsk = ''
    final_fname = os.path.join(config.getString('defaultsave.directory'),f_name) 
    with open(final_fname,'r') as res_file:
        for line in res_file:
            wmsk = line
    os.remove(final_fname)
    
    sys.stdout.write("Input mask: {0}\n".format(masks))
    sys.stdout.write("Extracted mask: {0}\n".format(r_masks))
    sys.stdout.write("Saved mask: {0}".format(wmsk))


Output:

.. testoutput:: ExExportSpectraMask

    Input mask: [1, 4, 8, 10, 11, 12, 12, 198, 199, 200]
    Extracted mask: [  1   4   8  10  11  12 198 199 200]
    Saved mask: 1 4 8 10-12 198-200

.. categories::

.. sourcelink::
