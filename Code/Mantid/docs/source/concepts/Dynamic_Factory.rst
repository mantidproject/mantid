.. _Dynamic Factory:

Dynamic Factory
===============

What is it?
-----------

A dynamic factory is a software concept that in instrumental in
implementing the :ref:`Plugin <Plugin>` technology in Mantid.

A factory in software terms is an class that is responsible for creating
other objects on demand. In mantid terms the AlgorithmFactory is
responsible for creating instances of :ref:`Algorithms <Algorithm>` when
you need them.

As the factory is dynamic it does not have a set list of objects that it
can create instead it knows how to create instances of a particular base
class. During execution of the code any object of the appropriate base
class can be registered with the factory, and then the factory can be
used to create fresh instances of that object.



.. categories:: Concepts