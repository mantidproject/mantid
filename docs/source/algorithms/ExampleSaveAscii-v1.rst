.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This example python algorithm saves a workspace to file in ascii format. The
algorithm exists for demonstration purposes only; ordinarily SaveAscii should
be used instead.

Inputs are a filename and a workspace.
The output is a file in the default save directory with the input filename and
containing data from the input workspace in ascii format. Bin centers, rather
than bin boundaries, are saved.

.. _exsaveascii-usage:

Usage
-----

**Example - save example workspace:**

.. testcode:: ExSaveAscii

    # Load sample workspace
    temp_ws_ = CreateSampleWorkspace()

    # Use algorithm to save workspace
    filename = 'ExSaveAsciiFile.txt'
    ExampleSaveAscii(temp_ws_, filename)

    # Get the default save path
    workdir = config['defaultsave.directory']
    filepath = os.path.join(workdir, filename)
    
    # Read and print first 3 lines from file
    with open(filepath, 'r') as f:
        print(f.readline()[:-1])
        print(f.readline()[:-1])
        print(f.readline()[:-1])
    
    # Delete the test file
    os.remove(filepath)

Output:

.. testoutput:: ExSaveAscii

    # Time-of-flight , Spectrum , E
    1
    100.0000,0.3000,0.5477

.. categories::

.. sourcelink::
