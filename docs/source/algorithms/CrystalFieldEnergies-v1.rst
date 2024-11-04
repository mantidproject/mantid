.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is not for general use. Its purpose is to expose to python the crystal field calculations implemented in C++.
The algorithm calculates the crystal field energies and wave functions. The example shows how it can be used from python:

.. testcode::

      from CrystalField.energies import energies

      # The first parameter is a code for the rare earth ion
      # The rest of the parameters define the crystal field
      en, wf, ham = energies(1,  B20=0.37737, B22=3.9770, B40=-0.031787, B42=-0.11611, B44=-0.12544)

      # a list of crystal field energies
      print('energies:\n{}'.format(en[2:]))
      # a complex-valued matrix with wave functions
      print('wave functions:\n{}'.format(wf))
      # a complex-valued matrix with the Hamiltonian
      print('Hamiltonian:\n{}'.format(ham))

.. testoutput::
   :options: +ELLIPSIS +NORMALIZE_WHITESPACE

     energies:
     [29.32611185  29.32611185  44.3412485   44.3412485 ]
     wave functions:
     [[...  ...  ...
       ...  ...  ...]
      [...  ...  ...
       ...  ...  ...]
      [...  ...  ...
       ...  ...  ...]
      [...  ...  ...
       ...  ...  ...]
      [...  ...  ...
       ...  ...  ...]
      [...  ...  ...
       ...  ...  ...]]
     Hamiltonian:
     [[ 1.86648   +0.j  0.        +0.j  9.27182972+0.j  0.        +0.j
       -3.36590841+0.j  0.        +0.j]
      [ 0.        +0.j  4.96692   +0.j  0.        +0.j 19.33604706+0.j
        0.        +0.j -3.36590841+0.j]
      [ 9.27182972+0.j  0.        +0.j -6.8334    +0.j  0.        +0.j
       19.33604706+0.j  0.        +0.j]
      [ 0.        +0.j 19.33604706+0.j  0.        +0.j -6.8334    +0.j
        0.        +0.j  9.27182972+0.j]
      [-3.36590841+0.j  0.        +0.j 19.33604706+0.j  0.        +0.j
        4.96692   +0.j  0.        +0.j]
      [ 0.        +0.j -3.36590841+0.j  0.        +0.j  9.27182972+0.j
        0.        +0.j  1.86648   +0.j]]

Please note that this area is under active development and any name can be changed in the future.

.. categories::

.. sourcelink::
