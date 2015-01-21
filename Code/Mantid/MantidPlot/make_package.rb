#!/usr/bin/env ruby
#This loop changes the linking from /usr/local/lib to @rpath

require 'pathname'

library_filenames = ["/usr/local/lib/libboost_regex-mt.dylib",
                     "/usr/local/lib/libboost_date_time-mt.dylib",
                     "/usr/local/lib/libboost_python-mt.dylib",
                     "/usr/local/lib/libgsl.0.dylib",
                     "/usr/local/lib/libgslcblas.0.dylib",
                     "/usr/local/lib/libjsoncpp.dylib",
		     "/usr/local/lib/libmuparser.2.dylib",
                     "/usr/local/lib/libNeXus.0.dylib",
                     "/usr/local/lib/libNeXusCPP.0.dylib",
                     "/usr/local/lib/libPocoFoundation.17.dylib",
                     "/usr/local/lib/libPocoUtil.17.dylib",
                     "/usr/local/lib/libPocoXML.17.dylib",
                     "/usr/local/lib/libPocoNet.17.dylib",
                     "/usr/local/lib/libPocoCrypto.17.dylib",
                     "/usr/local/lib/libPocoNetSSL.17.dylib",
                     "/usr/local/opt/openssl/lib/libssl.1.0.0.dylib",
                     "/usr/local/opt/openssl/lib/libcrypto.1.0.0.dylib",
                     "/usr/local/lib/libTKernel.9.dylib",
                     "/usr/local/lib/libTKBO.9.dylib",
	             "/usr/local/lib/libTKernel.9.dylib", 
	             "/usr/local/lib/libTKBO.9.dylib", 
	             "/usr/local/lib/libTKPrim.9.dylib", 
	             "/usr/local/lib/libTKMesh.9.dylib", 
	             "/usr/local/lib/libTKBRep.9.dylib",
                     "/usr/local/lib/libTKGeomAlgo.9.dylib", 
	             "/usr/local/lib/libTKTopAlgo.9.dylib", 
	             "/usr/local/lib/libTKMath.9.dylib", 
	             "/usr/local/lib/libTKG2d.9.dylib",
	             "/usr/local/lib/libTKG3d.9.dylib", 
	             "/usr/local/lib/libTKGeomBase.9.dylib",
	             "/usr/local/lib/libqwt.5.dylib",
	             "/usr/local/lib/libqwtplot3d.0.dylib",
	             "/usr/local/lib/libqscintilla2.11.dylib",
                     "/usr/local/lib/libmxml.dylib",
                     "/usr/local/lib/libhdf5.9.dylib",
                     "/usr/local/lib/libhdf5_hl.9.dylib",
                     "/usr/local/lib/libmfhdf.4.2.10.dylib",
                     "/usr/local/lib/libdf.4.2.10.dylib",
                     "/usr/local/lib/libsz.2.dylib",
                     "/usr/local/lib/libjpeg.8.dylib"
]

library_filenames.each do |filename|
  basename = File.basename(filename)
  `cp #{filename} Contents/MacOS/`
  `chmod +w Contents/MacOS/#{basename}`
  `install_name_tool -id @rpath/#{basename} Contents/MacOS/#{basename}`
end

library_filenames.push("/usr/local/Cellar/poco/1.4.7p1/lib/libPocoFoundation.17.dylib")
library_filenames.push("/usr/local/Cellar/openssl/1.0.1k/lib/libcrypto.1.0.0.dylib")
library_filenames.push("/usr/local/Cellar/poco/1.4.7p1/lib/libPocoNet.17.dylib")
library_filenames.push("/usr/local/Cellar/poco/1.4.7p1/lib/libPocoCrypto.17.dylib")
library_filenames.push("/usr/local/Cellar/poco/1.4.7p1/lib/libPocoUtil.17.dylib")
library_filenames.push("/usr/local/Cellar/hdf4/4.2.10/lib/libdf.4.2.10.dylib")
library_filenames.push("/usr/local/Cellar/hdf5/1.8.14/lib/libhdf5.9.dylib")
library_filenames.push("/usr/local/Cellar/nexusformat/4.3.3/lib/libNeXus.0.dylib")
library_filenames.push("/usr/local/Cellar/poco/1.4.7p1/lib/libPocoXML.17.dylib")


search_patterns = ["**/*.dylib","**/*.so","**/MantidPlot"]
search_patterns.each do |pattern|
  Dir[pattern].each do |library|
    p "library: #{library}"
    dependencies = `otool -L #{library}`
    dependencies.split("\n").each do |dependency|
      library_filenames.each do |filename|
        if dependency.include? filename
          basename = File.basename(filename)
          #p filename 
          #p basename
          `install_name_tool -change #{filename} @rpath/#{basename} #{library}`
        end
      end
    end
  end
end

`macdeployqt ../MantidPlot.app`

