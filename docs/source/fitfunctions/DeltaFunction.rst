.. _func-DeltaFunction:

=============
DeltaFunction
=============

.. index:: DeltaFunction

Description
-----------

DeltaFunction represets the Dirac delta function and can only be used with the :ref:`Convolution <func-Convolution>`:

.. math:: \int\limits_{A}^{B}R(x-\xi) \times \mbox{Height}\times \delta(\xi-\mbox{Centre}) \mbox{d}\xi = \mbox{Height} \times R(x-\mbox{Centre})

Usually it is used as a part of a :ref:`CompositeFunction<func-CompositeFunction>`, for example:

``composite=Convolution; name=Resolution,Filename="Resolution.dat"; (name=DeltaFunction,Height=0.01; name=Lorentzian,Height=1,FWHM=0.1)``

.. attributes::

.. properties::

.. categories::

.. sourcelink::
