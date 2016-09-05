.. algorithm::

.. summary::

.. alias::

.. properties::

Description
------------

Given simulated s(q,e) data workspace and a reference OSIRIS s(q,e) workspace,
interpolates the simulated data onto the same (q, e) grid as the reference workspace.
This allows direct comparison between simulated and experimental data upon the
same axes. Currently only supports OSIRIS experimental data.

Usage
-----

**Example 1 - Interpolate a simulated data set onto the axes of an experimental
set**

.. code-block:: python

    #create a simulated S(Q,E) workspace of a sin function
    x_data = np.arange(-2., 2., 0.05)
    q_data = np.arange(0.5, 1.3, 0.1)
    y_data = np.asarray([val*(np.cos(5*x_data)+1) for val in q_data])
    y_data = y_data.flatten()
    x_data = np.tile(x_data, len(q_data))
    sim_ws= CreateWorkspace(DataX=x_data, DataY=y_data, NSpec = len(q_data),
                            VerticalAxisUnit='MomentumTransfer',
                            VerticalAxisValues=q_data)
    #create an empty OSIRIS workspace (this would be your experimental OSIRIS resolution function)
    idf_dir = config['instrumentDefinition.directory']
    osiris = LoadEmptyInstrument(idf_dir + 'OSIRIS_Definition.xml')
    osiris = CropWorkspace(osiris,  StartWorkspaceIndex=970, EndWorkspaceIndex=980)
    osiris = Rebin(osiris, [-0.6, 0.02, 0.6])
    #interpolate the two workspaces
    interpolated_ws = NMoldyn4Interpolation(sim_ws, ref_ws)
    print 'No. of Q-values in simulation = ' + str(sim_ws.getNumberHistograms())
    print 'No. of Q-values in reference = ' + str(osiris.getNumberHistograms())
    print 'No. of Q-values in interpolated set = '+ str(interpolated_ws.getNumberHistograms())

Output:

.. code-block:: python

    No. of Q-values in simulation = 20
    No. of Q-values in reference = 42
    No. of Q-values in interpolated set = 42

**Example 2 - Convolution of a simulation and a resolution function**

Convolution initially fails due to being on different axes:

.. code-block:: python

    #create a simulated S(Q,E) workspace of a sin function
    x_data = np.arange(-2., 2., 0.05)
    q_data = np.arange(0.5, 1.3, 0.1)
    y_data = np.asarray([val*(np.cos(5*x_data)+1) for val in q_data])
    y_data = y_data.flatten()
    x_data = np.tile(x_data, len(q_data))
    sim_ws= CreateWorkspace(DataX=x_data, DataY=y_data, NSpec = len(q_data),
                            VerticalAxisUnit='MomentumTransfer',
                            VerticalAxisValues=q_data)
    #create an empty OSIRIS workspace (this would be your experimental OSIRIS resolution function)
    idf_dir = config['instrumentDefinition.directory']
    osiris = LoadEmptyInstrument(idf_dir + 'OSIRIS_Definition.xml')
    osiris = CropWorkspace(osiris,  StartWorkspaceIndex=970, EndWorkspaceIndex=980)
    osiris = Rebin(osiris, [-0.6, 0.02, 0.6])

    output_ws = ConvolveWorkspaces(Workspace1=sim_ws, Workspace2=osiris)

Output:

.. code-block:: python

    Error in execution of algorithm ConvolveWorkspaces:
    Size mismatch

But using interpolation, one can convolve the two workspaces:

.. code-block:: python

    #interpolates the two datasets
    interpolated_ws = NMoldyn4Interpolation(InputWorkspace=sim_ws, ReferenceWorkspace=osiris)
    #convolves the two workspaces
    output_ws = ConvolveWorkspaces(Workspace1=sim_ws, Workspace2=interpolated_ws)
    print 'No. of Q-values in simulation = '+str(sim_ws.getNumberHistograms())
    print 'No. of Q-values in resolution function = '+str(ref_ws.getNumberHistograms())
    print 'No. of Q-values in interpolated set = '+str(interpolated_ws.getNumberHistograms())
    print 'No. of Q-values in convolved set = '+str(output_ws.getNumberHistograms())

And a convolved workspace with the same shape as the resolution function is produced:

.. code-block:: python

    No. of Q-values in simulation = 20
    No. of Q-values in resolution function = 42
    No. of Q-values in interpolated set = 42
    No. of Q-values in convolved set = 42

.. categories::

.. sourcelink::
