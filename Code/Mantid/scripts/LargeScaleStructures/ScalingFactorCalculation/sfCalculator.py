from MantidFramework import *
from mantidsimple import *
from numpy import zeros
from pylab import *
import time
import copy

class sfCalculator():      
    
    tof_min = 10000  #microS
    tof_max = 21600  #microS

    #range of x pixel to use in the X integration (we found out that there is a frame effect that introduces noise)
    x_pixel_min = 90
    x_pixel_max = 190

    #from,width,to in microS
    rebin_parameters = '0,200,200000'  

    #turn on or off the plots
    bPlot = False
    bFittingPlot = True
    
    #size of detector
    alpha_pixel_nbr = 256 
    beta_pixel_nbr = 304  #will be integrated over this dimension
    
    #name of numerators and denominators
    numerator = None #ex: AiD0
    denominator = None   #ex: AiD1

    y_axis_numerator = None
    y_axis_error_numerator = None
    y_axis_denominator = None
    y_axis_error_denominator = None
    x_axis = None     
         
    #define the peak region      
    n_peak_pixel_min = 130
    n_peak_pixel_max = 135
    d_peak_pixel_min = 130
    d_peak_pixel_max = 135

    peak_pixel_min = None
    peak_pixel_max = None
    back_pixel_min = None
    back_pixel_max = None

    #define the background range used in the background subtraction
    n_back_pixel_min = 125
    n_back_pixel_max = 140
    d_back_pixel_min = 125
    d_back_pixel_max = 140

    y_axis_ratio = None
    y_axis_error_ratio = None
    x_axis_ratio = None
            
    def __init__(self, numerator=None, denominator=None):
        
        self.numerator = numerator
        self.denominator = denominator

        self.x_axis_ratio = None
        self.y_axis_error_ratio = None
        self.y_axis_ratio = None
        
    def setNumerator(self, minPeak, maxPeak, minBack, maxBack):
        if minPeak != 0:
            self.n_peak_pixel_min = minPeak
        if maxPeak != 0 :
            self.n_peak_pixel_max = maxPeak
        if minBack != 0:
            self.n_back_pixel_min = minBack
        if maxBack != 0:
            self.n_back_pixel_max = maxBack
        
    def setDenominator(self, minPeak, maxPeak, minBack, maxBack):
        if minPeak != 0:
            self.d_peak_pixel_min = minPeak
        if maxPeak != 0:
            self.d_peak_pixel_max = maxPeak
        if minBack != 0:
            self.d_back_pixel_min = minBack
        if maxBack != 0:
            self.d_back_pixel_max = maxBack
        
    def run(self):
        """
        Perform the calculation

        """
        
        #perform calculation for numerator
        self._calculateFinalYAxis(bNumerator=True)
        
        #perform calculation for denominator
        self._calculateFinalYAxis(bNumerator=False)
        
        #calculate y_axis of numerator/denominator
#        self._x_axis_ratio = self._x_axis
        self.y_axis_ratio = self.y_axis_numerator/self.y_axis_denominator
        self.y_axis_error_ratio = ((self.y_axis_error_numerator/self.y_axis_numerator)**2 +
                                    (self.y_axis_error_denominator/self.y_axis_denominator)**2)
        self.y_axis_error_ratio = sqrt(self.y_axis_error_ratio)
        self.y_axis_error_ratio *= self.y_axis_ratio
        
    def _calculateFinalYAxis(self, bNumerator=True):
        """
        run full calculation for numerator or denominator
        """
        if bNumerator is True:
            file = self.numerator
#            _id = self.id_numerator
            self.peak_pixel_min = self.n_peak_pixel_min
            self.peak_pixel_max = self.n_peak_pixel_max
            self.back_pixel_min = self.n_back_pixel_min
            self.back_pixel_max = self.n_back_pixel_max
        else:
            file = self.denominator
