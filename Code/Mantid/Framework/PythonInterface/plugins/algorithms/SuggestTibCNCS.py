#pylint: disable=no-init,invalid-name
from mantid.api import PythonAlgorithm, AlgorithmFactory
import mantid.simpleapi
from mantid.kernel import FloatBoundedValidator,Direction
from numpy import sqrt,floor

class Interval(object):
    """Simple class that provides check for overlapping intervals
    """
    def __init__(self,minv,maxv):
        self.min=minv
        self.max=maxv
    def overlap(self, other):
        if other.max >self.min and other.max <self.max:
            return True
        if other.min >self.min and other.min<self.max:
            return True
        if other.min<self.min and other.max>self.max:
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

    def summary(self):
        """ Return summary
        """
        return "Suggest possible time independent background range for CNCS."

    def PyInit(self):
        """ Declare properties
        """
        val=FloatBoundedValidator()
        val.setBounds(0.5,50) #reasonable incident nergy range for CNCS
        self.declareProperty("IncidentEnergy",0.,val,"Incident energy (0.5 to 50 meV)")
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
        dtpulseplus=1500

        #Create intervals that cannot be used for TIB. For ease,
        #move everything to times lower than t_inf, make sure
        #one doesn't overlap with the frame edge, then if the TIB
        #interval is in the previous frame, jut move it up

        intervalList=[]
        intervalList.append(Interval(tinf-dtinfminus,tinf)) #interval close to t_inf, on the lower side
        intervalList.append(Interval(tmin,tmin)) #intervaldenoting frame edge. This will make sure that one cannot get an interval overlapping t_min
        intervalList.append(Interval(tinf-frame,tinf-frame+dtinfplus))  #interval close to t_inf, on the upper side, but moved one frame down

        if tpulse+dtpulseplus<tmax:
            itpulse=Interval(tpulse-dtpulseminus,tpulse+dtpulseplus)
        else:
            itpulse=Interval(tpulse-dtpulseminus-frame,tpulse+dtpulseplus-frame)

        if itpulse.overlap(Interval(tinf,tinf)):
            #if the prompt pulse overlaps with t_inf move the upper part one frame down
            intervalList.append(Interval(itpulse.min,tinf))
            intervalList.append(Interval(tinf-frame,itpulse.max-frame))
        else:
            if tinf<itpulse.min:
                itpulse=Interval(itpulse.min-frame,itpulse.max-frame)
            intervalList.append(itpulse)

        #create the list of times to checked. These are the lower parts of the intervals
        timestocheck=[]
        for i in intervalList:
            if i.min>tinf-frame:
                timestocheck.append(i.min)
        timestocheck.sort()
        timestocheck.reverse()

        for t in timestocheck:
            tInterval=Interval(t-dtib,t)
            if all( not inter.overlap(tInterval) for inter in intervalList ):
                tibmin=tInterval.min
                tibmax=tInterval.max
                break
            tInterval=Interval(t-dtibreduced,t)
            if all( not inter.overlap(tInterval) for inter in intervalList ):
                tibmin=tInterval.min
                tibmax=tInterval.max
                break

        #move to the data frame
        if tibmin<tmin:
            tibmin+=frame
            tibmax+=frame

        #return the result
        self.setProperty("TibMin",tibmin+50)
        self.setProperty("TibMax",tibmax-50)
        return


AlgorithmFactory.subscribe(SuggestTibCNCS)
