#pylint: disable=invalid-name,no-init,too-many-locals,too-many-branches

from mantid.simpleapi import *
from mantid.kernel import *
from mantid.api import *

import os
import numpy as np
import math


def _split_line(a):
    elements = a.split()  # split line on character
    extracted = []
    for n in elements:
        extracted.append(float(n))
    return extracted  # values as list


def _find_starts(data, c, l1):
    for l in range(l1, len(data)):
        char = data[l]
        if char.startswith(c):
            line = l
            break
    return line


def _find_tab_starts(data, c, l1):
    for l in range(l1, len(data)):
        char = data[l][1:]
        if char.startswith(c):
            line = l
            break
    return line


def _find_ends(data, c, l1):
    for l in range(l1, len(data)):
        char = data[l]
        if char.endswith(c):
            line = l
            break
    return line


def _find_char(data, c, l1):
    for l in range(l1, len(data)):
        char = data[l]
        if char.find(c):
            line = l
            break
    return line


def _make_list(a, l1, l2):
    data = ''
    for m in range(l1, l2 + 1):
        data += a[m]
        alist = data.split(',')
    return alist


class LoadNMoldyn3Ascii(PythonAlgorithm):

    _file_name = None
    _functions = None
    _out_ws = None


    def category(self):
        return 'PythonAlgorithms;Inelastic;Simulation'


    def summary(self):
        return 'Imports functions from CDL and ASCII files output by nMOLDYN 3.'


    def PyInit(self):
        self.declareProperty(FileProperty('Filename', '',
                                          action=FileAction.Load,
                                          extensions=['.cdl', '.dat']),
                                          doc='File path for data')

        self.declareProperty(StringArrayProperty('Functions'),
                             doc='Names of functions to attempt to load from file')

        self.declareProperty(WorkspaceProperty('OutputWorkspace', '',
                                               direction=Direction.Output),
                             doc='Output workspace name')


    def validateInputs(self):
        issues = dict()

        sample_filename = self.getPropertyValue('Filename')
        file_type = os.path.splitext(sample_filename)[1]
        function_list = self.getProperty('Functions').value

        if len(function_list) == 0 and file_type == '.cdl':
            issues['Functions'] = 'Must specify at least one function when loading a CDL file'

        if len(function_list) > 0 and file_type == '.dat':
            issues['Functions'] = 'Cannot specify functions when loading an ASCII file'

        return issues


    def PyExec(self):
        # Do setup
        self._setup()

        # Load data file
        data, name, ext = self._load_file()

        # Run nMOLDYN import
        if ext == 'cdl':
            self._cdl_import(data, name)
        elif ext == 'dat':
            self._ascii_import(data, name)
        else:
            raise RuntimeError('Unrecognised file format: %s' % ext)

        # Set the output workspace
        self.setProperty('OutputWorkspace', self._out_ws)


    def _setup(self):
        """
        Gets algorithm properties.
        """
        self._file_name = self.getPropertyValue('Filename')
        self._out_ws = self.getPropertyValue('OutputWorkspace')

        raw_functions = self.getProperty('Functions').value
        self._functions = [x.strip() for x in raw_functions]


    def _load_file(self):
        """
        Attempts to load the sample file.

        @returns A tuple with the ASCII data, sample file name and file extension
        """

        # Get some data about the file
        path = self._file_name
        base = os.path.basename(path)
        name = os.path.splitext(base)[0]
        ext = os.path.splitext(path)[1]

        # Remove dot from extension
        if len(ext) > 1:
            ext = ext[1:]

        logger.debug('Base filename for %s is %s' % (self._file_name, name))
        logger.debug('File type of %s is %s' % (self._file_name, ext))

        if not os.path.isfile(path):
            path = FileFinder.getFullPath(path)

        logger.information('Got file path for %s: %s' % (self._file_name, path))

        # Open file and get data
        handle = open(path, 'r')
        data = []
        for line in handle:
            line = line.rstrip()
            data.append(line)
        handle.close()

        return data, name, ext


    def _find_dimensions(self, data):
        """
        Gets the number of Q, time and frequency values in given raw data.

        @param data Raw data to search
        """

        num_q_values = _find_tab_starts(data, 'NQVALUES', 0)
        num_time_values = _find_tab_starts(data, 'NTIMES', 0)
        num_freq_values = _find_tab_starts(data, 'NFREQUENCIES', 0)

        q_el = data[num_q_values].split()
        num_q = int(q_el[2])
        t_el = data[num_time_values].split()
        num_t = int(t_el[2])
        f_el = data[num_freq_values].split()
        num_f = int(f_el[2])

        logger.debug(data[2][1:-1])
        logger.debug(data[3][1:-1])
        logger.debug(data[6][1:-1])

        return num_q, num_t, num_f


    def _cdl_import(self, data, name):
        """
        Import data from CDL file.

        @param data Raw data
        @param name Name of data file
        """

        logger.notice('Loading CDL file: %s' % name)

        len_data = len(data)

        # raw head
        nQ, nT, nF = self._find_dimensions(data)
        ldata = _find_starts(data, 'data:', 0)
        lq1 = _find_starts(data, ' q =', ldata)  # start Q values
        lq2 = _find_starts(data, ' q =', lq1 - 1)
        Qlist = _make_list(data, lq1, lq2)
        if nQ != len(Qlist):
            raise RUntimeError('Error reading Q values')
        Qf = Qlist[0].split()
        Q = [float(Qf[2]) / 10.0]
        for m in range(1, nQ - 1):
            Q.append(float(Qlist[m]) / 10.0)

        Q.append(float(Qlist[nQ - 1][:-1]) / 10.0)
        logger.information('Q values = ' + str(Q))

        lt1 = _find_starts(data, ' time =', lq2)  # start T values
        lt2 = _find_ends(data, ';', lt1)
        Tlist = _make_list(data, lt1, lt2)
        if nT != len(Tlist):
            raise RuntimeError('Error reading Time values')

        Tf = Tlist[0].split()
        T = [float(Tf[2])]
        for m in range(1, nT - 1):
            T.append(float(Tlist[m]))

        T.append(float(Tlist[nT - 1][:-1]))
        T.append(2 * T[nT - 1] - T[nT - 2])
        logger.information('T values = ' + str(T[:2]) + ' to ' + str(T[-3:]))

        lf1 = _find_starts(data, ' frequency =', lq2)  # start F values
        lf2 = _find_ends(data, ';', lf1)
        Flist = _make_list(data, lf1, lf2)
        if nF != len(Flist):
            raise RuntimeError('Error reading Freq values')

        Ff = Flist[0].split()
        F = [float(Ff[2])]
        for m in range(1, nF - 1):
            F.append(float(Flist[m]))

        F.append(float(Flist[nF - 1][:-1]))
        F.append(2 * F[nF - 1] - T[nF - 2])
        logger.information('F values = ' + str(F[:2]) + ' to ' + str(F[-3:]))

        # Function
        output_ws_list = list()
        for func in self._functions:
            start = []
            lstart = lt2
            if func[:3] == 'Fqt':
                nP = nT
                xEn = np.array(T)
                eZero = np.zeros(nT)
                xUnit = 'TOF'
            elif func[:3] == 'Sqw':
                nP = nF
                xEn = np.array(F)
                eZero = np.zeros(nF)
                xUnit = 'Energy'
            else:
                raise RuntimeError('Failed to parse function string ' + func)

            for n in range(0, nQ):
                for m in range(lstart, len_data):
                    char = data[m]
                    if char.startswith('  // ' + func):
                        start.append(m)
                        lstart = m + 1
            lend = _find_ends(data, ';', lstart)
            start.append(lend + 1)

            # Throw error if we couldn't find the function
            if len(start) < 2:
                raise RuntimeError('Failed to parse function string ' + func)

            Qaxis = ''
            for n in range(0, nQ):
                logger.information(str(start))
                logger.information('Reading : ' + data[start[n]])

                Slist = _make_list(data, start[n] + 1, start[n + 1] - 1)
                if n == nQ - 1:
                    Slist[nP - 1] = Slist[nP - 1][:-1]
                S = []
                for m in range(0, nP):
                    S.append(float(Slist[m]))
                if nP != len(S):
                    raise RuntimeError('Error reading S values')
                else:
                    logger.information('S values = ' + str(S[:2]) + ' to ' + str(S[-2:]))
                if n == 0:
                    Qaxis += str(Q[n])
                    xDat = xEn
                    yDat = np.array(S)
                    eDat = eZero
                else:
                    Qaxis += ',' + str(Q[n])
                    xDat = np.append(xDat, xEn)
                    yDat = np.append(yDat, np.array(S))
                    eDat = np.append(eDat, eZero)
            outWS = name + '_' + func

            CreateWorkspace(OutputWorkspace=outWS,
                            DataX=xDat,
                            DataY=yDat,
                            DataE=eDat,
                            Nspec=nQ,
                            UnitX=xUnit,
                            VerticalAxisUnit='MomentumTransfer',
                            VerticalAxisValues=Qaxis)

            output_ws_list.append(outWS)

        GroupWorkspaces(InputWorkspaces=output_ws_list,
                        OutputWorkspace=self._out_ws)


    def _ascii_import(self, data, name):
        """
        Import ASCII data.

        @param data Raw ASCII data
        @param name Name of data file
        """

        from IndirectCommon import getEfixed
        from IndirectNeutron import ChangeAngles, InstrParas

        logger.notice('Loading ASCII data: %s' % name)

        val = _split_line(data[3])
        Q = []
        for n in range(1, len(val)):
            Q.append(val[n])

        nQ = len(Q)
        x = []
        y = []
        for n in range(4, len(data)):
            val = _split_line(data[n])
            x.append(val[0])
            yval = val[1:]
            y.append(yval)

        nX = len(x)
        logger.information('nQ = ' + str(nQ))
        logger.information('nT = ' + str(nX))

        xT = np.array(x)
        eZero = np.zeros(nX)
        #Qaxis = ''
        for m in range(0, nQ):
            logger.information('Q[' + str(m + 1) + '] : ' + str(Q[m]))

            S = []
            for n in range(0, nX):
                S.append(y[n][m])

            if m == 0:
                #Qaxis += str(Q[m])
                xDat = xT
                yDat = np.array(S)
                eDat = eZero
            else:
                #Qaxis += ',' + str(Q[m])
                xDat = np.append(xDat, xT)
                yDat = np.append(yDat, np.array(S))
                eDat = np.append(eDat, eZero)

        CreateWorkspace(OutputWorkspace=self._out_ws,
                        DataX=xDat,
                        DataY=yDat,
                        DataE=eDat,
                        Nspec=nQ,
                        UnitX='TOF')

        Qmax = Q[nQ - 1]
        instr = 'MolDyn'
        ana = 'simul'
        if Qmax <= 2.0:
            refl = '2'
        else:
            refl = '4'

        InstrParas(self._out_ws, instr, ana, refl)
        efixed = getEfixed(self._out_ws)
        logger.information('Qmax = ' + str(Qmax) + ' ; efixed = ' + str(efixed))
        pi4 = 4.0 * math.pi
        wave = 1.8 * math.sqrt(25.2429 / efixed)
        theta = []
        for n in range(0, nQ):
            qw = wave * Q[n] / pi4
            ang = 2.0 * math.degrees(math.asin(qw))
            theta.append(ang)

        ChangeAngles(self._out_ws, instr, theta)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(LoadNMoldyn3Ascii)
