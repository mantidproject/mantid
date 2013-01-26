#
# File: ReduceDictionary.py
#
# This module contains some top level methods for constructing a python
# dictionary with the parameter values needed by the ReduceOneSCD_Run.py
# and ReduceSCD_Parallel.pay scripts.
#
# This method will load the dictionary of parameter name, value pairs from 
# the specified configuration file.  The key words None, True and False are
# converted from a string to the corresponding value.  Also, the list of
# run numbers is built up from possibly multiple lines with the key "run_nums".
# The run numbers themselves may be specified as a comma separated list of
# individual run numbers, or ranges specified with a colon separator.
#
def LoadDictionary( filename ):
  params_dictionary = {}
  run_nums = []
  file = open(filename)
  for line in file:
    line = line.strip();
    line = line.rstrip();
    if (not line.startswith('#')) and len(line) > 2:
      words = line.split()
      if len(words) > 1:
        if words[1] == "None":
          params_dictionary[words[0]] = None
        elif words[1] == "True":
          params_dictionary[words[0]] = True
        elif words[1] == "False":
          params_dictionary[words[0]] = False
        elif words[0] == "run_nums":
          run_list = ParseRunList(words[1])
          for i in range(0,len(run_list)):
            run_nums.append(run_list[i])
        else:
          params_dictionary[words[0]] = words[1]
      else:
        print "Syntax Error On Line: " + line

    params_dictionary["run_nums"]=run_nums
  return params_dictionary;

#
# Return a list of run numbers from a string containing a comma separated
# list of individual run numbers, and/or ranges of run numbers specified 
# with a colon separator.
#
def ParseRunList( run_string ):
  run_list = []
  groups = run_string.split(",")
  for group in groups:
    runs = group.split(":")
    if len(runs) == 1:
      run_list.append( runs[0] )
    else:
      first = int(runs[0])
      last  = int(runs[1])
      for run in range(first, last+1):
        run_list.append(str(run)) 

  return run_list

