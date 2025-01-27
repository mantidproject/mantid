.. _CppCodingStandards:

====================
C++ Coding Standards
====================

.. contents:: Contents
   :local:

References
^^^^^^^^^^

- `LLVM Coding standard <http://llvm.org/docs/CodingStandards.html>`__
- *The C++ Programming Language* (third edition), Bjarne Stroustrup
- `C++ ANSI Standards <http://www.open-std.org/jtc1/sc22/wg21/>`__

Overview
^^^^^^^^

Mantid follow the LLVM C++ coding standards and standards on this
page. Where these standards overlap the standards on this page take
precedence.

`This page
<https://github.com/mantidproject/mantid/wiki/clang-format>`__
describes a tool that you may find useful for checking your code
satisfies LLVM whitespace conventions. It's also useful to set up your
text editor to use `two spaces instead of tabs
<http://llvm.org/docs/CodingStandards.html#use-spaces-instead-of-tabs>`__.

Files
^^^^^

- Header files will use the ``.h`` extension.
- Code body files will use the ``.cpp`` file extension.
- All ``.cpp`` files must have a corresponding ``.h`` file.

Headers
^^^^^^^

Header Files
------------

All header files in a project must have a header comment. In many
cases this will be included as part of the class header, but any
header files that do not contain classes must include the same
information as a file header. The information that must be included
is:

-  class description
-  Copyright
-  GPL license text

Layout of Header File
---------------------

The contents of the header file should be arranged in the following
order:

- ``#pragma once`` header guard to prevent duplicate imports.
- ``#include`` directives in the following order, where each section shold be
  sorted alphabetically:

  - Main Module Header
  - Mantid Headers from the same project
  - Mantid headers from other projects
  - 3rd party library headers
  - System ``#include`` directives
-  Any required namespace(s)
-  Forward declarations of other classes
-  Global constants, enumerations & typedefs
-  Doxygen class description
-  Class definition:

   -  Public constants, enumerations & types
   -  Public constructors & destructors
   -  Public functions & operators
   -  Protected functions
   -  Private constants, enumerations & types
   -  Private functions
   -  Private variables
   -  Bodies of inline functions and (for a template class) member
      functions.

In most cases header files should only contain a single class. The only
exceptions to this should be for internal utility classes, and extremely
small highly related classes such as exception classes.

CPP file Headers
----------------

We will not be including body file headers. The relevant class file
header in the ``.h`` file will suffice.

Function headers
----------------

These will follow the style put forward in the `Doxygen documentation
<http://www.stack.nl/~dimitri/doxygen/docblocks.html>`__.  Headers
comments must be included above all functions definitions and should
describe the function, all parameters and returns values as a minimum.

Where a function is declared in the header file and defined in the
``.cpp`` file the following approach should be taken:

- The header file need only contain documentation for items that do
  not appear in a ``.cpp`` file (e.g. the class itself, member
  variables, inlined methods and templated classes). This keeps the
  header files compact and easier to read. Brief comments can
  optionally be included (as in the example below).

Example header file function declaration

.. code-block:: c++

    /// Creates an instance of an algorithm
    IAlgorithm* createAlgorithm(const std::string& algName, const int& version);

Example CPP file function definition

.. code-block:: c++

    /** Creates and initialises an instance of an algorithm
     *
     * @param algName The name of the algorithm required
     * @param version The version of the algorithm
     * @return A pointer to the created algorithm
     *
     * @throw NotFoundError Thrown if algorithm requested is not registered
     */
     IAlgorithm* FrameworkManagerImpl::createAlgorithm(const std::string& algName, const int& version) {
       IAlgorithm* alg = AlgorithmManager::Instance().create(algName,version).get();
       return alg;
     }

Naming Conventions
^^^^^^^^^^^^^^^^^^

Names should be descriptive and meaningful, but not too long (say <20
characters). This is helped by sensible and consistent (across the
project) use of abbreviations.

- **Constants** (including static const members): All upper
  case. Internal words separated by underscore eg: ``ERROR_NO_DATA``