#            _id = self.id_denominator
            self.peak_pixel_min = self.d_peak_pixel_min
            self.peak_pixel_max = self.d_peak_pixel_max
            self.back_pixel_min = self.d_back_pixel_min
            self.back_pixel_max = self.d_back_pixel_max
        
        nexus_file_numerator = file
        LoadEventNexus(Filename=nexus_file_numerator, 
                       OutputWorkspace='EventDataWks')
        mt1 = mtd['EventDataWks']
        proton_charge = self._getProtonCharge(mt1)
        rebin(InputWorkspace='EventDataWks', 
              OutputWorkspace='HistoDataWks', 
              Params=self.rebin_parameters)
        mt2 = mtd['HistoDataWks']
        x_axis = mt2.readX(0)[:]
        self.x_axis = x_axis
        
        self._createIntegratedWorkspace(InputWorkspace=mt2, 
                                        OutputWorkspace='IntegratedDataWks',
                                        proton_charge=proton_charge, 
                                        from_pixel=self.x_pixel_min, 
                                        to_pixel=self.x_pixel_max)

        Transpose(InputWorkspace='IntegratedDataWks', 
                  OutputWorkspace='TransposeIntegratedDataWks')
        ConvertToHistogram(InputWorkspace='TransposeIntegratedDataWks',
                           OutputWorkspace='TransposeIntegratedDataWks_t')
        FlatBackground(InputWorkspace='TransposeIntegratedDataWks_t',
                       OutputWorkspace='TransposeHistoFlatDataWks',
                       StartX=self.back_pixel_min,
                       EndX=self.peak_pixel_min)
        Transpose(InputWorkspace='TransposeHistoFlatDataWks',
                  OutputWorkspace='DataWks')
        mt3 = mtd['DataWks']        
        self._calculateFinalAxis(Workspace=mt3, 
                           bNumerator=bNumerator)

        #cleanup workspaces
#        mtd.deleteWorkspace('EventDataWks')
#        mtd.deleteWorkspace('HistoDataWks')
#        mtd.deleteWorkspace('IntegratedDataWks')
#        mtd.deleteWorkspace('TransposeIntegratedDataWks')
#        mtd.deleteWorkspace('TransposeIntegratedDataWks_t')
#        mtd.deleteWorkspace('TransposeHistoFlatDataWks')
#        mtd.deleteWorkspace('DataWks')
        
    def _calculateFinalAxis(self, Workspace=None, bNumerator=None):
        """
        this calculates the final y_axis and y_axis_error of numerator and denominator
        """
        mt = Workspace
        x_axis = mt.readX(0)[:]
        self.x_axis = x_axis
        
        counts_vs_tof = zeros(len(x_axis))
        counts_vs_tof_error = zeros(len(x_axis))
        for x in range(self.alpha_pixel_nbr):
            counts_vs_tof += mt.readY(x)[:]
            counts_vs_tof_error += mt.readE(x)[:]**2
        counts_vs_tof_error = sqrt(counts_vs_tof_error)
        index_tof_min = self._getIndex(self.tof_min, x_axis)
        index_tof_max = self._getIndex(self.tof_max, x_axis)

        if (bNumerator is True):
            self.y_axis_numerator = counts_vs_tof[index_tof_min:index_tof_max]
            self.y_axis_error_numerator = counts_vs_tof_error[index_tof_min:index_tof_max]
            self.x_axis_ratio = self.x_axis[index_tof_min:index_tof_max]
        else:
            self.y_axis_denominator = counts_vs_tof[index_tof_min:index_tof_max]
            self.y_axis_error_denominator = counts_vs_tof_error[index_tof_min:index_tof_max]
            self.x_axis_ratio = self.x_axis[index_tof_min:index_tof_max]

    def _createIntegratedWorkspace(self,
                                   InputWorkspace=None, 
                                   OutputWorkspace=None,
                                   proton_charge=None, 
                                   from_pixel=0, 
                                   to_pixel=303):
        """
        This creates the integrated workspace over the second pixel range (beta_pixel_nbr here) and
        returns the new workspace handle
        """
        x_axis = InputWorkspace.readX(0)[:]
        x_size = to_pixel - from_pixel + 1 
        y_axis = zeros((self.alpha_pixel_nbr, len(x_axis) - 1))
        y_error_axis = zeros((self.alpha_pixel_nbr, len(x_axis) - 1))
        y_range = arange(x_size) + from_pixel
        for x in range(self.beta_pixel_nbr):
            for y in y_range:
                index = int(self.alpha_pixel_nbr * x + y)
                y_axis[y, :] += InputWorkspace.readY(index)[:]
                y_error_axis[y, :] += ((InputWorkspace.readE(index)[:]) * 
                                        (InputWorkspace.readE(index)[:]))

        y_axis = y_axis.flatten()
        y_error_axis = sqrt(y_error_axis)
        #plot_y_error_axis = _y_error_axis #for output testing only    -> plt.imshow(plot_y_error_axis, aspect='auto', origin='lower')
        y_error_axis = y_error_axis.flatten()

        #normalization by proton charge
        y_axis /= (proton_charge * 1e-12)

        CreateWorkspace(OutputWorkspace, 
                        DataX=x_axis, 
                        DataY=y_axis, 
                        DataE=y_error_axis, 
                       Nspec=self.alpha_pixel_nbr)
