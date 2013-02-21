# - Find Feel

INCLUDE(ParseArguments)

macro(feelpp_add_application)

  PARSE_ARGUMENTS(FEELPP_APP
    "SRCS;LINK_LIBRARIES;CFG;GEO;MESH;LABEL;DEFS;DEPS;SCRIPTS;TEST"
    "NO_TEST;EXCLUDE_FROM_ALL;INCLUDE_IN_ALL;ADD_OT"
    ${ARGN}
    )
  CAR(FEELPP_APP_NAME ${FEELPP_APP_DEFAULT_ARGS})

  set(execname feelpp_${FEELPP_APP_NAME})

  if ( FEELPP_ENABLE_VERBOSE_CMAKE )
    MESSAGE("*** Arguments for Feel++ application ${FEELPP_APP_NAME}")
    MESSAGE("    Sources: ${FEELPP_APP_SRCS}")
    MESSAGE("    Link libraries: ${FEELPP_APP_LINK_LIBRARIES}")
    MESSAGE("       Cfg file: ${FEELPP_APP_CFG}")
    MESSAGE("      Deps file: ${FEELPP_APP_DEPS}")
    MESSAGE("      Defs file: ${FEELPP_APP_DEFS}")
    MESSAGE("       Geo file: ${FEELPP_APP_GEO}")
    MESSAGE("       Mesh file: ${FEELPP_APP_MESH}")
    MESSAGE("       Exec file: ${execname}")
    MESSAGE("exclude from all: ${FEELPP_APP_EXCLUDE_FROM_ALL}")
    MESSAGE("include from all: ${FEELPP_APP_INCLUDE_IN_ALL}")
  endif()


  if ( FEELPP_APP_EXCLUDE_FROM_ALL)
    add_executable(${execname}  EXCLUDE_FROM_ALL  ${FEELPP_APP_SRCS}  )
  elseif( FEELPP_APP_INCLUDE_IN_ALL)
    add_executable(${execname}  ${FEELPP_APP_SRCS}  )
  else()
    add_executable(${execname}  EXCLUDE_FROM_ALL  ${FEELPP_APP_SRCS}  )
  endif()
  if ( FEELPP_APP_DEPS )
    add_dependencies(${execname} ${FEELPP_APP_DEPS})
  endif()
  if ( FEELPP_APP_DEFS )
    set_property(TARGET ${execname} PROPERTY COMPILE_DEFINITIONS ${FEELPP_APP_DEFS})
  endif()
  target_link_libraries( ${execname} ${FEELPP_APP_LINK_LIBRARIES} ${FEELPP_LIBRARIES})
  #INSTALL(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/${execname}"  DESTINATION bin COMPONENT Bin)
  if ( NOT FEELPP_APP_NO_TEST )
	IF(NProcs2 GREATER 1)
    		add_test(NAME ${execname}-np-${NProcs2} COMMAND mpirun -np ${NProcs2} ${CMAKE_CURRENT_BINARY_DIR}/${execname} ${FEELPP_APP_TEST})
	ENDIF()
    add_test(NAME ${execname}-np-1 COMMAND mpirun -np 1 ${CMAKE_CURRENT_BINARY_DIR}/${execname} ${FEELPP_APP_TEST})
  endif()
  #add_dependencies(crb ${execname})
  # Add label if provided
  if ( FEELPP_APP_LABEL )
    set_property(TARGET ${execname} PROPERTY LABELS ${FEELPP_APP_LABEL})
    if ( NOT FEELPP_APP_NO_TEST )
	IF(NProcs2 GREATER 1)
      		set_property(TEST ${execname}-np-${NProcs2} PROPERTY LABELS ${FEELPP_APP_LABEL})
	ENDIF()
      set_property(TEST ${execname}-np-1 PROPERTY LABELS ${FEELPP_APP_LABEL})
    endif()
    if ( TARGET ${FEELPP_APP_LABEL} )
      add_dependencies( ${FEELPP_APP_LABEL} ${execname} )
    endif()
  endif()

  if (FEELPP_ENABLE_SLURM )
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${execname}.msub "#! /bin/bash
#MSUB -r ${execname}         # Request name
#MSUB -n 64                  # Number of tasks to use
#MSUB -T 1800                # Elapsed time limit in seconds of the job (default: 1800)
#MSUB -o ${execname}_%I.o    # Standard output. %I is the job id
#MSUB -e ${execname}_%I.e    # Error output. %I is the job id
#MSUB -A ra0840              # Project ID
#MSUB -q standard            # Choosing large nodes
##MSUB -@ noreply@cea.fr:end # Uncomment this line for being notified at the end of the job by sending a mail at the given address