- **Classes, namespaces, structs, enums (and enum values) and typedefs**:
  PascalCase (First letter upper case, then lower
  case. Internal words begin with upper case letter).
- **Function Names**: function names will be in camelCase (starting
  with a lower case character and capital for each later word).
- **Variable Names / Type Prefixes**: variables will be given sensible
  descriptive names. Type prefixes will not be used. variable names
  in Mantid are usually camelCase (starting with a lower case
  character and capital for each later word).
- **Scope Prefixes**
   - Static member variables use a ``g_`` prefix.
   - Non-static member variables use an ``m_`` prefix.
- **Local variables used as integer loop counters** As an exception,
  these may use very short names like ``i, j, k, l`` eg. ``for (int
  i = 0; i < 5; i++)``

Preprocessor
^^^^^^^^^^^^

1. Use of the pre-processor should be minimised. Constants and type
   aliases should be declared using ``const`` and ``typedef`` rather
   than ``#define``, and functions should be used in favour of macros.
2. ``#include`` statements should use quotes (``""``) for inclusion of
   Mantid code and angle brackets (``<>``) for system files (this
   includes headers from ``Third_Party``)
3. All header files should have guards against repeated inclusion,
   with the guard flags named consistently. (See `here
   <https://en.wikipedia.org/wiki/Include_guard>`__ for an
   explanation)
4. Header files should only include those other header files within
   the project that are necessary (e.g. for definition of the base
   class).  In many cases a forward declaration of the form ``class
   CPidClientObject;`` is sufficient, and this helps to avoid cyclical
   inclusions of headers.
5. It should be possible to compile each header file
   individually. That is, a file consisting solely of ``#include
   "header.h"`` should compile without errors. This avoids
   undocumented interdependencies between headers.

Classes and Namespaces
^^^^^^^^^^^^^^^^^^^^^^

#. There should be only one class or namespace declared in each header
   file. This recommendation should only be relaxed where classes are
   closely tied together.
#. Namespaces should use the C++-17 nested specifier e.g. `namespace A::B{}`
#. There should be only one class or namespace defined per body file
   (unless classes are closely tied as in (1) above). All the
   definitions for that class/namespace should be in the one file
   (unless this yields a source file that is unmanageably large).
#. Data members should be private. Access to data from other classes
   should only be through protected or public methods (or by
   ‘friends’, but see item 8). Inside a large class, consider
   reserving direct access to private data members for a smaller
   manageable core of member functions.
#. All constructors for a class must initialise all its member variables
   that do not have a default constructor (including primitive and
   pointer types).
#. All base classes must have a virtual destructor.

   - This may be disregarded only in exceptional circumstances when
     the overhead of a virtual-table would significantly affect
     performance. Such a class must not be subclassed, and must be
     adequately commented to warn other developers against subclassing
     the class.
   - In addition, it is recommended that where possible, programming
     techniques are used to prevent inheritance of classes with a
     non-virtual destructor. While comments may suffice they can
     easily be ignored or misunderstood, particularly by inexperienced
     developers

#. Classes’ constructors and destructors must not call their own virtual
   member functions, directly or indirectly. (C++ does not resolve such
   function calls polymorphically.)
#. Do not define special members functions when they would be
   identical to those automatically generated by the compiler. Use ``=
   delete`` to remove invalid compiler-generated versions. Consider
   following the `rule-of-zero <https://rmf.io/cxx11/rule-of-zero/>`__
   and writing an additional class for resource management.
#. The use of ``friend`` should be avoided and its use requires
   justification. As an exception, overloads of the ``<<`` and ``>>``
   operators for serialising the class may be declared as ``friend``.
#. Use of multiple inheritance should be restricted to cases in which
   the second and subsequent base classes are all interfaces. (An
   interface in this context is a class consisting only of pure virtual
   functions.).
#. Virtual inheritance should only be used when the base class
   involved is an interface.