#        mt3 = mtd[OutputWorkspace]
#        return mt3 
            
    def _getIndex(self, value, array):
        """
        returns the index where the value has been found
        """
        return array.searchsorted(value)
            
    def _getProtonCharge(self, st=None):
        """
        Returns the proton charge of the given workspace in picoCoulomb
        """
        if st is not None:
            mt_run = st.getRun()
            proton_charge_mtd_unit = mt_run.getProperty('gd_prtn_chrg').value
            proton_charge = proton_charge_mtd_unit / 2.77777778e-10
            return proton_charge
        return None              

    def __mul__(self, other):
        """
        operator * between two instances of the class
        """
        
        product = sfCalculator()
        
        product.numerator = self.numerator + '*' + other.numerator
        product.denominator = self.denominator + '*' + other.denominator
        
        product.x_axis_ratio = self.x_axis_ratio
        product.y_axis_ratio = self.y_axis_ratio * other.y_axis_ratio
        product.y_axis_error_ratio = sqrt((other.y_axis_ratio*self.y_axis_error_ratio)**2 +
                                          (other.y_axis_error_ratio*self.y_axis_ratio)**2)
        return copy.deepcopy(product)
    
    def fit(self):
        """
        This is going to fit the counts_vs_tof with a linear expression and return the a and
        b coefficients (y=a+bx)
        """
        CreateWorkspace('DataToFit', 
                        DataX=self.x_axis_ratio, 
                        DataY=self.y_axis_ratio,
                        DataE=self.y_axis_error_ratio,
                        Nspec=1)
        Fit(InputWorkspace='DataToFit',
            Function="name=UserFunction, Formula=a+b*x, a=1, b=2", 
            Output='Res')
        res = mtd['Res_Parameters']
        self.a = res.getDouble("Value",0)
        self.b = res.getDouble("Value",1)
        self.error_a = res.getDouble("Error",0)
        self.error_b = res.getDouble("Error",1)            

def plotObject(instance):
    
#    return

#    print 'a: ' + str(instance.a[-1])
#    print 'b: ' + str(instance.b[-1])    
    
    figure()
    errorbar(instance.x_axis_ratio, 
             instance.y_axis_ratio, 
             instance.y_axis_error_ratio, 
             marker='s', 
             mfc='red',
             linestyle='',
             label='Exp. data')
    
    if (instance.a is not None):
        x=linspace(10000,22000,100)
        _label = "%.3f + x*%.2e" % (instance.a, instance.b)
        plot(x,instance.a+instance.b*x, label=_label)
    
    xlabel("TOF (microsS)")
    ylabel("Ratio")
    
    title(instance.numerator + '/' + instance.denominator)
    show()
    legend()

def recordSettings(a,b,error_a,error_b,name,instance):
    """
    This function will record the various fitting parameters and the 
    name of the ratio
    """
    a.append(instance.a)
    b.append(instance.b)
    error_a.append(instance.error_a)
    error_b.append(instance.error_b)
    name.append(instance.numerator + '/' + instance.denominator)

