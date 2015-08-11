:orphan:

.. testcode:: mwTest_Basic_Python_Exercises_3_Solutions[3]

   # Write a program that builds a list of the first 20 Fibonacci numbers, then
   ## Use the list to print out the value of the ratio of successive numbers of the sequence,
   ##   printing out the final value.
   ## Extend the program so that the Fibonacci list is calculated in a function that takes the
   ##   number of required values as a parameter and returns the list. 
   
   # Function to calculate the first n fibonacci numbers
   # and return them as a list
   def fib(nfibs):
       if nfibs == 0:
           return []
       elif nfibs == 1:
           return [0]
       else:
           pass
       # First two numbers
       fibs = [0,1]
       if nfibs == 2:
           return fibs
       for i in range(2, nfibs):
           fibs.append(fibs[i-2] + fibs[i-1])
   
       return fibs
   
   #### fib ends here ####
   
   # Print out successive ratio remembering that the first number is a zero
   nfibs = 20
   fib_nums = fib(nfibs)
   for i in range(1,nfibs):
       try:
           numerator = fib_nums[i]
           denominator = fib_nums[i-1]
           ratio = float(numerator)/denominator
       except ZeroDivisionError:
           print 'Warning: Invalid ratio: ' + str(numerator) + '/' + str(denominator)
       else:
           print 'Ratio ' + str(numerator) + '/' + str(denominator) + ': ', ratio
   
   ##### Produces #####
   #Warning: Invalid ratio: 1/0
   #Ratio 1/1:  1.0
   #Ratio 2/1:  2.0
   #Ratio 3/2:  1.5
   #Ratio 5/3:  1.66666666667
   #Ratio 8/5:  1.6
   #Ratio 13/8:  1.625
   #Ratio 21/13:  1.61538461538
   #Ratio 34/21:  1.61904761905
   #Ratio 55/34:  1.61764705882
   #Ratio 89/55:  1.61818181818
   #Ratio 144/89:  1.61797752809
   #Ratio 233/144:  1.61805555556
   #Ratio 377/233:  1.61802575107
   #Ratio 610/377:  1.61803713528
   #Ratio 987/610:  1.61803278689
   #Ratio 1597/987:  1.61803444782
   #Ratio 2584/1597:  1.6180338134
   #Ratio 4181/2584:  1.61803405573

.. testoutput:: mwTest_Basic_Python_Exercises_3_Solutions[3]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   Warning: Invalid ratio: 1/0
   Ratio 1/1:  1.0
   Ratio 2/1:  2.0
   Ratio 3/2:  1.5
   Ratio 5/3:  1.66666666667
   Ratio 8/5:  1.6
   Ratio 13/8:  1.625
   Ratio 21/13:  1.61538461538
   Ratio 34/21:  1.61904761905
   Ratio 55/34:  1.61764705882
   Ratio 89/55:  1.61818181818
   Ratio 144/89:  1.61797752809
   Ratio 233/144:  1.61805555556
   Ratio 377/233:  1.61802575107
   Ratio 610/377:  1.61803713528
   Ratio 987/610:  1.61803278689
   Ratio 1597/987:  1.61803444782
   Ratio 2584/1597:  1.6180338134
   Ratio 4181/2584:  1.61803405573


