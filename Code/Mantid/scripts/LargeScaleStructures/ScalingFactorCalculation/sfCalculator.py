from MantidFramework import *
from mantidsimple import *
#from numpy import zeros
from pylab import *
import time

class sfSetup:
        
    nexus_path = '/mnt/hgfs/j35/'
#    list_runs = {'D0': 55889,    #0
#             'AiD0': 55890,  #1
#             'AiD1': 55891,  #2
#             'AiD2': 55892,  #3
#             'AiD3': 55893,  #4
#             'AiD4': 55894,  #5
#             'AiD5': 55895,  #6
#             'AiiAiD5': 55896,  #7
#             'AiiAiD6': 55897,  #8
#             'AiiAiD7': 55898,  #9
#             'AiiiAiiAiD7': 55899,  #10
#             'AiiiAiiAiD8': 55900,  #11
#             'AivAiiiAiiAiD8': 55901,  #12
#             'AivAiiiAiiAiD9': 55902}  #13
    
    list_runs = []
    
    pre = 'REF_L_'
    post = '_event.nxs'

    #new way to specify input
    #run number: nbr of attenuators

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
    
    y_axis_ratio = None
    y_axis_error_ratio = None
    x_axis_ratio = None

    #fitting parameters  y=a+b*x
    a = None
    b = None
    error_a = None
    error_b = None
            
    def __init__(self):
        self.x_axis_ratio = None
        self.y_axis_ratio = None
        self.y_axis_error_ratio = None
    
    def setNexusPath(self, nexus_path=''):
        """
        Sets the full path to the folder where the data are located
        """
        if nexus_path == '':
            return
        
        self.nexus_path = nexus_path
        
    def setListRuns(self, list_runs):
        """
        Define the dictionnary 
        """
        self.list_runs = list_runs
    
    def setXPixelRange(self, pixel_range=[-1,-1]):
        """
        This will set the X range of pixels to avoid the frame effect we found by plotting Y vs X 
        integrated over the full range of TOF
        """
        if size(pixel_range) != 2:
            return
        
        if (pixel_range[0] != -1):
            self.x_pixel_min = pixel_range[0]
            
        if (pixel_range[1] != -1):
            self.x_pixel_max = pixel_range[1]
            
    def setTofRange(self, TOF_range=[-1,-1]):
        """
        Select the range of TOF in microS for the calculation
        """
        if size(TOF_range) != 2:
            return
        
        if (TOF_range[0] != -1):
            self.tof_min = TOF_range[0]
            
        if (TOF_range[1] != -1):
            self.tof_max = TOF_range[1]
            
    def setPeakPixelRange(self, peak_range=[-1,-1]):
        """
        Select the range of peak pixels
        """
        if size(peak_range) != 2:
            return
        
        if peak_range[0] != -1:
            self.peak_pixel_min = peak_range[0]
        
        if peak_range[1] != -1:
            self.peak_pixel_max = peak_range[1]
            
    def setBackPixelRange(self, back_range=[-1,-1]):
        """
        Select the range of background pixels
        """ 
        if size(back_range) != 2:
            return
        
        if back_range[0] != -1:
            self.back_pixel_min = back_range[0]
            
        if back_range[1] != -1:
            self.back_pixel_max = back_range[1]
            
    def setRebinParameters(self, para=None):
        """
        Set the min, max and width parameters for the rebinning
        'min,width,max'
        """
        if para is None:
            return   
            
        self.rebin_parameters = para    
            
    def __mul__(self, other):
        """
        operator * between two instances of the class
        """
        
        product = sfSetup()
        
        product.numerator = self.numerator + '*' + other.numerator
        product.denominator = self.denominator + '*' + other.denominator
        
        product.x_axis_ratio = self.x_axis_ratio
        product.y_axis_ratio = self.y_axis_ratio * other.y_axis_ratio
        product.y_axis_error_ratio = sqrt((other.y_axis_ratio*self.y_axis_error_ratio)**2 +
                                          (other.y_axis_error_ratio*self.y_axis_ratio)**2)
        return product
    
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
                