def outputFittingParameters(a,b,a_error,b_error,name,output_file_name):
    """
    Create an ascii file of the various fittings parameters
    y=a+bx
    1st column: name of numerator/denominator
    2nd column: a
    3rd column: b
    4th column: error_a
    5th column: error_b
    """
    _content = ['#y=a+bx\n','#numerator/denominator a b error_a error_b\n','#\n']
    sz = len(a)
    for i in range(sz):
        _line = name[i] + ' '
        _line += str(a[i]) + ' '
        _line += str(b[i]) + ' '
        _line += str(error_a[i]) + ' '
        _line += str(error_b[i]) + '\n'
        _content.append(_line)
    
    f = open(output_file_name, 'w')
    f.writelines(_content)
    f.close()

def createInputDictionary(list_files):
    """
    Using the list_files, will produce a dictionary of the run number and number of attenuator
    ex:
        list_files = "1000:0, 1001:1, 1002:1, 1003:2"
        return {1000:0, 1001:1, 1002:2, 1003:2}
    """
    if (list_files == ''):
        return None
    first_split = list_files.split(',')
    _input_dico = {}
    _nbr_files = len(first_split)
    for i in range(_nbr_files):
        _second_split = first_split[i].split(':')
        _input_dico[_second_split[0]] = _second_split[1]
    return _input_dico

def getSh(mt, top_tag, bottom_tag):
    """
        returns the height and units of the given slit#
    """
    mt_run = mt.getRun()
    st = mt_run.getProperty(top_tag).value
    sb = mt_run.getProperty(bottom_tag).value
    sh = float(sb[0]) - float(st[0])
    units = mt_run.getProperty(top_tag).units
    return sh, units
    
def getS1h(mt=None):
    """    
        returns the height and units of the slit #1 
    """
    if mt != None:
        _h,units = getSh(mt,'s1t','s1b') 
        return _h,units
    return None,''
    
def getS2h(mt=None):
    """    
        returns the height and units of the slit #2 
    """
    if mt != None:
        _h,units = getSh(mt,'s2t','s2b') 
        return _h,units
    return None,None

def getSlitsValue(full_list_runs, S1H, S2H):
    """
    Retrieve the S1H and S2H values
    """
    _nbr_files = len(full_list_runs)
    for i in range(_nbr_files):
        _full_file_name = full_list_runs[i]
        LoadEventNexus(Filename=_full_file_name, 
                       OutputWorkspace='tmpWks')
        mt1 = mtd['tmpWks']
        _s1h_value, _s1h_units = getS1h(mt1)
        _s2h_value, _s2h_units = getS2h(mt1)
        S1H[i] = _s1h_value
        S2H[i] = _s2h_value

def isRunsSorted(list_runs, S1H, S2H):
    """
    Make sure the files have been sorted
    """
    sz = len(S1H)
    sTotal = zeros(sz)
    for i in range(sz):
        sTotal[i] = S1H[i] + S2H[i]
     
    sorted_sTotal = sorted(sTotal)
     
    for i in range(len(sTotal)):
        _left = list(sTotal)[i]
        _right = sorted_sTotal[i]
        
        _left_formated = "%2.1f" %_left
        _right_formated = "%2.1f" %_right
        if (_left_formated != _right_formated):
            return False
    
    return True

