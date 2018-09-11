.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The algorithm extracts list of the spectra numbers which are masked on the input workspace and,
if requested, saves these spectra as legacy *.msk* file in ISIS ASCII format.

The format is described on :ref:`LoadMask <algm-LoadMask>` algorithm pages.

**Note** It is not recommended to use exported *.msk* files for masking workspaces during reduction as
the result of applying such mask will be different depending on the stage of reduction of the source and target files.
You need to make additional efforts (see :ref:`LoadMask <algm-LoadMask>` algorithm description) to avoid this effect.
It is better to use :ref:`SaveMask <algm-SaveMask>` algorithm instead. This algorithm produces *.xml* files, 
containing lists of masked detectors, which will produce identical masking regardless of the workspace grouping and
spectra-detector map, defined for data acquisition electronics during experiment.

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
