.. _MantidStandards:

================
Mantid Standards
================

These standards relate specifically to the implementation of the
Mantid framework and the MantidPlot application.

.. contents:: Contents
   :local:

General Notes on Naming
^^^^^^^^^^^^^^^^^^^^^^^

The items described on this page form part of the public interface to
Mantid, which means that their names are hard to change once they are
out in the wild. As naming is one of the most difficult tasks to `do
well <http://martinfowler.com/bliki/TwoHardThings.html>`_, it is
important to take time and care when choosing a name. Some guidance on
naming in a more code-related context can be found `here
<http://blog.codinghorror.com/i-shall-call-it-somethingmanager/>`_ but
the advice should still be applicable to naming Mantid concepts.

Algorithms
^^^^^^^^^^

Standards to follow when implementing an `algorithm
<http://docs.mantidproject.org/nightly/concepts/Algorithm.html>`_ in
both C++ and Python.

Naming
------

Algorithm names start with a capital letter and have a capital letter
for each new word, with no underscores. Use alphabet and numeric
characters only. Numbers are only allowed after the first character.

Names should be descriptive but not too long (max 20 chars). If
possible, avoid abbreviations unless they are common and well
understood. To avoid a proliferation of different synonyms for
algorithms that have a common goal, e.g. Create... vs Generate..., we
standardise on a set of prefixes for common tasks:

+-----------------------------------------------------------------------+------------------+--------------------+
| Task                                                                  | Preferred Prefix | Example            |
+=======================================================================+==================+====================+
| Creating a new object, e.g. workspace, with exception of file loaders | Create           | CreateMDWorkspace  |
+-----------------------------------------------------------------------+------------------+--------------------+
| Loading a file                                                        | Load             | LoadMappingTable   |
+-----------------------------------------------------------------------+------------------+--------------------+
| Applying a value to an existing object, e.g set UB matrix             | Set              | SetUB              |
+-----------------------------------------------------------------------+------------------+--------------------+
| Retrieve a value, e.g. Ei                                             | Get              | GetDetectorOffsets |
+-----------------------------------------------------------------------+------------------+--------------------+
| Adding a new item to an existing list                                 | Add              | AddSampleLog       |
+-----------------------------------------------------------------------+------------------+--------------------+
| Search for something, e.g. peaks                                      | Find             | FindPeaks          |
+-----------------------------------------------------------------------+------------------+--------------------+

Categories
----------

Plain english using `Title Case
<http://www.grammar-monster.com/lessons/capital_letters_title_case.htm>`_. Connecting
words should have lower case first letters. Use alphabet characters
only, numbers are not allowed, e.g. Muon or SANS.

Properties
----------

Property names start with a capital letter and have a capital letter
for each new word, with no underscores. Use alphabet and numeric
characters only. Numbers are only allowed after the first character.

Wherever possible and unambiguous, the primary input workspace should
be called ``InputWorkspace`` and the primary output workspace should
be called ``OutputWorkspace``. An algorithm with a single In/Out
workspace should name its property ``Workspace``. Certain groups of
algorithms have other standards to adhere to.

Fit Functions
^^^^^^^^^^^^^

Standards to following when implementing a fitting function (both C++
& Python).

Naming
------

Function names start with a capital letter and have a capital letter
for each new word, with no underscores. Use alphabet and numeric
characters only. Numbers are only allowed after the first character.


Categories
----------

Plain english using `Title Case
<http://www.grammar-monster.com/lessons/capital_letters_title_case.htm>`_. Connecting
words should have lower case first letters. Numbers are not allowed.

Parameters
----------

Parameter names must:

- Start with a capital letter
- Have a capital letter for each new word (e.g. 'InputWorkspace')
- Use alphanumeric characters only (i.e. cannot contain any of these ``/,._-'\"`` or whitespace)
- Can contain numbers but only allowed after the first character.

Notable exceptions to these rules are lattice constants (i.e. a, b, c,
alpha, beta, gamma).

Workspace Names
^^^^^^^^^^^^^^^

No firm restrictions. The use of two underscores as a prefix will mark
the workspace as hidden. It is recommended to use only the alphabet,
numeric and the underscore characters.
