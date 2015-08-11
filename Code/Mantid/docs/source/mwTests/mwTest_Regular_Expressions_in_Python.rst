:orphan:

.. testcode:: mwTest_Regular_Expressions_in_Python[4]

   import re
   
   def checkForMatch(checker, test):
       if checker.match(test) != None:
           print 'String matches!'
       else:
           print 'String does not contain a match'
   # End of function definition
   
   checker = re.compile('[a-z]')
   checkForMatch(checker, 'a')  # Prints "String matches!"
   checkForMatch(checker, '9')  # Prints "String does not contain a match"

.. testoutput:: mwTest_Regular_Expressions_in_Python[4]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   String matches!
   String does not contain a match


.. testsetup:: mwTest_Regular_Expressions_in_Python[24]

   import re
   
   def checkForMatch(checker, test):
       if checker.match(test) != None:
           print 'String matches!'
       else:
           print 'String does not contain a match'

.. testcode:: mwTest_Regular_Expressions_in_Python[24]

   checker = re.compile('[a-z]', re.IGNORECASE)
   checkForMatch(checker, 'a')  # Prints "String matches!"
   checkForMatch(checker, 'A')  # Also prints "String matches!"

.. testoutput:: mwTest_Regular_Expressions_in_Python[24]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   String matches!
   String matches!


.. testsetup:: mwTest_Regular_Expressions_in_Python[49]

   filestr  = '''Running 13 tests.............OK!
   Failed 2 of 5 tests
   Running 1 test.OK!
   Running 12 test............OK!
   '''
   filetestsRun = 'testResults.log'
   with open(filetestsRun , 'w') as outfile:
      outfile.write(filestr)

.. testcode:: mwTest_Regular_Expressions_in_Python[49]

   import re
   
   filetestsRun = 'testResults.log'
   f = open(filetestsRun,'r')
   reTestCount = re.compile("Running\\s*(\\d+)\\s*test", re.IGNORECASE)
   reCrashCount = re.compile("OK!")
   reFailCount = re.compile("Failed\\s*(\\d+)\\s*of\\s*(\\d+)\\s*tests", re.IGNORECASE)
   testCount = 0
   failCount = 0
   testsPass = True
   for line in f.readlines():
       m=reTestCount.search(line)
       if m:
           testCount += int(m.group(1))
           m=reCrashCount.search(line)
           if not m:
               failCount += 1
               testsPass = False
       m=reFailCount.match(line)
       if m:
           # Need to decrement failCount because crashCount will
           # have incremented it above
           failCount -= 1
           failCount += int(m.group(1))
           testsPass = False
      
   f.close()
   
   print "Tests Passed: {}".format(testsPass)
   print "Tests Failed: {}".format(failCount)
   print "Total Tests: {}".format(testCount)

.. testcleanup:: mwTest_Regular_Expressions_in_Python[49]

   import os
   os.remove(filetestsRun)

.. testoutput:: mwTest_Regular_Expressions_in_Python[49]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   Tests Passed: False
   Tests Failed: 1
   Total Tests: 26


