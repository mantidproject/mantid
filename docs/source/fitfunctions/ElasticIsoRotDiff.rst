.. _func-ElasticIsoRotDiff:

=================
ElasticIsoRotDiff
=================

.. index:: ElasticIsoRotDiff

Description
-----------

This fitting function models the elastic part of the dynamic structure factor
for a particle undergoing continuous and isotropic rotational diffusion [1]_,
:ref:`IsoRotDiff <func-IsoRotDiff>`.

.. math::

   S(Q,E) = Height \cdot j_0(Q\cdot Radius)^2 \delta (E-Centre)

where:

-  :math:`Height` - Intensity scaling, a fit parameter
-  :math:`Q` - Momentum transfer, an attribute (non-fitting)
-  :math:`Radius` - Radius of rotation, a fit parameter
-  :math:`Centre` - Centre of peak, a fit parameter

Because of the spherical symmetry of the problem, the structure factor
is expressed in terms of the :math:`j_l(z)`
`spherical Bessel functions <http://mathworld.wolfram.com/SphericalBesselFunctionoftheFirstKind.html>`__.

.. attributes::

:math:`Q` (double, default=0.3) Momentum transfer

.. properties::

References
----------

.. [1] M. Bee, "Quasielastic Neutron Scattering", Taylor & Francis, 1988.

Usage
-----

**Example - Global fit to a synthetic elastic signal:**

The signal is modeled by the convolution of a resolution function
with the elastic component of a rotator. The resolution is modeled as
a normal distribution. We insert a random noise in the rotator. Finally,
we choose a linear background noise. The goal is to find out the radius
of the rotator and the overal intensity of the signal with a fit to the
following model:

:math:`S(Q,E) = R(Q,E) \otimes ElasticIsoRotDiff(Q,E) + (a+bE)`

