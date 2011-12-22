if(DOXYGEN_FOUND)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Mantid_template.doxyfile
                 ${CMAKE_CURRENT_BINARY_DIR}/Mantid_cmake.doxyfile @ONLY)
  add_custom_target( doxygen # ALL?
                     ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Mantid_cmake.doxyfile
		     WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		     COMMENT "Generating API documentation with Doxygen" VERBATIM
		   )
endif(DOXYGEN_FOUND)

##### diff Mantid.doxyfile Mantid.doxyfile_vs
# OUTPUT_DIRECTORY       = Doc           | OUTPUT_DIRECTORY       = ../Doc
# QT_AUTOBRIEF           = YES           | QT_AUTOBRIEF           = NO
# WARN_FORMAT  = "$file:$line: $text "   | WARN_FORMAT = "$file($line): $text"

##### diff Mantid.doxyfile Mantid.doxyfile_web
# OUTPUT_DIRECTORY       = Doc           | OUTPUT_DIRECTORY       = ../Doc
# STRIP_FROM_PATH        =               | STRIP_FROM_PATH        = /isis/mantid_temp/Code/Mantid
# QT_AUTOBRIEF           = YES           | QT_AUTOBRIEF           = NO
#                                        > DETAILS_AT_TOP         = NO
# BUILTIN_STL_SUPPORT    = NO            | BUILTIN_STL_SUPPORT    = YES
#                                        > OPTIMIZE_FOR_FORTRAN   = NO
#                                        > OPTIMIZE_OUTPUT_VHDL   = NO
# EXTRACT_ALL            = NO            | EXTRACT_ALL            = YES
#                                        > SORT_GROUP_NAMES       = NO
# SHOW_DIRECTORIES       = NO            | SHOW_DIRECTORIES       = YES
# ALPHABETICAL_INDEX     = NO            | ALPHABETICAL_INDEX     = YES
#                                        > # If the GENERATE_DOCSET tag is set to YES, additional index files 
#                                        > # will be generated that can be used as input for Apple's Xcode 3 
#                                        > # integrated development environment, introduced with OSX 10.5 (Leopard). 
#                                        > # To create a documentation set, doxygen will generate a Makefile in the 
#                                        > # HTML output directory. Running make will produce the docset in that 
#                                        > # directory and running "make install" will install the docset in 
#                                        > # ~/Library/Developer/Shared/Documentation/DocSets so that Xcode will find 
#                                        > # it at startup.
#                                        > GENERATE_DOCSET        = NO
#                                        > # When GENERATE_DOCSET tag is set to YES, this tag determines the name of the 
#                                        > # feed. A documentation feed provides an umbrella under which multiple 
#                                        > # documentation sets from a single provider (such as a company or product suite) 
#                                        > # can be grouped.
#                                        > DOCSET_FEEDNAME        = "Doxygen generated docs"
#                                        > # When GENERATE_DOCSET tag is set to YES, this tag specifies a string that 
#                                        > # should uniquely identify the documentation set bundle. This should be a 
#                                        > # reverse domain-name style string, e.g. com.mycompany.MyDocSet. Doxygen 
#                                        > # will append .docset to the name.
#                                        > DOCSET_BUNDLE_ID       = org.doxygen.Project
# MACRO_EXPANSION        = YES           | MACRO_EXPANSION        = NO
# HAVE_DOT               = NO            | HAVE_DOT               = YES
# COLLABORATION_GRAPH    = YES           | COLLABORATION_GRAPH    = NO
# UML_LOOK               = NO            | UML_LOOK               = YES
# TEMPLATE_RELATIONS     = NO            | TEMPLATE_RELATIONS     = YES
# SEARCHENGINE           = NO            | SEARCHENGINE           = YES

##### diff Mantid.doxyfile Mantid.jenkins.doxyfile
# OUTPUT_DIRECTORY       = Doc           | OUTPUT_DIRECTORY       = Code/Mantid/Framework/Doc
