#!/usr/bin/env python

# This script will launch all (debug) tests in parallel

from multiprocessing import Pool
import os

def runtest(folder):
    print "Launching ", folder
    os.system('cd %s/test ; rm stdout.txt' % folder)
    f = open('%s/test/stdout.txt' % folder, 'w')
    f.write("----------------------------------------------\n") 
    f.write("----------------------------------------------\n") 
    f.write("---------- %s -------------------\n" % folder) 
    f.write("----------------------------------------------\n") 
    f.write("----------------------------------------------\n")
    f.close()
    os.system("cd %s/test ; ./runTests.sh >> stdout.txt 2> stderr.txt " % folder )
    print "... completed: ", folder, "..."
    

if __name__ == '__main__':
    p = Pool(6)
    #Best to put the slowest tests at the front...
    folders = ['Algorithms', 'Nexus',  'CurveFitting', 'DataHandling', 
                 'DataObjects', 'API', 'Kernel', 'PythonAPI']
    p.map(runtest, folders)
    
    #Compile the outputs
    folders.sort()
    os.system("rm test_results.txt");
    for folder in folders:
        os.system('cat %s/test/stdout.txt >> test_results.txt ' % folder)
                       
    #show it!
        
    import MantidBuild
    MantidBuild.color_output("cat test_results.txt", '.')