.. testcode:: ExampleElasticIsoRotDiff

    import numpy as np
    try:
        from scipy.special import spherical_jn
        def sjn(n, z): return spherical_jn(range(n+1), z)
    except ImportError:
        from scipy.special import sph_jn
        def sjn(n, z): return sph_jn(n, z)[0]
    """Generate resolution function with the following properties:
    1. Normal distribution along the energy axis, same for all Q-values
    2. FWHM = 0.005 meV
    3. Dynamic range = [-0.1, 0.1] meV with spacing 0.0004 meV
    """
    FWHM=0.005
    sigma = FWHM/(2*np.sqrt(2*np.log(2)))
    dE=0.0004  # spacing in the dynamic range
    dataX = np.arange(-0.1,0.1,dE)  # dynamic range
    Emin=min(dataX)
    Emax=max(dataX)
    nE=len(dataX)
    dataY = np.exp(-0.5*(dataX/sigma)**2)  # the resolution function
    Qs = np.array([0.3, 0.5, 0.7, 0.9, 1.1, 1.3, 1.5, 1.9])  # Q-values
    nQ = len(Qs)
    # Workspace containing resolution for each Q, the same in this case
    resolution=CreateWorkspace(np.tile(dataX,nQ), np.tile(dataY,nQ), NSpec=nQ, UnitX="deltaE",
        VerticalAxisUnit="MomentumTransfer", VerticalAxisValues=Qs)

    """Generate a synthetic elastic signal for a particle undergoing isotropic rotational diffusion.
    1. Radius of rotation = 2.5 Angstroms
    2. Up to 10% of noise in the rotator signal
    3. Linear background noise, up to 1% of the intensity
    """
    R=2.5  # Nominal radius, a value to find in the fit
    qdataY=np.empty(0)
    H=2-np.random.random() # global intensity, a value to find in the fit
    for Q in Qs:
        centre=dE*np.random.random()  # some shift along the energy axis
        dataY = np.exp(-0.5*((dataX-centre)/sigma)**2)  # resolution shifted by "centre"
        noise = dataY*np.random.random(nE)*0.1 # noise is up to 10% of the elastic signal
        background = np.random.random()+np.random.random()*dataX  # linear background
        background = (0.01*H*max(dataY)) * (background/max(np.abs(background))) # up to 1% of H
        j0 = sjn(0,Q*R)[0]
        qdataY=np.append(qdataY, H*j0**2*(dataY+noise) + background)
    # Create data workspace
    data=CreateWorkspace(np.tile(dataX,nQ), qdataY, NSpec=nQ, UnitX="deltaE",
        VerticalAxisUnit="MomentumTransfer", VerticalAxisValues=Qs)

    """Now we fit our model to the data workspace. Our model is:
        S(Q,E) = Convolution(resolution, ElasticIsoRotDiff) + LinearBackground
    We do a global fit (all spectra) to find out the radius R and height H
    """
    # Our initial guess are Height=1.0 and Radius=0.98. Here's a template of the
    # model for each spectrum:
    single_model_template="""(composite=Convolution,FixResolution=true,NumDeriv=true;
    name=TabulatedFunction,Workspace=resolution,WorkspaceIndex=_WI_,Scaling=1,Shift=0,XScaling=1;
    name=ElasticIsoRotDiff,Q=_Q_,Height=1,Centre=0,Radius=0.98);
    name=LinearBackground,A0=0,A1=0"""
    # Now create the string representation model for all spectra:
    global_model="composite=MultiDomainFunction,NumDeriv=true;"
    wi=0
    for Q in Qs:
        single_model = single_model_template.replace("_Q_", str(Q))  # insert Q-value
        single_model = single_model.replace("_WI_", str(wi))  # insert workspace index
        global_model += "(composite=CompositeFunction,NumDeriv=true,$domains=i;{0});\n".format(single_model)
        wi+=1
    # Introduce ties: Height and Radius same for all spectra
    ties=['='.join(["f{0}.f0.f1.Radius".format(wi) for wi in reversed(range(nQ))]),
        '='.join(["f{0}.f0.f1.Height".format(wi) for wi in reversed(range(nQ))]) ]
    global_model += "ties=("+','.join(ties)+')'  # introduce ties in the global model
    # Now relate each domain(i.e. spectrum) to each single model
    domain_model=dict()
    for wi in range(nQ):
        if wi == 0:
            domain_model.update({"InputWorkspace": data.name(), "WorkspaceIndex": str(wi),
                "StartX": str(Emin), "EndX": str(Emax)})
        else:
            domain_model.update({"InputWorkspace_"+str(wi): data.name(), "WorkspaceIndex_"+str(wi): str(wi),
                "StartX_"+str(wi): str(Emin), "EndX_"+str(wi): str(Emax)})

    """Invoke the Fit algorithm using global_model and domain_model.
    Output of the fit are three workspaces, but we are interested in workspace
    with name glofit_data_Parameters, containing optimized values for Radius and Height
    """
    output_workspace = "glofit_"+data.name()
    Fit(Function=global_model, Output=output_workspace, CreateOutput=True, MaxIterations=500, **domain_model)
    # Extract Height and Radius from workspace glofit_data_Parameters.
    # Check optimal values are close to nominal ones
    nparms=0
    parameter_ws = mtd[output_workspace+"_Parameters"]
    for irow in range(parameter_ws.rowCount()):
        row = parameter_ws.row(irow)
        if row["Name"]=="f0.f0.f1.Radius":
            Radius=row["Value"]  # Extract value of optimized Radius
            nparms+=1
        elif row["Name"]=="f0.f0.f1.Height":
            Height=row["Value"]  # Extract value of optimized Height
            nparms+=1
        if nparms==2:
            break
    if abs(H-Height)/H < 0.1:
        print("Optimal Height within 10% of nominal value")
    if abs(R-Radius)/R < 0.05:
        print("Optimal Radius within 5% of nominal value")

.. testcleanup:: ExampleElasticIsoRotDiff

   DeleteWorkspace("resolution")
   DeleteWorkspace("data")
   DeleteWorkspace("glofit_data_Workspaces")
   DeleteWorkspace("glofit_data_NormalisedCovarianceMatrix")
   DeleteWorkspace("glofit_data_Parameters")

Output:

.. testoutput:: ExampleElasticIsoRotDiff

    Optimal Height within 10% of nominal value
    Optimal Radius within 5% of nominal value

.. categories::

.. sourcelink::