`install_name_tool -change /usr/local/lib/Qt3Support.framework/Versions/4/Qt3Support @loader_path/../Frameworks/Qt3Support.framework/Versions/4/Qt3Support Contents/MacOS/mantidqtpython.so` 
`install_name_tool -change /usr/local/lib/QtOpenGL.framework/Versions/4/QtOpenGL @loader_path/../Frameworks/QtOpenGL.framework/Versions/4/QtOpenGL Contents/MacOS/mantidqtpython.so`
`install_name_tool -change /usr/local/lib/QtSvg.framework/Versions/4/QtSvg @loader_path/../Frameworks/QtSvg.framework/Versions/4/QtSvg Contents/MacOS/mantidqtpython.so`
`install_name_tool -change /usr/local/lib/QtGui.framework/Versions/4/QtGui @loader_path/../Frameworks/QtGui.framework/Versions/4/QtGui Contents/MacOS/mantidqtpython.so`
`install_name_tool -change /usr/local/lib/QtXml.framework/Versions/4/QtXml @loader_path/../Frameworks/QtXml.framework/Versions/4/QtXml Contents/MacOS/mantidqtpython.so`
`install_name_tool -change /usr/local/lib/QtSql.framework/Versions/4/QtSql @loader_path/../Frameworks/QtSql.framework/Versions/4/QtSql Contents/MacOS/mantidqtpython.so`
`install_name_tool -change /usr/local/lib/QtNetwork.framework/Versions/4/QtNetwork @loader_path/../Frameworks/QtNetwork.framework/Versions/4/QtNetwork Contents/MacOS/mantidqtpython.so`
`install_name_tool -change /usr/local/lib/QtCore.framework/Versions/4/QtCore @loader_path/../Frameworks/QtCore.framework/Versions/4/QtCore Contents/MacOS/mantidqtpython.so`
`install_name_tool -change /usr/local/lib/QtHelp.framework/Versions/4/QtHelp @loader_path/../Frameworks/QtHelp.framework/Versions/4/QtHelp  Contents/MacOS/mantidqtpython.so`
`install_name_tool -change /usr/local/lib/QtWebKit.framework/Versions/4/QtWebKit @loader_path/../Frameworks/QtWebKit.framework/Versions/4/QtWebKit Contents/MacOS/mantidqtpython.so`

`install_name_tool -change /usr/local/lib/QtOpenGL.framework/Versions/4/QtOpenGL @loader_path/../Frameworks/QtOpenGL.framework/Versions/4/QtOpenGL Contents/MacOS/libqwtplot3d.0.dylib`
`install_name_tool -change /usr/local/lib/QtGui.framework/Versions/4/QtGui @loader_path/../Frameworks/QtGui.framework/Versions/4/QtGui Contents/MacOS/libqwtplot3d.0.dylib`
`install_name_tool -change /usr/local/lib/QtCore.framework/Versions/4/QtCore @loader_path/../Frameworks/QtCore.framework/Versions/4/QtCore Contents/MacOS/libqwtplot3d.0.dylib`

`install_name_tool -change /usr/local/lib/QtGui.framework/Versions/4/QtGui @loader_path/../Frameworks/QtGui.framework/Versions/4/QtGui Contents/MacOS/libqwt.5.dylib`
`install_name_tool -change /usr/local/lib/QtCore.framework/Versions/4/QtCore @loader_path/../Frameworks/QtCore.framework/Versions/4/QtCore Contents/MacOS/libqwt.5.dylib`

`install_name_tool -change /usr/local/lib/QtGui.framework/Versions/4/QtGui @loader_path/../Frameworks/QtGui.framework/Versions/4/QtGui Contents/MacOS/libqscintilla2.11.dylib`
`install_name_tool -change /usr/local/lib/QtCore.framework/Versions/4/QtCore @loader_path/../Frameworks/QtCore.framework/Versions/4/QtCore Contents/MacOS/libqscintilla2.11.dylib`

`install_name_tool -change /usr/local/lib/QtCore.framework/Versions/4/QtCore @loader_path/../Frameworks/QtCore.framework/Versions/4/QtCore Contents/MacOS/libqscintilla2.11.dylib`

pyqt4_patterns = ["**/PyQt4/*.so"]
pyqt4_patterns.each do |pattern|
  Dir[pattern].each do |library|
    p "library: #{library}"
    basename =  File.basename(library)
    `chmod +w Contents/MacOS/PyQt4/#{basename}`  
    `install_name_tool -id @rpath/#{basename} Contents/MacOS/PyQt4/#{basename}`
  end
end

`install_name_tool -change /usr/local/lib/QtCore.framework/Versions/4/QtCore @loader_path/../../Frameworks/QtCore.framework/Versions/4/QtCore Contents/MacOS/PyQt4/QtCore.so`

`install_name_tool -change /usr/local/lib/QtGui.framework/Versions/4/QtGui @loader_path/../../Frameworks/QtGui.framework/Versions/4/QtGui Contents/MacOS/PyQt4/QtGui.so`
`install_name_tool -change /usr/local/lib/QtCore.framework/Versions/4/QtCore @loader_path/../../Frameworks/QtCore.framework/Versions/4/QtCore Contents/MacOS/PyQt4/QtGui.so`

