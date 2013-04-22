"""*WIKI* 
Suggest possible time independent background range for CNCS. It works for incident energy range from 0.5 to 50 meV. By default TibMax is 500 microseconds before the neutrons arrive at the sample, and TibMin is 3500 microseconds before Tibmax.
This range is moved around if a prompt pulse is in this interval, or it goes below the TOF frame minimum. 
*WIKI*"""

from mantid.api import PythonAlgorithm, AlgorithmFactory
import mantid.simpleapi 
from mantid.kernel import FloatBoundedValidator,Direction
from numpy import sqrt,floor


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
        #calculate tel, tmin, tmax, tinf, tpulse
        tel=1e6*(3.5+36.262)/self.e2v(energy)
        tmin=tel-1e6/120.
        if tmin<0:
            tmin=0.
        tmax=tmin+1e6/60.
        tinf=1e6*(36.262)/self.e2v(energy)
        if tinf<tmin:
            tinf+=1e6/60.
        tpulse=1e6/60*floor(tmax*60e-60)

        #check for TIB
        dtib=3500. # default length of TIB range
        dtibreduced=2500 #reduced range
        dtinf=500. # microseconds before tinf
        
        tibmax=tinf-dtinf
        if tibmax<tmin:         #move up a frame
            tibmax+=1e6/60.
        tibmin=tibmax-dtib
        if tibmin>tel:
            tibmin+=1000.       #reduce TIB range if at large TOFs, to be far from elastic line        
        
        if (tibmin-tpulse)*(tibmax-tpulse)<0 and tpulse<tel: # prompt pulse inside TIB range at low TOFs
            tibmin=tmax-dtibreduced 
            tibmax=tmax

        if (tibmin-tpulse)*(tibmax-tpulse)<0 and tpulse>tel: # prompt pulse inside TIB range at high TOFs
            tibmin=tmin
            tibmax=tmin+dtibreduced

        if tibmin<tmin:     # bring tibmin inside the frame
            tibmin=tmin        

        #if we have more space at lareg TOFs move either above or below the prompt pulse
        if (tibmax-tibmin)<2000 and (tmax-tpulse)>2000:
            tibmax=tmax
            tibmin=max(tmax-dtibreduced,tpulse)
        
        if (tibmax-tibmin)<2000 and (tmax-tpulse)<2000:
            tibmax=tpulse
            tibmin=tibmax-dtibreduced       

        #return the result
        self.setProperty("TibMin",tibmin+50)
        self.setProperty("TibMax",tibmax-50)
        return 
    
    
AlgorithmFactory.subscribe(SuggestTibCNCS)
