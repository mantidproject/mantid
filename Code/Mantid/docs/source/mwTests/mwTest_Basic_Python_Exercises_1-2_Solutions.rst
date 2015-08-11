:orphan:

.. testcode:: mwTest_Basic_Python_Exercises_1-2_Solutions[3]

   # Write a program that prints out the square of the first
   # 20 integers a block such that the block has a dimension of 4x5.
   
   # range(i,j) produces a list of numbers from i -> j-1
   for i in range(1,21):
       print str(i*i).center(3), # center is a function for strings that centers the contents to the given width
       if i % 4 == 0:
           print
   
   # ------------ Produces -------------
   # 1   4   9   16
   # 25  36  49  64
   # 81 100 121 144
   #169 196 225 256
   #289 324 361 400

.. testoutput:: mwTest_Basic_Python_Exercises_1-2_Solutions[3]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   1   4   9   16
    25  36  49  64
    81 100 121 144
   169 196 225 256
   289 324 361 400


.. testcode:: mwTest_Basic_Python_Exercises_1-2_Solutions[33]

   # prev_2 - One before previous number
   # prev_1 - Previous number
   prev_2, prev_1 = 0, 1
   
   # Already have first 2 terms above
   print prev_2
   print prev_1
   
   # Now the next 23 terms (range(0,23) will run the loop 23 times)
   for i in range(0,23):
       current = prev_2 + prev_1
       print current,
       # Ratio to previous
       print "ratio to previous=",float(current)/prev_1
       # Move the previous markers along one for the next time around
       prev_2 = prev_1
       prev_1 = current

.. testoutput:: mwTest_Basic_Python_Exercises_1-2_Solutions[33]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   0
   1
   1 ratio to previous= 1.0
   2 ratio to previous= 2.0
   3 ratio to previous= 1.5
   5 ratio to previous= 1.66666666667
   8 ratio to previous= 1.6
   13 ratio to previous= 1.625
   21 ratio to previous= 1.61538461538
   34 ratio to previous= 1.61904761905
   55 ratio to previous= 1.61764705882
   89 ratio to previous= 1.61818181818
   144 ratio to previous= 1.61797752809
   233 ratio to previous= 1.61805555556
   377 ratio to previous= 1.61802575107
   610 ratio to previous= 1.61803713528
   987 ratio to previous= 1.61803278689
   1597 ratio to previous= 1.61803444782
   2584 ratio to previous= 1.6180338134
   4181 ratio to previous= 1.61803405573
   6765 ratio to previous= 1.61803396317
   10946 ratio to previous= 1.61803399852
   17711 ratio to previous= 1.61803398502
   28657 ratio to previous= 1.61803399018
   46368 ratio to previous= 1.61803398821