if __name__ == '__main__':
    
    #Input from user
    list_runs = ['55889','55890','55891','55892','55893','55894','55895','55896','55897','55898','55899','55900','55901','55902']
    list_attenuator = [0,1,1,1,1,1,1,2,2,2,3,3,4,4]
    
    list_peak_back = zeros((len(list_runs),4))   #[peak_min, peak_max, back_min, back_max]
    list_peak_back[9,] = [128,136,120,145]
    list_peak_back[11,] = [125,140,115,150]
    list_peak_back[10,] = [128,136,120,145]
    list_peak_back[13,] = [120,145,105,155]
    list_peak_back[12,] = [125,140,115,150]
    
    nexus_path = '/mnt/hgfs/j35/'
    pre = 'REF_L_'
    nexus_path_pre = nexus_path + pre
    post = '_event.nxs'
    
    for (offset,item) in enumerate(list_runs):
        list_runs[offset] = nexus_path_pre + list_runs[offset] + post

    #####
    #Input file should be as it is here !
    #####
    
    #retrieve the S1H and S2H val/units for each NeXus    
    S1H={}
    S2H={}
    getSlitsValue(list_runs, S1H, S2H)
 
    #make sure the file are sorted from smaller to bigger openning
    if isRunsSorted(list_runs, S1H, S2H):
        
        #initialize record fitting parameters arrays
        a=[]
        b=[]
        error_a=[]
        error_b=[]
        name=[]
        _previous_cal = None

        _first_1A = True
        _first_2A = True
        _first_3A = True
        _first_4A = True
        _first_5A = True
        _first_6A = True

        _index_first_1A = -1
        _index_first_2A = -1
        _index_first_3A = -1
        _index_first_4A = -1
        _index_first_5A = -1
        _index_first_6A = -1
        
        index_numerator = -1
        index_denominator = -1
        
        list_objects = []
        
        for i in range(len(list_runs)):
            
            if list_attenuator[i] == 0:
                continue

            if list_attenuator[i] == 1: #skip first 1 attenuator
                if (_first_1A):
                    _first_1A = False
                    _index_first_1A = i
                    continue
                else:
                    index_numerator = i
                    index_denominator = _index_first_1A
            
                cal1 = sfCalculator(numerator=list_runs[index_numerator], 
                                   denominator=list_runs[index_denominator])
                
                cal1.setNumerator(minPeak=list_peak_back[index_numerator,0], 
                                  maxPeak=list_peak_back[index_numerator,1],
                                  minBack=list_peak_back[index_numerator,2],
                                  maxBack=list_peak_back[index_numerator,3])
                cal1.setDenominator(minPeak=list_peak_back[index_denominator,0], 
                                    maxPeak=list_peak_back[index_denominator,1],
                                    minBack=list_peak_back[index_denominator,2],
                                    maxBack=list_peak_back[index_denominator,3])                

                cal1.run()
                cal1.fit()
                recordSettings(a,b,error_a,error_b,name,cal1)
