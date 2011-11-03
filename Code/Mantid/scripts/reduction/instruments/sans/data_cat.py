import os
import sqlite3
import re

def list_all():
	data_dir =  os.path.join(os.path.expanduser("~"), "data/eqsans")
	log_ws = "__log"

	def read_prop(prop):
		try:
			return mtd[log_ws].getRun().getProperty(prop).value
		except:
			return ""

	db = sqlite3.connect("/home/m2d/data_catalog")
	c = db.cursor()
	#c.execute("""create table dataset (run int, title text, start text, duration real)""")

	print "%-6s %-60s %-22s %-10s" % ("Run", "Title", "Start", "Duraction [s]")

	icount = 0
	for f in os.listdir(data_dir):
		if f.endswith("nxs"):
			r_re = re.search("EQSANS_([0-9]+)_event", f)
			if r_re is None:
				continue

			run_number = int(r_re.group(1))
			t = (run_number,)
			c.execute('select * from dataset where run=?', t)
			rows = c.fetchall()

			if len(rows) == 0:
				path = os.path.join(data_dir, f)
				LoadEventNexus(path, OutputWorkspace=log_ws, MetaDataOnly=True)
				runno = int(read_prop("run_number"))
				title = read_prop("run_title")
				run_start = read_prop("start_time")		
				duration = float(read_prop("duration"))

				t = (runno, title, run_start, duration)
				print "%-6s %-60s %-22s %-10s" % (runno, title, run_start, duraction)
				print "inserting", runno, title, run_start, duration

				c.execute('insert into dataset values (?,?,?,?)', t)
			else:
				row = rows[0]
				print "%-6s %-60s %-22s %-10s" % (row[0], row[1], row[2], row[3])

			icount += 1
			#if icount>2:
			#	break
	db.commit()
	c.close()
	
        
    
if __name__ == '__main__':
    list_all()