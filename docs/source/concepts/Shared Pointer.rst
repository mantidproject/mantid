.. _Shared Pointer:

Shared Pointer
==============

What are they?
--------------

Shared pointers are used extensively within the Mantid Framework to
simplify memory management and reduce memory leaks. We use the Shared
Pointer `definition from the Boost
library <http://www.boost.org/doc/libs/1_35_0/libs/smart_ptr/smart_ptr.htm>`__.

Shared pointers are objects which store pointers to dynamically
allocated (heap) objects. They behave much like built-in C++ pointers
except that they automatically delete the object pointed to at the
appropriate time. Shared pointers are particularly useful in the face of
exceptions as they ensure proper destruction of dynamically allocated
objects. They can also be used to keep track of dynamically allocated
objects shared by multiple owners.

Conceptually, Shared pointers are seen as owning the object pointed to,
and thus responsible for deletion of the object when it is no longer
needed.

Declaring a shared pointer
--------------------------

creating a shared pointer to a new object

``boost::shared_ptr``\ \ `` ptr(new C);``

assigning a shared pointer

``boost::shared_ptr``\ \ `` instrument = workspace->getInstrument();``

Several of our shared pointers have typedefs to give them much shorter
definitions. For example instead of boost::shared\_ptr you can just type
workspace\_sptr (where sptr stands for shared pointer).

Using a shared pointer
----------------------

Shared pointer can be used just like any pointer.

``workspacePointer->readX(1);``

The only real differences are when casting the pointer instead of

``Workspace2D* input2D = dynamic_cast``\ \ ``(m_input);``

you would use

``Workspace2D_sptr input2D = boost::dynamic_pointer_cast``\ \ ``(input);``

and that you should not delete a shared pointer, it will take care of
itself.



.. categories:: Concepts