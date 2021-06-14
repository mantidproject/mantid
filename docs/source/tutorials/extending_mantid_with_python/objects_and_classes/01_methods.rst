.. _01_methods:

=======
Methods
=======

Methods are essentially normal functions which must be called using that
object and always have at least 1 argument. They are defined with
the ``def`` keyword but they are "attached" to an object.

The first argument to the method is always treated as a reference to the
object that called the method and the standard convention is to name the
variable ``self``.

Python puts the ``self`` argument in automatically. For example, we can
add a ``sleep()`` method to our ``Person`` class:

.. code-block:: python

    class Person(object):

        name = "Bob"

        def sleep(self):
            print(self.name + ' is sleeping')

The method can only be used through an object so one must have been constructed
first, e.g.

.. code-block:: python

    person1 = Person()
    # Note: We call it with no arguments, Python inserts self automatically.
    # We use parentheses because sleep is a function
    person1.sleep()

Other than the ``self`` argument, methods behave in the same manner as
standard functions and you can take any additional arguments required:

.. code-block:: python

    class Person(object):

        name = "Bob"

        def sleep(self, nhours):
            print(self.name + ' is sleeping for ' + str(nhours) + ' hours')

    ##################################################################
    person1 = Person()
    # Called with 1 argument, Python inserts the self automatically.
    person1.sleep(7)

Special Method: __init__
========================

Python has many so-called special methods whose names are both prefixed and
suffixed with a double underscore.

The most useful of these is named ``__init__``. If present, this method is
automatically called by Python when the object is constructed, i.e. when
``Person()`` is called in our example.

The method is used to initialize an object and can take parameters, for
example we can set the name to something passed in by the user:

.. code-block:: python

    class Person(object):
        def __init__(self, name):
            # Add a 'name' field to the object and assign a value to it.
            self.name = name

        def sleep(self):
            print(self.name +' is sleeping')

    ###################################################

    # Usage:
    person1 = Person("Bob")
    person2 = Person("Alice")

    person1.sleep() # => prints "Bob is sleeping"
    person2.sleep() # => prints "Alice is sleeping"

This is the preferred way of initializing an object as the object is
then in a known good state from the start.
