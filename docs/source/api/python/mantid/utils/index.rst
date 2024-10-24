.. _mantid.utils:

====================
 :mod:`mantid.utils`
====================

This module is intended to contain code that can be shared between
algorithms, such as helper functions and constants. Additionally, it
can be used as a library to pull internal functions from an algorithm
so that the code can be shared with external software needing the
same functionality without the need to duplicate code.

Unit tests for functions in this module should be placed in the
``Framework/PythonInterface/test/python/mantid/utils/`` directory and
added to its ``CMakeLists.txt`` to make sure the testing code gets
registered with Mantid.


Calling functions from ``utils``
================================

To use functions defined within the ``mantid.utils`` module, the
particular file can be imported and used similarly to the other
python modules. The code below shows an example usage for importing
and calling a function within a hypothetical file. The following
applies to accessing inside another algorithm, as well as within
from Mantid Workbench.

.. code-block:: python

    from mantid.utils import examplelibrary

    # Call a stand-alone function defined in examplelibrary:
    result = examplelibrary.do_something()

A basic real-world example is shown below to compute absorption
correction on data using functions extracted from
:ref:`SNSPowderReduction <algm-SNSPowderReduction>` so they
can be called from different algorithms.


.. code-block:: python

    from mantid.kernel import PropertyManagerDataService
    from mantid.simpleapi import Load, PDLoadCharacterizations, PDDetermineCharacterizations
    from mantid.utils import absorptioncorrutils

    # Load characterization file
    char_files = ["PG3_char_2020_01_04_PAC_limit_1.4MW.txt", "PG3_char_2020_05_06-HighRes-PAC_1.4_MW.txt"]
    charfile = ','.join(char_files)
    charTable = PDLoadCharacterizations(Filename=charfile)
    chars = charTable[0]

    data = Load(Filename="PG3_46577.nxs.h5", MetaDataOnly=True)

    PDDetermineCharacterizations(InputWorkspace=data,
                                 Characterizations=chars,
                                 ReductionProperties="props")

    props = PropertyManagerDataService.retrieve("props")

    # Sample only absorption correction
    abs_sample, _ = absorptioncorrutils.calculate_absorption_correction(
        "PG3_46577.nxs.h5",  # input filename
        "SampleOnly",        # absorption correction method
        props,               # PropertyManager
        "Si",                # sample_formula
        1.165,               # mass_density
        element_size=2,      # integration element cube in mm
        cache_dir="/tmp",    # cache diretory for speeding up repeated calculation
        )