class sfCalculator(sfSetup):      
    
    #name of numerators and denominators
    numerator = None #ex: AiD0
    denominator = None   #ex: AiD1

    _y_axis_numerator = None
    _y_axis_error_numerator = None
    _y_axis_denominator = None
    _y_axis_error_denominator = None
    _x_axis = None     
         
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

    _y_axis_ratio = None
    _y_axis_error_ratio = None
    _x_axis_ratio = None
            
    def __init__(self, numerator=None, denominator=None):
        
        self.numerator = numerator
        self.denominator = denominator

        self.x_axis_ratio = None
        self.y_axis_error_ratio = None
        self.y_axis_ratio = None
        
        #Launch the calculation
        #self.run()

    def setNumerator(self, minPeak=None, maxPeak=None, minBack=None, maxBack=None):
        if minPeak is not None:
            self.n_peak_pixel_min = minPeak
        if maxPeak is not None:
            self.n_peak_pixel_max = maxPeak
        if minBack is not None:
            self.n_back_pixel_min = minBack
        if maxBack is not None:
            self.n_back_pixel_max = maxBack
        
    def setDenominator(self, minPeak=None, maxPeak=None, minBack=None, maxBack=None):
        if minPeak is not None:
            self.d_peak_pixel_min = minPeak
        if maxPeak is not None:
            self.d_peak_pixel_max = maxPeak
        if minBack is not None:
            self.d_back_pixel_min = minBack
        if maxBack is not None:
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
        self.y_axis_ratio = self._y_axis_numerator/self._y_axis_denominator
        self.y_axis_error_ratio = ((self._y_axis_error_numerator/self._y_axis_numerator)**2 +
                                    (self._y_axis_error_denominator/self._y_axis_denominator)**2)
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
        _x_axis = mt2.readX(0)[:]
        self._x_axis = _x_axis
        
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
        _x_axis = mt.readX(0)[:]
        self._x_axis = _x_axis
        
        counts_vs_tof = zeros(len(_x_axis))
        counts_vs_tof_error = zeros(len(_x_axis))
        for x in range(self.alpha_pixel_nbr):
            counts_vs_tof += mt.readY(x)[:]
            counts_vs_tof_error += mt.readE(x)[:]**2
        counts_vs_tof_error = sqrt(counts_vs_tof_error)
        index_tof_min = self._getIndex(self.tof_min, _x_axis)
        index_tof_max = self._getIndex(self.tof_max, _x_axis)

        if (bNumerator is True):
            self._y_axis_numerator = counts_vs_tof[index_tof_min:index_tof_max]
            self._y_axis_error_numerator = counts_vs_tof_error[index_tof_min:index_tof_max]
            self.x_axis_ratio = self._x_axis[index_tof_min:index_tof_max]
        else:
            self._y_axis_denominator = counts_vs_tof[index_tof_min:index_tof_max]
            self._y_axis_error_denominator = counts_vs_tof_error[index_tof_min:index_tof_max]
            self.x_axis_ratio = self._x_axis[index_tof_min:index_tof_max]

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
        _x_axis = InputWorkspace.readX(0)[:]
        x_size = to_pixel - from_pixel + 1 
        _y_axis = zeros((self.alpha_pixel_nbr, len(_x_axis) - 1))
        _y_error_axis = zeros((self.alpha_pixel_nbr, len(_x_axis) - 1))
        y_range = arange(x_size) + from_pixel
        for x in range(self.beta_pixel_nbr):
            for y in y_range:
                _index = int(self.alpha_pixel_nbr * x + y)
                _y_axis[y, :] += InputWorkspace.readY(_index)[:]
                _y_error_axis[y, :] += ((InputWorkspace.readE(_index)[:]) * 
                                        (InputWorkspace.readE(_index)[:]))

        _y_axis = _y_axis.flatten()
        _y_error_axis = sqrt(_y_error_axis)
        #plot_y_error_axis = _y_error_axis #for output testing only    -> plt.imshow(plot_y_error_axis, aspect='auto', origin='lower')
        _y_error_axis = _y_error_axis.flatten()

        #normalization by proton charge
        _y_axis /= (proton_charge * 1e-12)

        CreateWorkspace(OutputWorkspace, 
                        DataX=_x_axis, 
                        DataY=_y_axis, 
                        DataE=_y_error_axis, 
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
        
        _previous_cal=None
        _keep_cal=None
        
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
            
                cal = sfCalculator(numerator=list_runs[index_numerator], 
                                   denominator=list_runs[index_denominator])
                cal.run()
                cal.fit()
                recordSettings(a,b,error_a,error_b,name,cal)
                #plotObject(cal)
                _keep_cal=cal
            
            if list_attenuator[i] == 2:
                if (_first_2A):
                    _first_2A = False
                    _index_first_2A = i
                    _previous_cal = _keep_cal 
                    continue
                else:
                    index_numerator = i
                    index_denominator = _index_first_2A
                    
                print _previous_cal.numerator
                print _previous_cal.denominator
                plotObject(_previous_cal)
                cal = sfCalculator(numerator=list_runs[index_numerator], 
                                   denominator=list_runs[index_denominator])
                print _previous_cal.numerator
                print _previous_cal.denominator
                plotObject(_previous_cal)
                cal.run()
                print _previous_cal.numerator
                print _previous_cal.denominator
                plotObject(_previous_cal)

                new_cal = cal * _previous_cal
                new_cal.fit()
                recordSettings(a,b,error_a,error_b,name,new_cal)
                #plotObject(new_cal)
                _keep_cal=new_cal

                break

            if list_attenuator[i] == 3: 
                if (_first_3A):
                    _first_3A = False
                    _index_first_3A = i
                    _previous_cal = _keep_cal
                    continue
                else:
                    index_numerator = i
                    index_denominator = _index_first_3A
                    
                cal = sfCalculator(numerator=list_runs[index_numerator], 
                                   denominator=list_runs[index_denominator])
                cal.run()
                new_cal = cal * _previous_cal
                new_cal.fit()
                recordSettings(a,b,error_a,error_b,name,new_cal)
                plotObject(new_cal)
                _keep_cal = new_cal
        
            if list_attenuator[i] == 4: 
                if (_first_4A):
                    _first_4A = False
                    _index_first_4A = i
                    _previous_cal = _keep_cal
                    continue
                else:
                    index_numerator = i
                    index_denominator = _index_first_4A
                    
                cal = sfCalculator(numerator=list_runs[index_numerator], 
                                   denominator=list_runs[index_denominator])
                cal.run()
                new_cal = cal * _previous_cal
                new_cal.fit()
                recordSettings(a,b,error_a,error_b,name,new_cal)
                plotObject(new_cal)
                _keep_cal = new_cal
        
            if list_attenuator[i] == 5: 
                if (_first_5A):
                    _first_5A = False
                    _index_first_5A = i
                    _previous_cal = _keep_cal
                    continue
                else:
                    index_numerator = i
                    index_denominator = _index_first_5A
                    
                cal = sfCalculator(numerator=list_runs[index_numerator], 
                                   denominator=list_runs[index_denominator])
                cal.run()
                new_cal = cal * _previous_cal
                new_cal.fit()
                recordSettings(a,b,error_a,error_b,name,new_cal)
                plotObject(new_cal)
                _keep_cal = new_cal
        
            if list_attenuator[i] == 6: 
                if (_first_6A):
                    _first_6A = False
                    _index_first_6A = i
                    _previous_cal = _keep_cal
                    continue
                else:
                    index_numerator = i
                    index_denominator = _index_first_6A
                    
                cal = sfCalculator(numerator=list_runs[index_numerator], 
                                   denominator=list_runs[index_denominator])
                cal.run()
                new_cal = cal * _previous_cal
                new_cal.fit()
                recordSettings(a,b,error_a,error_b,name,new_cal)
                plotObject(new_cal)
                _keep_cal = new_cal
    
        #output the fitting parameters in an ascii
        output_file_name = '/home/j35/Desktop/SFcalculator.txt'
        outputFittingParameters(a,b,error_a,error_b,name,output_file_name)

    else:
        """
        sort the files 
        """
        pass
    