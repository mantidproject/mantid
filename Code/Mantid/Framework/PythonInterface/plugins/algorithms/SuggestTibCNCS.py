"""*WIKI* 
Suggest possible time independent background range for CNCS. It works for incident energy range from 0.5 to 50 meV. By default TibMax is 500 microseconds before the neutrons arrive at the sample, and TibMin is 3400 microseconds before Tibmax.
This range is moved around if a prompt pulse is in this interval, or it goes below the TOF frame minimum, or it can be reduced to 2400 microseconds. 
*WIKI*"""

from mantid.api import PythonAlgorithm, AlgorithmFactory
import mantid.simpleapi 
from mantid.kernel import FloatBoundedValidator,Direction
from numpy import sqrt,floor

class interval(object):
    def __init__(self,minv,maxv):
        self.min=minv
        self.max=maxv
    def overlap(self, other):
        if (other.max >self.min and other.max <self.max):
            return True
        if (other.min >self.min and other.min<self.max):
            return True
        if (other.min<self.min and other.max>self.max):
            return True
        return False

class SuggestTibCNCS(PythonAlgorithm):
    """ Check if certain sample logs exists on a workspace
    """
    def category(self):
        """ Return category
        """
        return "PythonAlgorithms;Utility;Inelastic"
    
    def name(self):
        """ Return name
        """
        return "SuggestTibCNCS"
    
    def PyInit(self):
        """ Declare properties
        """
        val=mantid.kernel.FloatBoundedValidator()
        val.setBounds(0.5,50) #reasonable incident nergy range for CNCS
        self.declareProperty("IncidentEnergy",0.,val,"Incident energy")
        self.declareProperty("TibMin",0.,Direction.Output)        
        self.declareProperty("TibMax",0.,Direction.Output)  
        return
    
    def e2v(self,energy):
        return sqrt(energy/5.227e-6)

    def PyExec(self):
        """ Main execution body
        """
        #get parameter
        energy = self.getProperty("IncidentEnergy").value
        frame=1e6/60.
        #calculate tel, tmin, tmax, tinf, tpulse
        tel=1e6*(3.5+36.262)/self.e2v(energy)
        tmin=tel-frame*0.5
        if tmin<0:
            tmin=0.
        tmax=tmin+frame
        tinf=1e6*(36.262)/self.e2v(energy)
        if tinf<tmin:
            tinf+=frame
        tpulse=frame*floor(tmax/frame)

        #check for TIB
        dtib=3500. # default length of TIB range
        dtibreduced=2500 #reduced range
        
        dtinfminus=500
        dtinfplus=1500
        dtpulseminus=50
        dtpulseplus=500
        
        intervallist=[]
        itinfminus=interval(tinf-dtinfminus,tinf)
        itinfplus=interval(tinf-frame,tinf-frame+dtinfplus)
        iframeedge=interval(tmin,tmin)
        intervallist.append(itinfplus)
        intervallist.append(iframeedge)
        intervallist.append(itinfminus)
        
        if tpulse+dtpulseplus<tmax:
            itpulse=interval(tpulse-dtpulseminus,tpulse+dtpulseplus)
        else:
            itpulse=interval(tpulse-dtpulseminus-frame,tpulse+dtpulseplus-frame)
        
        if itpulse.overlap(interval(tinf,tinf)):
            itpulse1=interval(itpulse.min,tinf)
            itpulse2=interval(tinf-frame,itpulse.max+tinf-frame)
            intervallist.append(itpulse1)
            intervallist.append(itpulse2)
        else:
            if tinf<itpulse.min:
                itpulse=interval(itpulse.min-frame,itpulse.max-frame)
            intervallist.append(itpulse)        
        
        timestocheck=[]
        for i in intervallist:
            if i.min>tinf-frame:
                timestocheck.append(i.min)
        timestocheck.sort()
        timestocheck.reverse()
        
        for t in timestocheck:
            tinterval=interval(t-dtib,t)
            if all( not inter.overlap(tinterval) for inter in intervallist ):
                tibmin=tinterval.min
                tibmax=tinterval.max
                break
            tinterval=interval(t-dtibreduced,t)
            if all( not inter.overlap(tinterval) for inter in intervallist ):
                tibmin=tinterval.min
                tibmax=tinterval.max
                break
                
        if tibmin<tmin:
            tibmin+=frame
            tibmax+=frame    

        #return the result
        self.setProperty("TibMin",tibmin+50)
        self.setProperty("TibMax",tibmax-50)
        return 
    
    
AlgorithmFactory.subscribe(SuggestTibCNCS)
