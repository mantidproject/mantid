from mantid.simpleapi import *
from mantid.api import (DataProcessorAlgorithm, AlgorithmFactory, PropertyMode, MatrixWorkspaceProperty,
                        Progress, WorkspaceGroupProperty)
from mantid.kernel import (StringListValidator, StringMandatoryValidator, IntBoundedValidator,
                           FloatBoundedValidator, Direction, logger, MaterialBuilder)
from mantid import config
import sys, os.path, math, random, numpy as np

class MuscatElasticReactor(DataProcessorAlgorithm):
 
    _sample_ws_name = None
    _sample_chemical_formula = None
    _sample_number_density = None
    _nrun1 = 10
    _nrun2 = 10
    _geom = None
    _numb_scat = 1
    _thickness = None
    _width = None
    _height = None
    _wave = None
    _number_angles = None
    _angles = None
    _q_values = None
    _delta_q = None
    _sofq = None
    _number_q = None
    _plot = False
    _save = False
    _sigc = None
    _sigi = None
    _sigt = None
    _siga_in = None
    _siga = None
    _sigss = None
    _sigss = None
    _sofq = None
    _vector = None
    _vkinc = None
    _Dk = None
    _total = 0.0
    _att = 0.0
    _QSS = 0.0

    def category(self):
        return "Workflow\\DECON"

    def summary(self):
        return "Calculates elastic multiple scattering."

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty('SofqWorkspace', '', direction=Direction.Input),
                            doc="Name for the input Sample workspace.")
        self.declareProperty(name='SampleChemicalFormula', defaultValue='',
                             validator=StringMandatoryValidator(),
                             doc='Sample chemical formula')
        self.declareProperty(name='SampleDensityType', defaultValue='Mass Density',
                             validator=StringListValidator(['Mass Density', 'Number Density']),
                             doc='Sample density type.')
        self.declareProperty(name='SampleDensity', defaultValue=1.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Sample number density')

        self.declareProperty(name='Geometry', defaultValue='Flat', 
                             validator=StringListValidator(['Flat','Cylinder']),
                             doc='Sample geometry')
        self.declareProperty(name='NeutronsSingle', defaultValue=1000,
                             validator=IntBoundedValidator(100),
                             doc='MonteCarlo neutrons - single scattering. Default=1000')
        self.declareProperty(name='NeutronsMultiple', defaultValue=1000,
                             validator=IntBoundedValidator(100),
                             doc='MonteCarlo neutrons - multiple scattering. Default=1000')
        self.declareProperty(name='NumberScatterings', defaultValue=1,
                             validator=IntBoundedValidator(0),
                             doc='Number of scatterings. Default=1')
        self.declareProperty(name='Wavelength', defaultValue=0.7,
                             doc='Incident wavelength. Default=0.7')
        self.declareProperty(name='NumberAngles', defaultValue=10,
                             validator=IntBoundedValidator(5),
                             doc='Number of angles for calulation. Default=10')

        self.declareProperty(name='Thickness', defaultValue=0.1,
                             validator=FloatBoundedValidator(0.0),
                             doc='Sample thickness')
        self.declareProperty(name='Width', defaultValue=2.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Sample width')
        self.declareProperty(name='Height', defaultValue=2.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Sample height')

        self.declareProperty(name='Plot', defaultValue=False,
                             doc='Switch Plot Off/On')
        self.declareProperty(name='Save', defaultValue=False,
                             doc='Switch Save result to nxs file Off/On')
 
    def PyExec(self):
        # Set up progress reporting
        prog = Progress(self, 0.0, 1.0, 2)

        self._setup()
        self._sample()
        self._calc_angles()
        two_pi = 2.0*math.pi
        four_pi = 2.0*two_pi
        self._vector = [1., 0., 0.]                 #incident vector fixed
        self._vector = self._unit_vector(self._vector)
        self._vkinc = two_pi/self._wave         #incident k-value
        Qmax = four_pi/self._wave
        self._set_geom()

#start loop over q values
        data_S0 = []
        data_S1 = []
        data_S2 = []
        data_S3 = []
        data_S4 = []
        data_S5 = []
        q = np.zeros(self._number_angles)
        for idx_ang in range(0, self._number_angles):
            self._new_vector()
            theta_t = self._angles[idx_ang]
            theta_r = np.radians(0.5*theta_t)
            q[idx_ang] = math.sin(theta_r)*Qmax
            Dkx = math.cos(theta_r)
            Dky = math.sin(theta_r)
            Dkz = 0.
            self._Dk = [Dkx, Dky, Dkz]
            self._Dk = self._unit_vector(self._Dk)

            self._total = np.zeros(6)                 #initialise totals
            self._att = 0.0                   #attenuation
            QS_sum = np.zeros(6)
#start loop over scatterings
            for ne in range(0,self._numb_scat+1):
                prog.report('Angle %i of %i ; Scatter %i of %i' % (idx_ang, self._number_angles, ne, self._numb_scat+1))
                if ne == 0:                   #first time no absorption
                    self._siga = 0.0
                else:
                    self._siga = self._siga_in
                self._QSS = 0.0
                QS_sum[ne] = 0.0
# new neutrons start here
                if ne <=1:              #if ms 
                    for neut in range(0,self._nrun1):    # no. of 1st scatterings
                        self._scatter(ne)
                else:
                    for neut in range(0,self._nrun2):    # no. of m.scatterings
                        self._scatter(ne)
                QS_sum[ne] = self._QSS
#end loop over scatterings
            D0 = self._total[0]/self._nrun1           #single scatt  no abs
            data_S0.append(D0)
            D1= self._total[1]/self._nrun1           #single scatt  with abs
            data_S1.append(D1)
            if self._numb_scat >= 2:
                D2 = self._total[2]/QS_sum[2]              #second scatt
                data_S2.append(D2)
                if self._numb_scat >= 3:
                    D3 = self._total[3]*4*self._nrun2/(QS_sum[3]*QS_sum[3])   #third scatt
                    data_S3.append(D3)
                    if self._numb_scat >= 4:
                        D4 = self._total[4]*27*self._nrun2*self._nrun2/(QS_sum[4]**3)
                        data_S4.append(D4)
                        if self._numb_scat == 5:
                            D5 = self._total[5]*16*16*self._nrun2*self._nrun2*self._nrun2/(QS_sum[5]**4)
                            data_S5.append(D5)
#end loop over q values
        data_ang = self._angles
        data_Q = q

        a0_ws = self._sample_ws_name + '_angle_scatt_0'
        self._create_ws(a0_ws, data_ang, data_S0, 'angle')
        a1_ws = self._sample_ws_name + '_angle_scatt_1'
        self._create_ws(a1_ws, data_ang, data_S1, 'angle')
        a_workspaces = [a0_ws, a1_ws]

        q0_ws = self._sample_ws_name + '_Q_scatt_0'
        self._create_ws(q0_ws, data_Q, data_S0, 'q')
        q1_ws = self._sample_ws_name + '_Q_scatt_1'
        self._create_ws(q1_ws, data_Q, data_S1, 'q')
        q_workspaces = [q0_ws, q1_ws]

        if self._numb_scat >= 2:
            a2_ws = self._sample_ws_name + '_angle_scatt_2'
            self._create_ws(a2_ws, data_ang, data_S2, 'angle')
            a_workspaces.append(a2_ws)

            q2_ws = self._sample_ws_name + '_Q_scatt_2'
            self._create_ws(q2_ws, data_Q, data_S2, 'q')
            q_workspaces.append(q2_ws)

            if self._numb_scat >= 3:
                a3_ws = self._sample_ws_name + '_angle_scatt_3'
                self._create_ws(a3_ws, data_ang, data_S3, 'angle')
                a_workspaces.append(a3_ws)

                q3_ws = self._sample_ws_name + '_Q_scatt_3'
                self._create_ws(q3_ws, data_Q, data_S3, 'q')
                q_workspaces.append(q3_ws)

                if self._numb_scat >= 4:
                    a4_ws = self._sample_ws_name + '_angle_scatt_4'
                    self._create_ws(a4_ws, data_ang, data_S4, 'angle')
                    a_workspaces.append(a4_ws)

                    q4_ws = self._sample_ws_name + '_Q_scatt_4'
                    self._create_ws(q4_ws, data_Q, data_S4, 'q')
                    q_workspaces.append(q4_ws)

                    if self._numb_scat == 5:
                        a5_ws = self._sample_ws_name + '_angle_scatt_5'
                        self._create_ws(a5_ws, data_ang, data_S5, 'angle')
                        a_workspaces.append(a5_ws)

                        q5_ws = self._sample_ws_name + '_Q_scatt_5'
                        self._create_ws(q5_ws, data_Q, data_S5, 'q')
                        q_workspaces.append(q5_ws)

        ang_out_ws = self._sample_ws_name + '_angle_ms'
        self._group_ws(','.join(a_workspaces), ang_out_ws)
        q_out_ws = self._sample_ws_name + '_Q_ms'
        self._group_ws(','.join(q_workspaces), q_out_ws)

        if self._plot:
            self._plot_result(a_workspaces)
            self._plot_result(q_workspaces)

        if self._save:
            self._save_output([ang_out_ws, q_out_ws])

    def _scatter(self, ne):
        four_pi = 4.0*math.pi
        self._start_point()
        Uk = self._vector               #start _vector
# if ms
        if ne > 1:
            for ims in range(1,ne):            #loop over m.scatterings
                Uk = self._q_dir(Uk)           #calcs QSS
#     WE NOW HAVE A NEW NEUTRON DIRECTION UKX,UKY,UKZ;MOD(K)=VKM
                self._direction = Uk
                self._isurf = 0
                dl = self._dist_exit()             #distance thru' sample
                if dl < 0:
                    dl = 0.0
                B4=1. - math.exp(-self._vmu*dl)
                self._vl = -(self._vmfp*math.log(1.0 - random.random()*B4))  #random path length
                self._B9 = self._B9*B4/self._sigt
                self._inc_xyz()                  #new xyz position
#end of ms part

        self._direction = self._Dk                     #Vx etc
        self._isurf = 0
        dl = self._dist_exit()                   #find distance to exit =exdist
        if ne == 0:
            dl = 0.
        QX = self._vkinc*(self._Dk[0] - Uk[0])                #scattered q-vectors
        QY = self._vkinc*(self._Dk[1] - Uk[1])
        QZ = self._vkinc*(self._Dk[2] - Uk[2])
        Q = math.sqrt(QX*QX + QY*QY + QZ*QZ)     #q-value
        if Q <= self._q_values[self._number_q -2]:                            #find S(Q)
            SQ = self._sofq_inter(Q)
            AT2 = math.exp(-self._vmu*dl)                 #attenuation along path
            weight = self._B9*AT2*SQ*self._sigt/four_pi     #weighting of scattering
            self._total[ne] += weight
            if ne == 1:
                self._B1 = self._B1*AT2                #b1=atten to 1st scatt
                self._att += self._B1               #final attenution


    def _setup(self):
        self._sample_ws_name = self.getPropertyValue('SofqWorkspace')
        logger.information('SofQ : ' + self._sample_ws_name)
        self._sample_chemical_formula = self.getPropertyValue('SampleChemicalFormula')
        self._sample_density_type = self.getProperty('SampleDensityType').value
        self._sample_density = self.getProperty('SampleDensity').value
        self._nrun1 = self.getProperty('NeutronsSingle').value
        self._nrun2 = self.getProperty('NeutronsMultiple').value
        self._geom = self.getPropertyValue('Geometry')
        logger.information('Geometry : ' + self._geom)
        self._numb_scat = self.getProperty('NumberScatterings').value
        if self._numb_scat < 1:
            raise ValueError('Number of scatterings %i is less than 1' %(self._numb_scat))
        if self._numb_scat > 5:
            self._numb_scat = 5
            logger.information('Number of scatterings set to 5 (max)')
        else:
            logger.information('Number of scatterings : %i ' % (self._numb_scat))
        self._thickness = self.getProperty('Thickness').value
        self._width = self.getProperty('Width').value
        self._height = self.getProperty('Height').value
        self._wave = self.getProperty('Wavelength').value
        self._number_angles = self.getProperty('NumberAngles').value
        self._q_values = mtd[self._sample_ws_name].readX(0)             # q array
        self._delta_q = self._q_values[1] - self._q_values[0]
        self._sofq = mtd[self._sample_ws_name].readY(0)             # S(q) values
        self._number_q = len(self._q_values)
        logger.information('Number of S(Q) values : %i ' % (self._number_q))

        self._plot = self.getProperty('Plot').value
        self._save = self.getProperty('Save').value

    def _sample(self):
        set_material_alg = self.createChildAlgorithm('SetSampleMaterial')
        logger.information('Sample chemical formula : %s ' % (self._sample_chemical_formula))
        if self._sample_density_type == 'Mass Density':
            logger.information('Sample Mass density : %f' % (self._sample_density))
            set_material_alg.setProperty('SampleMassDensity', self._sample_density)
            builder = MaterialBuilder()
            mat = builder.setFormula(self._sample_chemical_formula).setMassDensity(self._sample_density).build()
            self._sample_number_density = mat.numberDensity
            logger.information('Sample Number density : %f' % (self._sample_number_density))
        else:
            self._sample_number_density = self._sample_density
            logger.information('Sample Number density : %f' % (self._sample_number_density))
            set_material_alg.setProperty('SampleNumberDensity', self._sample_number_density)
        set_material_alg.setProperty('InputWorkspace', self._sample_ws_name)
        set_material_alg.setProperty('ChemicalFormula', self._sample_chemical_formula)
        set_material_alg.execute()
        sam_material = mtd[self._sample_ws_name].sample().getMaterial()
        # Sample cross section
        self._sigc = sam_material.cohScatterXSection()
        self._sigi = sam_material.incohScatterXSection()
        self._sigt = sam_material.totalScatterXSection()
        self._siga_in = sam_material.absorbXSection()
        self._siga = self._siga_in

        self._sigss = self._sofq*self._sigc + self._sigi    #q_dependent total scatt X-sect
        self._sigss = np.log(self._sigss)                   #interpolation later on
        self._sofq = np.log(self._sofq/self._sigt)          #S(Q) normalised

    def _calc_angles(self):
        Qmax = 4.0*math.pi/self._wave
        theta_r = np.arcsin(self._q_values/Qmax)
        theta_d = 2.0*np.rad2deg(theta_r)
        ang_inc = (theta_d[len(theta_d) - 1] - theta_d[0])/self._number_angles
        self._angles = np.zeros(self._number_angles)
        for idx_ang in range(self._number_angles):
            self._angles[idx_ang] = theta_d[0] + idx_ang*ang_inc
        logger.information('Number of angles : %i ; from %f to %f ' % 
                           (self._number_angles, self._angles[0], self._angles[self._number_angles -1]))


    def _unit_vector(self, vector):
# unit vector
        one = math.sqrt(vector[0]*vector[0] + vector[1]*vector[1] + vector[2]*vector[2])    #unit vector 
        vx = vector[0]/one                   #direction
        vy = vector[1]/one
        vz = vector[2]/one
        return [vx, vy, vz]

    def _inc_xyz(self):
# formerly inc_xyz(VL)
# self._position is x,y,z ; self._direction is Vx,y,z
        x = self._position[0] + self._vl*self._direction[0]
        y = self._position[1] + self._vl*self._direction[1]
        z = self._position[2] + self._vl*self._direction[2]
        self._position = [x, y, z]

    def _start_point(self):                              #get starting point
# formerly RONE to get starting point
# COMMON/Vector/VKINC,VMFP,Vmu,VKX,VKY,VKZ,B1,B9 & COMMON/DIR/VX,VY,VZ & COMMON/DEF/X,Y,Z,IREG
        self._direction = self._vector
        self._ireg = self._nreg
        if self._geom == 'Flat':
            X = 0.0
            Y = random.random() * self._surface_g[3]          #random width
            Z = random.random() * self._surface_g[1]          #random height
            self._position = [X, Y, Z]                      #entry flat face
        if self._geom == 'Cylinder':
            Y = random.random() * self._surface_g[self._nsurf + self._nreg -1]  #random width
            Z = random.random() * self._surface_g[1]           #random height
            X = -math.sqrt(-self._surface_g[self._nsurf - 1] - Y*Y)    #entry curved face
            self._position = [X, Y, Z]    #entry curved face
        self._isurf = 0
        dl = self._dist_exit()                   #find distance to exit =exdist
        self._B9 = 1.0 - math.exp(-self._vmu*dl)            #atten to exit
        self._vl = random.random()*dl             #random point along DL =VL
        self._B1 = math.exp(-self._vmu*self._vl)               #atten to VL
        self._B9 = self._B9/self._sigt                    #new weighting
        self._inc_xyz()                  #new xyz position
#        self._noinc += 1                 #increment ntn number

    def _new_vector(self):                #calc new V-mean-free-path
        IVKM = int(self._vkinc/self._delta_q) -1               #find Q value
        if IVKM >= self._number_q:                     #> Qmax
            sig_scat = math.exp(self._sigss[self._number_q -1])
        else:
            if IVKM <= 0:                 #< Qmin
                sig_scat = math.exp(self._sigss[0])
            else:                          #interpolate scatt x-sec
                if IVKM > self._number_q-2:
                    IVKM = self._number_q-2
                U = (self._vkinc - (IVKM*self._delta_q))/self._delta_q
                A = (self._sigss[IVKM] -2*self._sigss[IVKM+1] + self._sigss[IVKM+2])/2
                B =(-3*self._sigss[IVKM] + 4*self._sigss[IVKM+1] - self._sigss[IVKM+2])/2
                C = self._sigss[IVKM]
                sig_scat = math.exp(A*U*U + B*U + C)
        sig_total = sig_scat + self._siga            #new total x-sec
        self._vmu = self._sample_number_density*sig_total                #new trans coeff
        self._vmfp = 1.00/self._vmu               #new mean free path

    def _sofq_inter(self, Q):                #interpolate S(Q)
        if Q <= self._q_values[0]:            #Q < Qmin
            return math.exp(self._sofq[0])
        elif Q >= self._q_values[self._number_q -2]:
            return math.exp(self._sofq[self._number_q -2])
        else:                                #interpolate
            IQ0 = int((Q - self._q_values[0])/self._delta_q)
            if IQ0 > self._number_q -3:
                IQ0 = self._number_q -3
            IQ1 = IQ0+1
            IQ2 = IQ0+2
            U = (Q - IQ0*self._delta_q - self._q_values[0])/self._delta_q
            A = (self._sofq[IQ0] -2*self._sofq[IQ1]+ self._sofq[IQ2])/2
            B = (-3*self._sofq[IQ0] +4*self._sofq[IQ1] -self._sofq[IQ2])/2
            C = self._sofq[IQ0]
            SQ = math.exp(A*U*U + B*U + C)    #new value of S(Q)
            return SQ

    def _q_dir(self, Uk):    #find new q direction
        m, QQ, CosT = self._q_calc()
        if m >= 999:
            return [0, 0, 0]
        else:
            SQ = self._sofq_inter(QQ)                     #find S(Q) =SQ
            self._QSS += QQ*SQ
            self._B9 = self._B9*self._sigt*SQ*QQ
            FI = random.random()*6.28318531     #random scattering angle
            B3 = math.sqrt(1. - CosT*CosT)
            B2 = CosT
            if Uk[2] < 1.0:
                A2 = math.sqrt(1.0 - Uk[2]*Uk[2])
                UQTZ = math.cos(FI)*A2
                UQTX = -math.cos(FI)*Uk[2]*Uk[0]/A2 + math.sin(FI)*Uk[1]/A2
                UQTY = -math.cos(FI)*Uk[2]*Uk[1]/A2 - math.sin(FI)*Uk[0]/A2
                UKX = B2*Uk[0] + B3*UQTX
                UKY = B2*Uk[1] + B3*UQTY
                UKZ = B2*Uk[2] + B3*UQTZ
            else:
                UKX = B3*math.cos(FI)
                UKY = B3*math.sin(FI)
                UKZ = B2
            return [UKX, UKY, UKZ]

    def _q_calc(self):
        for m in range(0,1000):   
            QQ = self._q_values[self._number_q -1]*random.random()            #random q_value
            CosT = 1. - QQ*QQ/(2.*self._vkinc*self._vkinc)
            if CosT > -1.0 and CosT < 1.0:
                return m, QQ, CosT
		return m, QQ, 0.0

			
#   Geometry routines

    def _set_geom(self):
        self._nreg = 1
        self._surface_a = []
        self._surface_b = []
        self._surface_c = []
        self._surface_d = []
        self._surface_e = []
        self._surface_f = []
        self._surface_g = []
        half_width = 0.5*float(self._width)
        half_height = 0.5*float(self._height)
        if self._geom == 'Flat':
            self._surface_g = [-half_height, half_height, -half_width, half_width, 0, -float(self._thickness)]
            self._surface_f = [1.0, 1.0, 0, 0, 0, 0]
            self._surface_e = [0, 0, 0, 0, 0, 0]
            self._surface_d = [0, 0, 1.0, 1.0, 0, 0]
            self._surface_c = [0, 0, 0, 0, 0, 0]
            self._surface_b = [0, 0, 0, 0, 1.0, 1.0]
            self._surface_a = [0, 0, 0, 0, 0, 0]
            self._nsurf = 4+2*self._nreg            #is 6
            self._igeom = [-1, 1, -1, 1, 1, -1]
        if self._geom == 'Cylinder':
            if self._vector[1] != 0.0:
                raise ValueError('Vky not 0.0')
#  IF NREG>1 ,REG1 IS INNERMOST CYLINDER(SAMPLE) & REG2 NEXT RING(CONT)
#   REG3 WOULD BE OUTSIDE REG2 & SO ON
            self._nsurf = self._nreg+2            #is 3
            self._surface_g = [-half_height, half_height, -half_width*half_width, half_width]
            self._surface_f = [1.0, 1.0, 0, 0]
            self._surface_e = [0, 0, 0, 0]
            self._surface_d = [0, 0, 1.0, 1.0]
            self._surface_c = [0, 0, 1.0, 0]
            self._surface_b = [0, 0, 0, 0]
            self._surface_a = [0, 0, 1.0, 0]
            self._igeom = [-1, 1, -1, 1, 1, -1]

    def _test_in_reg(self):
# formerly TESTIN with COMMON/DEF/X,Y,Z,IREG
# TESTS WETHER A PT X,Y,Z IS IN REGION IREG
        X = self._position[0]
        Y = self._position[1]
        Z = self._position[2]
        VX = self._direction[0]
        VY = self._direction[1]
        VZ = self._direction[2]
        for i in range(0,self._nsurf):
            if self._igeom[i] != 0 or i != self.isurf:
                PP1 = self._surface_a[i]*X*X + self._surface_b[i]*X
                PP2 = self._surface_c[i]*Y*Y + self._surface_d[i]*Y
                PP3 = self._surface_e[i]*Z*Z + self._surface_f[i]*Z
                PP=PP1 + PP2 + PP3 + self._surface_g[i]
                if PP == 0.:
                    PPx = (self._surface_a[i]*X + self._surface_b[i])*VX
                    PPy = (self._surface_c[i]*Y + self._surface_d[i])*VY
                    PPz = (self._surface_e[i]*Z + self._surface_f[i])*VZ
                    PP=2.*(PPx + PPy + PPz)
                if PP == 0.:
                    PP=self._surface_a[i]*VX*VX + self._surface_c[i]*VY*VY + self._surface_e[i]*VZ*VZ
                if PP*self._igeom[i] < 0.:
                    self._in_region = False
                    return
        self._in_region = True

    def _dist_exit(self):
# formerly DTOEX
# CALCULATES THE DISTANCE (EXDIST) TO THE EXIT OF A REGION (IREG)
# ALONG THE DIRECTION VX,VY,VZ FROM X,Y,Z.  IF IREG = 0, FINDS
# DISTANCE TO NEAREST SURFACE.  THE EXIT SURFACE IS
# RETURNED IN ISURF.  IF ISURF .NE. 0 AT ENTRY, THEN THE POINT
# IS ASSUMED TO BE ON ISURF AND THAT DISTANCE IS 0.
        ex_dist = 100000.0
        II = 0
        for I in range(0,self._nsurf):
            if self._ireg != 0:
                if self._igeom[I] == 0:
                    self._isurf = II
                    if ex_dist == 100000.0:
                        ex_dist=-1.0
            DD = self._distance(I)
            if DD <= 0.0:
                    self._isurf = II
                    if ex_dist == 100000.0:
                        ex_dist=-1.0
            if DD > ex_dist:
                    self._isurf = II
                    if ex_dist == 100000.0:
                        ex_dist=-1.0
            II = I
            ex_dist = DD
        self._isurf = II
        if ex_dist == 100000.0:
            ex_dist=-1.0
#        logger.information('exDistance : '+str(ex_dist))
        return ex_dist

    def _distance(self, I):                              #FIND POSITIVE DISTANCE TO SURFACE I
# formerly function DIST  ie returns DIST
# FIND POSITIVE DISTANCE TO SURFACE I
# IF ISURF.NE.0, ASSUME CURRENTLY ON SURFACE ISURF
# CALCULATES THE DIST TO THE I'TH SURFACE FROM (X,Y,Z) IN DIR'N (VX,VY,VZ)
        VX = self._direction[0]
        VY = self._direction[1]
        VZ = self._direction[2]
        X = self._position[0]
        Y = self._position[1]
        Z = self._position[2]
        AA = self._surface_a[I]*VX*VX + self._surface_c[I]*VY*VY + self._surface_e[I]
        BB1 = 2*(self._surface_a[I]*VX*X + self._surface_c[I]*VY*Y + self._surface_e[I]*VZ*Z)
        BB2 = self._surface_b[I]*VX + self._surface_d[I]*VY + self._surface_f[I]*VZ
        BB = BB1 + BB2
        CC=0.
        if I != self._isurf:
            CCx = self._surface_a[I]*X*X + self._surface_b[I]*X
            CCy = self._surface_c[I]*Y*Y + self._surface_d[I]*Y
            CCz = self._surface_e[I]*Z*Z + self._surface_f[I]*Z
            CC = CCx + CCy + CCz + self._surface_g[I]
# WE NOW HAVE TO SOLVE THE QUADRATIC AA*T*T+BB*T+CC=0
        if AA == 0.:
            if BB == 0.:
#  TRAJECTORY PARALLEL TO PLANE, NO INTERSECTION
                return -1.0E4
#  SURFACE HAS NO CURVATURE IN DIRECTION OF TRAJECTORY
            return -CC/BB
#  FULL QUADRATIC TREATMENT REQUIRED
        BB = -0.5*BB/AA
        DD = BB*BB - CC/AA
        if DD < 0.:
#  TRAJECTORY DOES NOT INTERSECT THIS QUADRATIC SURFACE
            return -2.0E4
        if CC == 0.:
#  PRESENTLY ON THIS SURFACE, BUT MAY HIT IT AGAIN
            return BB + math.fabs(BB)
        if BB == 0.:
#  TEST SIZE OF RADICAL, COMPARED TO BB
            DD = math.sqrt(DD)
            DIST = BB - DD
            if DIST <= 0.0:
                DIST = BB + DD
            return DIST
        EE = 1.-CC/AA/(BB*BB)
        if math.fabs(EE) > 1.E-4:
#  BETTER PRECISION BY EXPANDING SQUARE ROOT
            DD = math.sqrt(DD)
            DIST = BB - DD
#  IF >0, THIS IS SMALLER POSITIVE DISTANCE; ELSE, TRY LARGER ONE
            if DIST <= 0.0:
                DIST = BB + DD
            return DIST
#  BETTER PRECISION BY EXPANDING SQUARE ROOT
        DD = math.fabs(BB)*(1. + 0.5*EE)
        DIST = BB - DD
        if DIST <= 0.0:
            DIST = BB + DD
        return DIST
	
	
#   general routines

    def _save_output(self, workspaces):
        workdir = config['defaultsave.directory']
        for ws in workspaces:
            path = os.path.join(workdir, ws + '.nxs')
            logger.information('Creating file : %s' % path)
            save_alg = self.createChildAlgorithm("SaveNexusProcessed", enableLogging = False)
            save_alg.setProperty("InputWorkspace", ws)
            save_alg.setProperty("Filename", path)
            save_alg.execute()

    def _plot_result(self, workspaces):
        from IndirectImport import import_mantidplot
        mp = import_mantidplot()
        mp.plotSpectrum(workspaces,0)

    def _create_ws(self, output_ws, x, y, unit):
        create_alg = self.createChildAlgorithm("CreateWorkspace", enableLogging = False)
        create_alg.setProperty("OutputWorkspace", output_ws)
        create_alg.setProperty("DataX", x)
        create_alg.setProperty("DataY", y)
        create_alg.setProperty("Nspec", 1)
        create_alg.setProperty("UnitX", 'MomentumTransfer')
        create_alg.execute()
        mtd.addOrReplace(output_ws, create_alg.getProperty("OutputWorkspace").value)
        if unit == 'angle':
            unitx = mtd[output_ws].getAxis(0).setUnit("Label")
            unitx.setLabel('2theta', 'deg')

    def _group_ws(self, input_ws, output_ws):
        group_alg = self.createChildAlgorithm("GroupWorkspaces", enableLogging=False)
        group_alg.setProperty("InputWorkspaces", input_ws)
        group_alg.setProperty("OutputWorkspace", output_ws)
        group_alg.execute()
        mtd.addOrReplace(output_ws, group_alg.getProperty("OutputWorkspace").value)


AlgorithmFactory.subscribe(MuscatElasticReactor)          # Register algorithm with Mantid