#. Unions and bitfields should only be used where essential for
   performance, or where required for interfacing with a third party
   library.

Mantid Namespaces
-----------------

All Mantid code lives within a minimum of a two tiered namespace. The
outer namespace for all Mantid code is Mantid, and this is followed by
a namespace identifying the library that contains the code. Third and
further level namespaces may be used to section code to further
improve readability and maintenance.

Functions and Variables
^^^^^^^^^^^^^^^^^^^^^^^

1. Variables, functions parameters, and function return values must have
   explicit types (no defaulting to ``int``).
2. A function declaration should not use ``void`` to indicate an empty
   parameter list.
3. Parameters in function prototypes should include names, not just
   their types. For example, use ``void eraseRange(int nFirst, int
   nLast);`` rather than ``void eraseRange(int, int);`` as this
   improves self-documentation.
4. Non-static member functions should be declared ``const`` if logically
   they do not alter the state of the class instance.
5. Simple accessor functions may be inline (e.g. ``inline int
   getCount() const { return m_nCount;}``). Otherwise, inline
   functions should be avoided unless essential for performance.
6. Operators should be overloaded sparingly. Operator overloads should
   behave in accordance with the semantics of the operator as applied
   to primitive types, or according to established conventions
   (e.g. ``<<`` and ``>>`` for serialisation).
7. ‘Magic numbers’ must not be used in the code. Constants and
   enumerations must be used instead.

Expressions and Statements
^^^^^^^^^^^^^^^^^^^^^^^^^^

1. Integers should not be cast to Booleans. For example, prefer ``if (x
   != 0)`` rather than ``if(x)``
2. The new style type casting must be used in place of the old C style
   type casts. If casting up or down an inheritance hierarchy, use
   ``dynamic_cast`` (which performs a run-time type check) rather than
   ``static_cast``.
3. Function calls with side effects, and the ``++``/``--``/assignment operators,
   should only be called as a standalone statement rather than embedded
   inside an expression.

   -  It is permissible, although discouraged, to have a function call
      with side effects as the right-operand of ``&&`` or ``||``. Any such
      instances must be commented in detail to alert other developers to
      the fact that the function is not always called.
4. A ``for`` loop should only have one control variable, and should
   not modify it in the body.
5. ``switch`` statements must include a ``default`` clause, even if
   only to catch errors.
6. Each ``case`` of a ``switch`` statement must either end with a
   ``break``/``return``, or contain a clear comment to alert other
   developers to the fact that execution will fall through to the next
   case. Multiple ``case`` labels (with no code between them) are,
   however, permitted for the same block of code.
7. ``goto`` must be avoided. When there is a need to break out of two
   or more nested loops in one go, the loops should be moved to a
   separate function where 'return' can be used instead.

Comments
^^^^^^^^

1. Sufficient commenting (to the level mandated by this document) of a
   piece of code must be performed at the same time as the code is
   written. It must not be put off until the end of development. When
   code is updated, all relevant comments must be updated as well
   (including those in the header).
2. ‘Dead code’ must not be kept in the source code. (‘Dead code’ here
   means code that has been commented out or unconditionally
   suppressed in some other way, for example using ``#if 0``
   preprocessor directives.)

   -  In the (rare) instances that dead code would serve an important
      documentation purpose for ongoing development, the dead code must
      be placed in an external location and may be referenced from the
      ‘live’ source code.
3. Comments must be indented to the same level as the code to which
   they refer.
4. The language used in comments must be professional and to the
   point.  Flippant or derogatory remarks must be avoided.
5. The collection of comments in a function must, on its own, be
   sufficient that a competent C++ developer can pick up the function
   for subsequent development.
6. Comments on a single line should use ``//`` rather than ``/* … */``.
7. No code block should exceed 20 statement lines without a comment of
   some sort. In general all code should contain 15% comment lines.
