.. _more_sequence_types:

===================
More Sequence Types
===================

Tuples
======

-  A tuple is similar to a list except that once a tuple has been
   created its value cannot be changed, only a new tuple created in its
   place. In this manner it can be thought of as a read-only list.

-  They are created by enclosing the required sequence by parentheses
   ``()``,

.. code:: python

   lottery_numbers = (1,2,3,4,5,6)

-  Most list operations are still supported; index lookup, slicing, etc.
   However, these operations can only be used in a read-only sense, e.g.

.. code:: python

   print(lottery_numbers[0])     # prints 1
   print(lottery_numbers[1:3])   # prints (2,3)
   # Assignment not allowed
   lottery_numbers[3] = 42      # gives error "TypeError: 'tuple' object does not support item assignment"

-  Tuples are most useful when returning values from functions as they
   allow returns of more than 1 item as will be shown later.

-  A meaning can be assigned to each position of the tuples through the use of the ``namedtuple`` type. This leads to unambiguous
   tuple assignment and creates self-documenting code. For instance, consider a tuple representing a geometric point:

.. code:: python

   from collections import namedtuple
   Point = namedtuple('Point', ['x', 'y', 'z'])
   p = Point(x=2, y=3, z=4)
   print(p.x, p[0]) # elements can be accessed using the field name or by index

- The meaning of the import ``from collections import nametuple`` will be outlined further on in this tutorial.

Dictionaries
============

-  Python contains a mapping type known as a *dictionary*.

-  The order of items in a dictionary is not user-defined (out of the
   box), i.e you cannot access a dictionary by index.

-  Instead a dictionary maps a key to a value so that look up is by key,
   which may be a number but it is not limited to this, and not an index
   position.

-  A dictionary is created using braces ``{}`` and can be created empty
   or initialized with elements. If initial elements are required then
   each key/value pair should be specified using ``key:value`` syntax
   and then each separated with a comma, e.g.

-  Accessing a value is done by using square brackets where the argument
   is the key, e.g.

 .. testcode:: dict1

   empty_dict = {}      # Empty dictionary
   my_lookup = {'a' : 1, 'b' : 2} # A dictionary with two keys, each
                                  # mapped to the respective value
   print(my_lookup['b'])

Gives the output:

.. testoutput:: dict1

    2

-  Trying to retrieve a key that does not exist results in a *KeyError*
   and the program will halt,

.. code:: python

   empty_dict['a']   # Results in "KeyError: 'a'"

-  Unlike tuples, dictionaries can be updated with new values, simply
   use the square brackets on the left-hand side of a assignment

-  This syntax can also be used to replace a value that is already in
   the dictionary since every key has to be unique,

.. code:: python

   empty_dict = {}      # Empty dictionary
   my_lookup = {'a' : 1, 'b' : 2} # A dictionary with two keys
   empty_dict['a'] = 1
   my_lookup['b'] = 3   # Replaces the value that was referenced by the key 'b' with the new value 3
   print(empty_dict['a'], my_lookup['b'])

-  To remove a key/value from the dictionary, use the ``del`` command

.. code:: python

   del my_lookup['b']   # Removes the key/value pair with the specified key
   my_lookup.clear()   # Empties the dictionary

-  As a dictionary's order is undefined it is not possible to use
   slicing syntax as with lists and tuples.

Sets
====

-  Sets are another unordered sequence of elements but unlike
   dictionaries, sets do not map keys to values instead they simply
   store a unique group of values.

-  Unlike the other sequence types there is no special syntax for
   creating a set, there is instead the ``set()`` or ``frozenset()``
   function. The difference simply corresponding to whether the
   structure is marked read-only after creation, where the ``frozenset``
   is the read-only structure.

-  To create a set simply pass a list or tuple to the ``set()``
   function,

-  Changing elements in a set is accomplished with the ``add()`` or
   ``remove()`` functions,

.. code:: python

   values = set([1,1,3])
   print(values)
   values.add(4)
   values.remove(1)
   print(values)

Gives the output:

.. code:: python

    set([1, 3])
    set([3, 4])

-  As with dictionaries, sets are unordered so it is not possible to
   access a set with a square bracket operators and they do not support
   slicing

Common Operations
=================

-  All sequence types support a number of common operations: ``len()``,
   ``x in s`` and ``x not in s``.

-  ``len()`` gives the length of the sequence passed as its argument.

-  ``x in s`` returns ``True`` if x is a member of the sequence s.

-  ``x not in s`` returns ``True`` if x is not a member of the sequence
   s.

-  Examples:

.. testcode:: operations1

   s = [1,2,3,4,5,6]      # Also works with all other sequence types
   print(len(s))

   test = 3 in s
   print(test)
   test = 7 not in s
   print(test)

Gives the output:

.. testoutput:: operations1

    6
    True
    True