#                plotObject(cal1)
                
                if (i < (len(list_runs)-1) and
                         list_attenuator[i+1] == 2):
                    list_objects.append(cal1)
            
            
            if list_attenuator[i] == 2:
                if (_first_2A):
                    _first_2A = False
                    _index_first_2A = i
                    continue
                else:
                    index_numerator = i
                    index_denominator = _index_first_2A
                    
                cal2 = sfCalculator(numerator=list_runs[index_numerator], 
                                   denominator=list_runs[index_denominator])

                cal2.setNumerator(minPeak=list_peak_back[index_numerator,0], 
                                  maxPeak=list_peak_back[index_numerator,1],
                                  minBack=list_peak_back[index_numerator,2],
                                  maxBack=list_peak_back[index_numerator,3])
                cal2.setDenominator(minPeak=list_peak_back[index_denominator,0], 
                                    maxPeak=list_peak_back[index_denominator,1],
                                    minBack=list_peak_back[index_denominator,2],
                                    maxBack=list_peak_back[index_denominator,3])                

                cal2.run()

                new_cal2 = cal2 * list_objects[-1]
                new_cal2.fit()
                recordSettings(a,b,error_a,error_b,name,new_cal2)
                plotObject(new_cal2)

                if (i < (len(list_runs)-1) and
                         list_attenuator[i+1] == 3):
                    list_objects.append(new_cal2)


            if list_attenuator[i] == 3: 
                if (_first_3A):
                    _first_3A = False
                    _index_first_3A = i
                    continue
                else:
                    index_numerator = i
                    index_denominator = _index_first_3A
                    
                cal3 = sfCalculator(numerator=list_runs[index_numerator], 
                                   denominator=list_runs[index_denominator])

                cal3.setNumerator(minPeak=list_peak_back[index_numerator,0], 
                                  maxPeak=list_peak_back[index_numerator,1],
                                  minBack=list_peak_back[index_numerator,2],
                                  maxBack=list_peak_back[index_numerator,3])
                cal3.setDenominator(minPeak=list_peak_back[index_denominator,0], 
                                    maxPeak=list_peak_back[index_denominator,1],
                                    minBack=list_peak_back[index_denominator,2],
                                    maxBack=list_peak_back[index_denominator,3])                

                cal3.run()
                new_cal3 = cal3 * list_objects[-1]
                new_cal3.fit()
                recordSettings(a,b,error_a,error_b,name,new_cal3)
                plotObject(new_cal3)
                
                if (i < (len(list_runs)-1) and 
                         list_attenuator[i+1] == 4):
                    list_objects.append(new_cal3)

        
            if list_attenuator[i] == 4: 
                if (_first_4A):
                    _first_4A = False
                    _index_first_4A = i
                    continue
                else:
                    index_numerator = i
                    index_denominator = _index_first_4A
                    
                cal4 = sfCalculator(numerator=list_runs[index_numerator], 
                                   denominator=list_runs[index_denominator])

                cal4.setNumerator(minPeak=list_peak_back[index_numerator,0], 
                                  maxPeak=list_peak_back[index_numerator,1],
                                  minBack=list_peak_back[index_numerator,2],
                                  maxBack=list_peak_back[index_numerator,3])
                cal4.setDenominator(minPeak=list_peak_back[index_denominator,0], 
                                    maxPeak=list_peak_back[index_denominator,1],
                                    minBack=list_peak_back[index_denominator,2],
                                    maxBack=list_peak_back[index_denominator,3])                

                cal4.run()
                new_cal4 = cal4 * list_objects[-1]
                new_cal4.fit()
                recordSettings(a,b,error_a,error_b,name,new_cal4)
                plotObject(new_cal4)
                
                if (i < (len(list_runs)-1) and 
                         list_attenuator[i+1] == 5):
                    list_objects.append(new_cal4)
                
            if list_attenuator[i] == 5: 
                if (_first_5A):
                    _first_5A = False
                    _index_first_5A = i
                    continue
                else:
                    index_numerator = i
                    index_denominator = _index_first_5A
                    
                cal5 = sfCalculator(numerator=list_runs[index_numerator], 
                                   denominator=list_runs[index_denominator])

                cal5.setNumerator(minPeak=list_peak_back[index_numerator,0], 
                                  maxPeak=list_peak_back[index_numerator,1],
                                  minBack=list_peak_back[index_numerator,2],
                                  maxBack=list_peak_back[index_numerator,3])
                cal5.setDenominator(minPeak=list_peak_back[index_denominator,0], 
                                    maxPeak=list_peak_back[index_denominator,1],
                                    minBack=list_peak_back[index_denominator,2],
                                    maxBack=list_peak_back[index_denominator,3])                
                                
                cal5.run()
                new_cal5 = cal5 * list_objects[-1]
                new_cal5.fit()
                recordSettings(a,b,error_a,error_b,name,new_cal5)
                plotObject(new_cal5)

                if (i < (len(list_runs)-1) and 
                         list_attenuator[i+1] == 6):
                    list_objects.append(new_cal5)
        
            if list_attenuator[i] == 6: 
                if (_first_6A):
                    _first_6A = False
                    _index_first_6A = i
                    continue
                else:
                    index_numerator = i
                    index_denominator = _index_first_6A
                    
                cal6 = sfCalculator(numerator=list_runs[index_numerator], 
                                   denominator=list_runs[index_denominator])

                cal6.setNumerator(minPeak=list_peak_back[index_numerator,0], 
                                  maxPeak=list_peak_back[index_numerator,1],
                                  minBack=list_peak_back[index_numerator,2],
                                  maxBack=list_peak_back[index_numerator,3])
                cal6.setDenominator(minPeak=list_peak_back[index_denominator,0], 
                                    maxPeak=list_peak_back[index_denominator,1],
                                    minBack=list_peak_back[index_denominator,2],
                                    maxBack=list_peak_back[index_denominator,3])                

                cal6.run()
                new_cal6 = cal6 * list_objects[-1]
                new_cal6.fit()
                recordSettings(a,b,error_a,error_b,name,new_cal6)
                plotObject(new_cal6)
    
        #output the fitting parameters in an ascii
        output_file_name = '/home/j35/Desktop/SFcalculator.txt'
        outputFittingParameters(a,b,error_a,error_b,name,output_file_name)

    else:
        """
        sort the files 
        """
        pass
    