8. The style of the comments is not mandated here. However the following
   are general recommendations:

   -  Comments should always be used to describe potential “difficult”
      sections of code utilising, for example, special algorithms
   -  Comments should be used in particular to explain branch conditions
      in ``if ... else`` and ``switch { case ...``-like statements
   - **Comments should be written at a higher level of abstraction
     than the code to which they pertain, rather than merely
     restating it**

Error Handling
^^^^^^^^^^^^^^

The type of error handling needed depends on what the code is
doing. In daemon / service type of program almost nothing may be
allowed to cause the process to terminate, whereas in some utility
programs it may be acceptable to terminate for many error
conditions. This is a design issue and the strictness of application
of the following should take into account the use of the code.

1. The developer should identify all errors that can be generated by a
   function and ensure that they are dealt with appropriately at some
   point in the system. This may be within the function itself or
   higher up in the call stack.
2. All exceptions must be caught and handled properly. (This may
   include terminating the program cleanly, for instance if no more
   memory can be allocated.)
3. Public functions should check their input parameters before using
   them. This checking may be made using the ``assert()`` macro or
   similar mechanism, and so only checked in the debug build, in which
   case comprehensive testing must be performed of the release build.
4. All error status values returned from a function call must be checked
   or explicitly ignored. (To explicitly ignore a function call's return
   value cast it to void, e.g. ``(void) f(a, b);``)
5. When using ``dynamic_cast`` on a pointer, a check must be made that
   the result is not ``null`` (i.e. that the cast was successful).
6. Destructors must not throw any exceptions, directly or indirectly.
   (Exceptions encountered while calling destructors during stack
   unwinding from an earlier exception will cause immediate program
   termination.)
7. Where the language permits it, and where the occurrence of errors
   can be identified at coding time (e.g. opening a file), errors
   should be trapped on an individual basis, rather than using a
   catch-all error handler.
8. Error messages displayed to the user must be understandable and
   informative. They must suggest an action to the user, which will
   resolve the problem, for example:

   -  No further action is required.
   -  Check the XYZ input data and then repeat the process.
   -  Contact the system administrator.

Cppcheck
^^^^^^^^

Cppcheck analyses all C++ code in the project to locate undefined behavior
and poor coding practices. Updates to Cppcheck result in such a large number of
these potential problems being found that it is impractical to solve them all
in the Pull Request where the update takes place.

Suppressions File
-----------------

When Cppcheck is updated, a suppressions file will be created using the
suppressions file generator script, ``generate_cppcheck_suppressions_list.py``.

This file, ``CppCheck_Suppressions.txt.in``, contains a list of all the problems
that Cppcheck found in the codebase. It has two functions:

- Suppress all errors for problems that are known to be entirely or
  almost-entirely false positives.
- Act as a to-do list for any remaining problems that should be fixed when found
  during normal development.

When modifying a ``.cpp`` file, it is likely that changes in line numbers will
cause once-suppressed errors to trigger again. Similar to the rules in
:ref:`CppModernization`, these issues should be fixed as part of the same unit
of work that revealed them.

Suppressing False Positives
---------------------------

Occasionally Cppcheck will incorrectly identify problems that cannot or should
not be fixed.

In order to allow the suppressions file to function as a to-do list of fixes and
to reduce developer time spent looking for solutions that have already been
considered, false positives should be suppressed inline.

.. attention::

   Inline suppressions are a last resort. You should be sure that a suppression
   cannot be fixed by modifying the code before suppressing it using the
   techniques below.

**Suppressing a single line of code:**

.. code-block:: c++

   // cppcheck-suppress arrayIndexOutOfBounds
   sizeFourArray[5] = 0;


**Suppressing multiple errors on a single line:**

.. code-block:: c++

   // cppcheck-suppress [arrayIndexOutOfBounds,zeroDiv]
   sizeFourArray[5] = 1 / 0;


**Suppressing multiple lines:**

.. code-block:: c++

   // cppcheck-suppress-begin arrayIndexOutOfBounds
   sizeFourArray[5] = 0;
   sizeFiveArray[5] = 1;
   // cppcheck-suppress-end arrayIndexOutOfBounds
