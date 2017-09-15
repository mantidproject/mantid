.. algorithm::

.. summary::

.. alias::

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
      print 'energies:\n', en
      # a complex-valued matrix with wave functions
      print 'wave functions:\n', wf
      # a complex-valued matrix with the Hamiltonian
      print 'Hamiltonian:\n', ham

.. testoutput::

     energies:
     [  0.00000000e+00   4.97379915e-14   2.93261118e+01   2.93261118e+01
        4.43412485e+01   4.43412485e+01]
     wave functions:
     [[ -6.93889390e-17+0.j  -3.38877312e-01+0.j   6.80756336e-01+0.j
        -6.25169857e-01+0.j   1.74683643e-01+0.j  -1.95256823e-02+0.j]
      [  5.42213428e-01+0.j  -7.97569062e-17+0.j  -2.33045938e-01+0.j
        -2.53767032e-01+0.j   8.51307403e-02+0.j   7.61609638e-01+0.j]
      [  5.55111512e-17+0.j   7.68873700e-01+0.j   1.21082286e-01+0.j
        -1.11195433e-01+0.j   6.14081735e-01+0.j  -6.86404558e-02+0.j]
      [ -7.68873700e-01+0.j   6.77450070e-17+0.j   1.11195433e-01+0.j
         1.21082286e-01+0.j   6.86404558e-02+0.j   6.14081735e-01+0.j]
      [ -5.55111512e-17+0.j  -5.42213428e-01+0.j  -2.53767032e-01+0.j
         2.33045938e-01+0.j   7.61609638e-01+0.j  -8.51307403e-02+0.j]
      [  3.38877312e-01+0.j  -2.36325596e-17+0.j   6.25169857e-01+0.j
         6.80756336e-01+0.j   1.95256823e-02+0.j   1.74683643e-01+0.j]]
     Hamiltonian:
     [[  1.86648000+0.j   0.00000000+0.j   9.27182972+0.j   0.00000000+0.j
        -3.36590841+0.j   0.00000000+0.j]
      [  0.00000000+0.j   4.96692000+0.j   0.00000000+0.j  19.33604706+0.j
         0.00000000+0.j  -3.36590841+0.j]
      [  9.27182972+0.j   0.00000000+0.j  -6.83340000+0.j   0.00000000+0.j
        19.33604706+0.j   0.00000000+0.j]
      [  0.00000000+0.j  19.33604706+0.j   0.00000000+0.j  -6.83340000+0.j
         0.00000000+0.j   9.27182972+0.j]
      [ -3.36590841+0.j   0.00000000+0.j  19.33604706+0.j   0.00000000+0.j
         4.96692000+0.j   0.00000000+0.j]
      [  0.00000000+0.j  -3.36590841+0.j   0.00000000+0.j   9.27182972+0.j
         0.00000000+0.j   1.86648000+0.j]]
      
Please note that this area is under active development and any name can be changed in the future.

.. categories::

.. sourcelink::