#set -x
cd \${BRIDGE_MSUB_PWD}        # BRIDGE_MSUB_PWD is a environment variable which contains the directory where the script was submitted
unset LC_CTYPE
ccc_mprun ${execname}  # you can add Feel++ options here
")
  endif()

  if ( FEELPP_APP_CFG )
    foreach(  cfg ${FEELPP_APP_CFG} )
      #      if ( EXISTS ${cfg} )
      # extract cfg filename  to be copied in binary dir
      get_filename_component( CFG_NAME ${cfg} NAME )
      configure_file( ${cfg} ${CFG_NAME} )
      INSTALL(FILES "${cfg}"  DESTINATION share/feel/config)
      #      else()
      #        message(WARNING "Executable ${FEELPP_APP_NAME}: configuration file ${cfg} does not exist")
      #      endif()
    endforeach()
  endif(FEELPP_APP_CFG)

  if ( FEELPP_APP_GEO )
    foreach(  geo ${FEELPP_APP_GEO} )
      # extract geo filename  to be copied in binary dir
      get_filename_component( GEO_NAME ${geo} NAME )
      configure_file( ${geo} ${GEO_NAME} )
      INSTALL(FILES "${geo}"  DESTINATION share/feel/geo)
    endforeach()
  endif(FEELPP_APP_GEO)

  if ( FEELPP_APP_MESH )
    foreach(  mesh ${FEELPP_APP_MESH} )
      # extract mesh filename  to be copied in binary dir
      get_filename_component( MESH_NAME ${mesh} NAME )
      configure_file( ${mesh} ${MESH_NAME} )
      INSTALL(FILES "${mesh}"  DESTINATION share/feel/mesh)
    endforeach()
  endif(FEELPP_APP_MESH)

  if ( FEELPP_APP_SCRIPTS )
    foreach(  script ${FEELPP_APP_SCRIPTS} )
      # extract mesh filename  to be copied in binary dir
      get_filename_component( SCRIPT_NAME ${script} NAME )
      configure_file( ${script} ${SCRIPT_NAME} )
    endforeach()
  endif(FEELPP_APP_SCRIPTS)

  if ( FEELPP_APP_ADD_OT AND OPENTURNS_FOUND )
    set(pycpp "${execname}_pywrapper.cpp")
    set(xml "${execname}_ot.xml")
    set(FEELPP_APP_OT_WRAPPER_NAME "${execname}_ot")
    get_filename_component( FEELPP_APP_OUTPUT_WE ${FEELPP_APP_CFG} NAME_WE )
    set(FEELPP_APP_OUTPUT ${FEELPP_APP_OUTPUT_WE}.res )

    configure_file(${FEELPP_SOURCE_DIR}/cmake/templates/ot_python_command_wrapper.cpp ${pycpp})
    if ( NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${FEELPP_APP_OT_WRAPPER_NAME}.xml)
      configure_file(${FEELPP_SOURCE_DIR}/cmake/templates/ot_python.xml.in ${CMAKE_CURRENT_SOURCE_DIR}/${FEELPP_APP_OT_WRAPPER_NAME}.xml)
    endif()
    configure_file(${FEELPP_APP_OT_WRAPPER_NAME}.xml ${xml})
    feelpp_ot_add_python_module(${FEELPP_APP_OT_WRAPPER_NAME} ${pycpp}
      LINK_LIBRARIES ${OpenTURNS_LIBRARIES}
      CFG ${FEELPP_APP_CFG} XML ${xml} TEST)
  endif()
endmacro(feelpp_add_application)


macro(OVERWITE_IF_DIFFERENT thetarget filename var dummy)
  if ( NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/${filename} )
    # be careful if file does not exist we use dummy to generate the cpp file which will
    # then be overwritten using the cmake -E copy_if_different command
    configure_file(${dummy}  ${CMAKE_CURRENT_BINARY_DIR}/${filename})
  endif()
  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/copy_${filename} ${var})
  add_custom_command(TARGET ${thetarget} COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_BINARY_DIR}/copy_${filename} ${CMAKE_CURRENT_BINARY_DIR}/${filename}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endmacro()

