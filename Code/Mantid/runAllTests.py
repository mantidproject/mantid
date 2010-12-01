#!/usr/bin/env python

# This script will launch all (debug) tests in parallel

from multiprocessing import Pool
import os

def runtest(folder):
    print "Launching ", folder
    real_folder = folder
    files = ""
    if folder == "Algorithms1":
        real_folder = "Algorithms"
        #Run tests for the tests starting with these letters
        files = ['%s*.h' % chr(x) for x in range(65, 65+10)]
        files = '1 ' + ' '.join(files)
    elif folder == "Algorithms2":
        real_folder = "Algorithms"
        #Run tests for the tests starting with these letters
        files = ['%s*.h' % chr(x) for x in range(65+10, 65+27)]
        files = '2 ' + ' '.join(files)
                         
    os.system('rm %s.out' % folder)
    f = open('./%s.out' % folder, 'w')
    f.write("----------------------------------------------\n") 
    f.write("----------------------------------------------\n") 
    f.write("---------- %s -------------------\n" % folder) 
    f.write("----------------------------------------------\n") 
    f.write("----------------------------------------------\n")
    f.close()
    os.system("cd %s/test ; ./runTests.sh %s >> ../../%s.out 2> ../../%s.err " % ( real_folder, files, folder, folder) )
    print "... completed: ", folder, "..."
    

if __name__ == '__main__':
    p = Pool(6)
    #Best to put the slowest tests at the front...
    simple_folders = ['Nexus',  'CurveFitting', 'DataHandling', 
                 'DataObjects', 'API', 'Kernel', 'PythonAPI', 'MDDataObjects', 'MDAlgorithms']
    # Algorithms will get split in half
    arguments = ['Algorithms1', 'Algorithms2' ] + simple_folders
    p.map(runtest, arguments)
    
    #Compile the outputs
    arguments.sort()
    os.system("rm test_results.txt");
    for folder in arguments:
        os.system('cat %s.out >> test_results.txt ' % folder)
        os.system('echo "--- stderr output ---" >> test_results.txt ')
        os.system('cat %s.err >> test_results.txt ' % folder)
                       
    #show it!
        
    import MantidBuild
    MantidBuild.color_output("cat test_results.txt", '.')