`install_name_tool -change /usr/local/lib/QtOpenGL.framework/Versions/4/QtOpenGL @loader_path/../../Frameworks/QtOpenGL.framework/Versions/4/QtOpenGL Contents/MacOS/PyQt4/QtOpenGL.so`
`install_name_tool -change /usr/local/lib/QtGui.framework/Versions/4/QtGui @loader_path/../../Frameworks/QtGui.framework/Versions/4/QtGui Contents/MacOS/PyQt4/QtOpenGL.so`
`install_name_tool -change /usr/local/lib/QtCore.framework/Versions/4/QtCore @loader_path/../../Frameworks/QtCore.framework/Versions/4/QtCore Contents/MacOS/PyQt4/QtOpenGL.so`

`install_name_tool -change /usr/local/lib/QtSql.framework/Versions/4/QtSql @loader_path/../../Frameworks/QtSql.framework/Versions/4/QtSql Contents/MacOS/PyQt4/QtSql.so`
`install_name_tool -change /usr/local/lib/QtGui.framework/Versions/4/QtGui @loader_path/../../Frameworks/QtGui.framework/Versions/4/QtGui Contents/MacOS/PyQt4/QtSql.so`
`install_name_tool -change /usr/local/lib/QtCore.framework/Versions/4/QtCore @loader_path/../../Frameworks/QtCore.framework/Versions/4/QtCore Contents/MacOS/PyQt4/QtSql.so`

`install_name_tool -change /usr/local/lib/QtSvg.framework/Versions/4/QtSvg @loader_path/../../Frameworks/QtSvg.framework/Versions/4/QtSvg Contents/MacOS/PyQt4/QtSvg.so`
`install_name_tool -change /usr/local/lib/QtGui.framework/Versions/4/QtGui @loader_path/../../Frameworks/QtGui.framework/Versions/4/QtGui Contents/MacOS/PyQt4/QtSvg.so`
`install_name_tool -change /usr/local/lib/QtCore.framework/Versions/4/QtCore @loader_path/../../Frameworks/QtCore.framework/Versions/4/QtCore Contents/MacOS/PyQt4/QtSvg.so`

`install_name_tool -change /usr/local/lib/QtXml.framework/Versions/4/QtXml @loader_path/../../Frameworks/QtXml.framework/Versions/4/QtXml Contents/MacOS/PyQt4/QtXml.so`
`install_name_tool -change /usr/local/lib/QtCore.framework/Versions/4/QtCore @loader_path/../../Frameworks/QtCore.framework/Versions/4/QtCore Contents/MacOS/PyQt4/QtXml.so`

`cp -r /Library/Python/2.7/site-packages/sphinx Contents/MacOS/`
`cp -r /Library/Python/2.7/site-packages/sphinx_bootstrap_theme Contents/MacOS/`
`cp -r /Library/Python/2.7/site-packages/IPython Contents/MacOS/`
`cp -r /Library/Python/2.7/site-packages/zmq Contents/MacOS/`
`cp -r /Library/Python/2.7/site-packages/pygments Contents/MacOS/`

#add other dependencies found in current package
#currently missing epics

`cp /Library/Python/2.7/site-packages/gnureadline.so Contents/MacOS/`
`cp /Library/Python/2.7/site-packages/readline.py Contents/MacOS/`
`cp /Library/Python/2.7/site-packages/readline.pyc Contents/MacOS/`
`cp /Library/Python/2.7/site-packages/pyparsing.py Contents/MacOS/`
`cp /Library/Python/2.7/site-packages/pyparsing.pyc Contents/MacOS/`
`cp -r /Library/Python/2.7/site-packages/_markerlib/ Contents/MacOS/`
`cp -r /Library/Python/2.7/site-packages/backports Contents/MacOS/`
`cp -r /Library/Python/2.7/site-packages/certifi Contents/MacOS/`
`cp -r /Library/Python/2.7/site-packages/tornado Contents/MacOS/`
`cp -r /Library/Python/2.7/site-packages/markupsafe Contents/MacOS/`
`cp -r /Library/Python/2.7/site-packages/jinja2 Contents/MacOS/`
`cp -r /usr/local/lib/python2.7/site-packages/nxs Contents/MacOS/`
`rm Contents/MacOS/nxs/*.pyc`
`cp -r /Library/Python/2.7/site-packages/psutil Contents/MacOS/`
`mkdir Contents/MacOS/bin`
`cp -r /usr/local/bin/ipython Contents/MacOS/bin/`

search_patterns.each do |pattern|
  Dir[pattern].each do |library|
    dependencies = `otool -L #{library}`
    dependencies.split("\n").each do |dependency|
      if dependency.include? "/usr/local/"
         p "issue with library: #{library} linked against: #{dependency}"
      end
      if dependency.include? "libhdf5.9.dylib"
         p "checkout #{library} #{dependency.strip}"
      end
    end
  end
end

