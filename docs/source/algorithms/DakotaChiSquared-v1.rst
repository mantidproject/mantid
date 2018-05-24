.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Compare two nexus files containing matrix workspaces and output chi
squared into a file for use with the Dakota program.

Usage
-----

.. testcode:: DakotaChiSquared
    
    #We need to create some files
    import mantid,os.path
    datafile=os.path.join(mantid.config.getString('defaultsave.directory'),'DakotaChiSquared_data.nxs')
    simfile=os.path.join(mantid.config.getString('defaultsave.directory'),'DakotaChiSquared_sim.nxs')
    chifile=os.path.join(mantid.config.getString('defaultsave.directory'),'DakotaChiSquared_chi.txt')
	
    CreateWorkspace(OutputWorkspace='data',DataX='1,2,3,4,5',DataY='1,0,1,4,4',DataE='1,1,1,2,2')
    CreateWorkspace(OutputWorkspace='sim',DataX='1,2,3,4,5',DataY='1,1,1,1,1',DataE='0,0,0,0,0')
    SaveNexus('data',datafile)
    SaveNexus('sim',simfile)
	
    #clean up the workspaces
    DeleteWorkspace("data")
    DeleteWorkspace("sim")
	
    #run the algorithm
    result=DakotaChiSquared(datafile,simfile,chifile)
    
    #Test to see if everything is ok
    print("Chi squared is {}".format(result[0]))
    print("Residuals are {}".format(result[1].dataY(0)))
    #and the content of the file
    f = open(chifile,'r')
    chistr=f.read()
    print("Content of the file : {}".format(chistr.strip()))
    f.close()
	
.. testcleanup:: DakotaChiSquared

    DeleteWorkspace(result[1])  
    import mantid,os.path
    datafile=os.path.join(mantid.config.getString('defaultsave.directory'),'DakotaChiSquared_data.nxs')
    simfile=os.path.join(mantid.config.getString('defaultsave.directory'),'DakotaChiSquared_sim.nxs')
    chifile=os.path.join(mantid.config.getString('defaultsave.directory'),'DakotaChiSquared_chi.txt')
    os.remove(datafile)
    os.remove(simfile)
    os.remove(chifile)
     
.. testoutput:: DakotaChiSquared    
    
    Chi squared is 5.5
    Residuals are [ 0.  -1.   0.   1.5  1.5]
    Content of the file : 5.5 obj_fn



.. categories::

.. sourcelink::