macro(feelpp_add_test)
  PARSE_ARGUMENTS(FEELPP_TEST
    "SRCS;LINK_LIBRARIES;CFG;GEO;LABEL;DEFS;DEPS"
    "NO_TEST;EXCLUDE_FROM_ALL"
    ${ARGN}
    )
  CAR(FEELPP_TEST_NAME ${FEELPP_TEST_DEFAULT_ARGS})

  if ( NOT FEELPP_TEST_SRCS )
    set(targetname test_${FEELPP_TEST_NAME})
    set(filename test_${FEELPP_TEST_NAME}.cpp)
    add_executable(${targetname} ${filename})
    target_link_libraries(${targetname} ${FEELPP_LIBRARIES} ${FEELPP_TEST_LINK_LIBRARIES} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} )
    set_property(TARGET ${targetname} PROPERTY LABELS testsuite)
    if ( TARGET testsuite )
      add_dependencies(testsuite ${targetname})
    endif()

    add_test(
      NAME test_${FEELPP_TEST_NAME}
      COMMAND ${targetname} --log_level=message
      )

    set_property(TEST ${targetname} PROPERTY LABELS testsuite)

    if ( FEELPP_TEST_GEO )
      foreach(  geo ${FEELPP_TEST_GEO} )
        # extract geo filename  to be copied in binary dir
        get_filename_component( GEO_NAME ${geo} NAME )
        configure_file( ${geo} ${GEO_NAME} )
        configure_file( ${geo} $ENV{HOME}/feel/geo/${GEO_NAME})
      endforeach()
    endif(FEELPP_TEST_GEO)

  endif()


endmacro(feelpp_add_test)

#
# feelpp_add_python_module
#
macro(feelpp_ot_add_python_module)
if ( FEELPP_ENABLE_OPENTURNS AND OPENTURNS_FOUND )
  PARSE_ARGUMENTS(FEELPP_OT_PYTHON
    "LINK_LIBRARIES;SCRIPTS;XML;CFG"
    "TEST"
    ${ARGN}
    )
  CAR(FEELPP_OT_PYTHON_NAME ${FEELPP_OT_PYTHON_DEFAULT_ARGS})
  CDR(FEELPP_OT_PYTHON_SOURCES ${FEELPP_OT_PYTHON_DEFAULT_ARGS})

  add_library( ${FEELPP_OT_PYTHON_NAME} MODULE  ${FEELPP_OT_PYTHON_SOURCES}  )
  target_link_libraries( ${FEELPP_OT_PYTHON_NAME} ${FEELPP_OT_PYTHON_LINK_LIBRARIES}  )
  set_target_properties( ${FEELPP_OT_PYTHON_NAME} PROPERTIES PREFIX "" )
  set_property(TARGET ${FEELPP_OT_PYTHON_NAME} PROPERTY LABELS feelpp)
  #configure_file(${FEELPP_OT_PYTHON_NAME}.xml.in ${FEELPP_OT_PYTHON_NAME}.xml)

#  add_dependencies(feelpp ${FEELPP_OT_PYTHON_NAME})

  install(TARGETS ${FEELPP_OT_PYTHON_NAME} DESTINATION lib/openturns/wrappers/ COMPONENT Bin)
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${FEELPP_OT_PYTHON_NAME}.xml" DESTINATION lib/openturns/wrappers/ COMPONENT Bin)

  if ( FEELPP_OT_PYTHON_SCRIPTS )
    foreach(  script ${FEELPP_OT_PYTHON_SCRIPTS} )
      configure_file( ${script} ${script} )
      if ( FEELPP_OT_PYTHON_TEST )
        add_test(${script} ${PYTHON_EXECUTABLE} ${script})
        set_property(TEST ${script} PROPERTY LABELS feelpp)
      endif()
    endforeach()
  endif()
  if ( FEELPP_OT_PYTHON_CFG )
    foreach(  cfg ${FEELPP_OT_PYTHON_CFG} )
      configure_file( ${cfg} ${cfg} )
      INSTALL(FILES "${cfg}"  DESTINATION share/feel/config)
    endforeach()
  endif()
endif( FEELPP_ENABLE_OPENTURNS AND OPENTURNS_FOUND )
endmacro(feelpp_ot_add_python_module)
