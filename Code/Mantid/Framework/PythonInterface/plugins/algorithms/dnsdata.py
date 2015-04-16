import sys, re
import datetime

class DNSdata:
    """
    class which describes the DNS data structure
    will be used for data read-in and write-out routines
    """
    def __init__(self):
        self.title = ""
        self.experiment_number = ""
        self.run_number = ""
        self.start_time = ""
        self.end_time = ""
        self.duration = None
        self.deterota = 0
        self.wavelength = None          # Angstrom
        self.incident_energy = None     # meV
        self.ndet = 24
        self.sample_name = ""
        self.userid = ""
        self.user_name = ""
        self.sample_description = ""
        self.coil_status = ""
        self.befilter_status = ""
        self.notes = ""
        self.monochromator_angle = None         # degree
        self.monochromator_position = None
        self.huber = None
        self.cradle_lower = None
        self.cradle_upper = None
        self.slit_i_upper_blade_position = None
        self.slit_i_lower_blade_position = None
        self.slit_i_left_blade_position = None
        self.slit_i_right_blade_position = None
        self.slit_f_upper_blade_position = None
        self.slit_f_lower_blade_position = None
        self.detector_position_vertical = None
        self.polarizer_translation = None
        self.polarizer_rotation = None
        self.flipper_precession_current = None
        self.flipper_z_compensation_current = None
        self.a_coil_current = None
        self.b_coil_current = None
        self.c_coil_current = None
        self.z_coil_current = None
        self.t1 = None       # T1
        self.t2 = None       # T2
        self.tsp = None      # T_setpoint
        self.tof_channel_number = None
        self.tof_channel_width = None
        self.tof_delay_time = None
        self.tof_elastic_channel = None
        self.chopper_rotation_speed = None
        self.chopper_slits = None
        self.monitor_counts = None


    def read_legacy(self, filename):
        """
        reads the DNS legacy ascii file into the DNS data object
        """
        with open(filename, 'r') as fhandler:
            # read file content and split it into blocks
            splitsymbol = \
                    '#--------------------------------------------------------------------------'
            unparsed = fhandler.read()
            blocks = unparsed.split(splitsymbol)

            # parse each block
            # parse block 0 (header)
            res = parse_header(blocks[0])
            #if not res: raise Exception "wrong file format" else
            try:
                self.run_number = res['file']
                self.experiment_number = res['exp']
                self.sample_name = res['sample']
                self.userid = res['userid']
            except:
                raise ValueError("The file %s does not contain valid DNS data format." % filename)
            # parse block 1 (general information)
            b1splitted = map(str.strip, blocks[1].split('#'))
            b1rest = [el for el in b1splitted]
            r_user = re.compile("User:\s*(?P<name>.*?$)")
            r_sample = re.compile("Sample:\s*(?P<sample>.*?$)")
            r_coil = re.compile("^(?P<coil>.*?)\s*xyz-coil.*")
            r_filter = re.compile("^(?P<filter>.*?)\s*Be-filter.*")
            for line in b1splitted:
                res = r_user.match(line)
                if res:
                    self.user_name = res.group("name")
                    b1rest.remove(line)
                res = r_sample.match(line)
                if res:
                    self.sample_description = res.group("sample")
                    b1rest.remove(line)
                res = r_coil.match(line)
                if res:
                    self.coil_status = res.group("coil")
                    b1rest.remove(line)
                res = r_filter.match(line)
                if res:
                    self.befilter_status = res.group("filter")
                    b1rest.remove(line)
                # the rest unparsed lines go to notes for the moment
                # [TODO]: parse more information about the sample
                self.notes = ' '.join(b1rest)

            # parse block 2 (wavelength and mochromator angle)
            # for the moment, only theta and lambda are needed
            b2splitted = map(str.strip, blocks[2].split('#'))
            # assume that theta and lambda are always on the fixed positions
            # assume theta is give in degree, lambda in nm
            line = b2splitted[2].split()
            self.monochromator_angle = float(line[2])
            self.wavelength = float(line[3])*10.0
            self.incident_energy = float(line[4])

            # parse block 3 (motors position)
            b3splitted = map(str.strip, blocks[3].split('#'))
            self.monochromator_position = float(b3splitted[2].split()[1])
            # DeteRota, angle of rotation of detector bank
            self.deterota = float(b3splitted[3].split()[1])
            # Huber default units degree
            self.huber = float(b3splitted[5].split()[1])
            self.cradle_lower = float(b3splitted[6].split()[1])
            self.cradle_upper = float(b3splitted[7].split()[1])
            # Slit_i, convert mm to meter
            self.slit_i_upper_blade_position = \
                    0.001*float(b3splitted[9].split()[2])
            self.slit_i_lower_blade_position = \
                    0.001*float(b3splitted[10].split()[1])
            self.slit_i_left_blade_position = \
                    0.001*float(b3splitted[11].split()[2])
            self.slit_i_right_blade_position = \
                    0.001*float(b3splitted[12].split()[1])
            # Slit_f
            self.slit_f_upper_blade_position = \
                    0.001*float(b3splitted[14].split()[1])
            self.slit_f_lower_blade_position = \
                    0.001*float(b3splitted[15].split()[1])
            # Detector_position vertical
            self.detector_position_vertical = \
                    0.001*float(b3splitted[16].split()[1])
            # Polarizer
            self.polarizer_translation = \
                    0.001*float(b3splitted[19].split()[1])
            self.polarizer_rotation = float(b3splitted[20].split()[1])

            # parse block 4 (B-fields), only currents in A are taken
            b4splitted = map(str.strip, blocks[4].split('#'))
            self.flipper_precession_current = float(b4splitted[2].split()[1])
            self.flipper_z_compensation_current = float(b4splitted[3].split()[1])
            self.a_coil_current = float(b4splitted[4].split()[1])
            self.b_coil_current = float(b4splitted[5].split()[1])
            self.c_coil_current = float(b4splitted[6].split()[1])
            self.z_coil_current = float(b4splitted[7].split()[1])


            # parse block 5 (Temperatures)
            # assume: T1=cold_head_temperature, T2=sample_temperature
            b5splitted = map(str.strip, blocks[5].split('#'))
            self.t1 = float(b5splitted[2].split()[1])
            self.t2 = float(b5splitted[3].split()[1])
            self.tsp = float(b5splitted[4].split()[1])

            # parse block 6 (TOF parameters)
            b6splitted = map(str.strip, blocks[6].split('#'))
            self.tof_channel_number = int(b6splitted[2].split()[2])
            if self.tof_channel_number > 1:
                self.tof_channel_width = float(b6splitted[3].split()[3])
                self.tof_delay_time = float(b6splitted[4].split()[2])
                self.tof_elastic_channel = int(b6splitted[6].split()[3])
                # chopper rotation speed
                self.chopper_rotation_speed = float(b6splitted[7].split()[2])
                # chopper number of slits
                self.chopper_slits = int(b6splitted[5].split()[2])

            # parse block 7 (Time and monitor)
            # assume everything to be at the fixed positions
            b7splitted = map(str.strip, blocks[7].split('#'))
            # duration
            line = b7splitted[2].split()
            self.duration = float(line[1]) # assume seconds [TODO]: check
            # monitor data
            line = b7splitted[3].split()
            self.monitor_counts = int(line[1])
            # start_time and end_time (if specified)
            outfmt = "%Y-%m-%dT%H:%M:%S"
            sinfmt = "start   at %a %b  %d %H:%M:%S %Y"
            einfmt = "stopped at %a %b  %d %H:%M:%S %Y"
            try:
                self.start_time = datetime.datetime.strptime(b7splitted[5], sinfmt).strftime(outfmt)
                self.end_time = datetime.datetime.strptime(b7splitted[6], einfmt).strftime(outfmt)
            except ValueError:
                # if start and end time are not given, let them empty
                pass


def parse_header(h):
    """
    parses the header string and returns the parsed dictionary
    """
    d = {}
    regexp = re.compile("(\w+)=(\w+)")
    result = regexp.finditer(h)
    for r in result:
        d[r.groups()[0]] = r.groups()[1]
    return d


if __name__ == '__main__':
    fname = sys.argv[1]
    dns_data = DNSdata()
    dns_data.read_legacy(fname)
    print dns_data.__dict__





