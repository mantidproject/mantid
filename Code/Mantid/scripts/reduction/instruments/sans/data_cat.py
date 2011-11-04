import os
import sqlite3
import re
import time

class DataSet(object):
	def __init__(self, run_number, title, run_start, duration, ssd):
		self.run_number = run_number
		self.title = title
		self.run_start = run_start
		self.duration = duration
		self.ssd = ssd
		
	@classmethod
	def header(cls):
		return "%-6s %-60s %-16s %-7s %-10s" % ("Run", "Title", "Start", "Time[s]", "SDD[mm]")
	def __str__(self):
		return "%-6s %-60s %-16s %-7g %-10.0f" % (self.run_number, self.title, self.run_start, self.duration, self.ssd)
	
	def from_file(self, filepath):
		pass
	
	def store(self):
		pass
	
	@classmethod
	def find(cls, file_path, cursor):
		r_re = re.search("EQSANS_([0-9]+)_event", file_path)
		if r_re is None:
			return None
		
		t = (int(r_re.group(1)),)
		cursor.execute('select * from dataset where run=?', t)
		rows = cursor.fetchall()

		if len(rows) == 0:
			log_ws = "__log"
			def read_prop(prop):
				try:
					return mtd[log_ws].getRun().getProperty(prop).value
				except:
					return ""
			def read_series(prop):
				try:
					return mtd[log_ws].getRun().getProperty(prop).getStatistics().mean
				except:
					print sys.exc_value
					return -1
			
			LoadEventNexus(file_path, OutputWorkspace=log_ws, MetaDataOnly=True)
			runno = int(read_prop("run_number"))
			title = read_prop("run_title")
			t_str = read_prop("start_time")
			t = time.strptime(t_str, '%Y-%m-%dT%H:%M:%S')
			run_start = time.strftime('%y-%m-%d %H:%M', t)
			duration = float(read_prop("duration"))
			sdd = float(read_series("detectorZ"))

			t = (runno, title, run_start, duration, sdd)
			cursor.execute('insert into dataset values (?,?,?,?,?)', t)
			return DataSet(runno, title, run_start, duration, sdd)
		else:
			row = rows[0]
			return DataSet(row[0], row[1], row[2], row[3], row[4])


def list_all(data_dir=None, replace_db=False):
	"""
		Returns a list of the data sets as text
	"""
	
	# Data directory
	if data_dir is None:
		data_dir =  os.path.join(os.path.expanduser("~"), "data/eqsans")

	# Connect/create to DB
	db_path = os.path.join(os.path.expanduser("~"), ".mantid_data_sets")
	db_exists = False
	if os.path.isfile(db_path):
		if replace_db:
			os.remove(db_path)
		else: 
			db_exists = True
		
	db = sqlite3.connect(db_path)
	c = db.cursor()
	
	if not db_exists:
		c.execute("""create table dataset (run int, title text, start text, duration real, ssd real)""")

	output = "%s\n" % DataSet.header()

	for f in os.listdir(data_dir):
		if f.endswith("nxs"):
			path = os.path.join(data_dir, f)
			d = DataSet.find(path, c)
			if d is not None:
				output += "%s\n" % str(d)

	db.commit()
	c.close()
	return output
	
if __name__ == '__main__':
    print list_all(replace_db=False)