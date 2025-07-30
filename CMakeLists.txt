cmake_minimum_required(VERSION 3.16)
project(HaliVerExperiments)

include(CTest)

set(CMAKE_CXX_STANDARD 11)  # or newer
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)
list(APPEND CMAKE_PREFIX_PATH ../../halide/install)

find_package(Halide REQUIRED)

set(OUTFILE halide.out)
set(VCT "/home/lars/data/vercors/bin/vct" CACHE STRING "Default value for VCT")
set(VERCORS ${VCT} --silicon-quiet --no-infer-heap-context-into-frame --dev-total-timeout=1200 --dev-assert-timeout 60)
set(VERCORS_PADRE ${VCT} --silicon-quiet --no-infer-heap-context-into-frame --dev-total-timeout=3600 --dev-assert-timeout 60)
# set(CMAKE_BINARY_DIR "/home/lars/data/HaliVerTests/Unittests/build" CACHE STRING "Default value for VCT")


function(build_unit_test)
  set(options SHOULDFAIL AND_FRONT ONLY_MEM)
  set(oneValueArgs TARGET DIR)
  set(multiValueArgs LABELS)
  cmake_parse_arguments(UT "${options}" "${oneValueArgs}"
                          "${multiValueArgs}" ${ARGN} )

  add_executable(${UT_TARGET} tests/unit/${UT_DIR}/${UT_TARGET}.cpp)

  target_link_libraries(${UT_TARGET} PRIVATE Halide::Halide)

  if(${UT_AND_FRONT})
    add_custom_command(
      OUTPUT ${UT_TARGET}_front.pvl ${UT_TARGET}.c
      COMMAND ./${UT_TARGET} ${UT_TARGET}
      DEPENDS ${UT_TARGET}
      VERBATIM
    )

    add_test(NAME ${UT_TARGET}_front.pvl
      COMMAND ${VERCORS} ${CMAKE_BINARY_DIR}/${UT_TARGET}_front.pvl
      )
    set_tests_properties(${UT_TARGET}_front.pvl PROPERTIES
      WILL_FAIL ${UT_SHOULDFAIL}
      LABELS unit:${UT_DIR}:front:${UT_LABELS}
    )
  else()
    add_custom_command(
      OUTPUT ${UT_TARGET}.c
      COMMAND ./${UT_TARGET} ${UT_TARGET}
      DEPENDS ${UT_TARGET}
      VERBATIM
    )
  endif()

  if(NOT ${UT_ONLY_MEM})
    add_test(NAME ${UT_TARGET}.c
      COMMAND ${VERCORS} ${CMAKE_BINARY_DIR}/${UT_TARGET}.c
    )
    set_tests_properties(${UT_TARGET}.c PROPERTIES
      WILL_FAIL ${UT_SHOULDFAIL}
      LABELS unit:${UT_DIR}:back:${UT_LABELS}
    )

  else()
    add_custom_command(
      OUTPUT ${UT_TARGET}_mem.c
      COMMAND ./${UT_TARGET} ${UT_TARGET} -mem_only
      DEPENDS ${UT_TARGET}
      VERBATIM
    )

    add_test(NAME ${UT_TARGET}_mem.c
      COMMAND ${VERCORS} ${CMAKE_BINARY_DIR}/${UT_TARGET}_mem.c
    )
    set_tests_properties(${UT_TARGET}_mem.c
      PROPERTIES
      WILL_FAIL ${UT_SHOULDFAIL}
      LABELS unit:${UT_DIR}:back:mem:${UT_LABELS}
    )

    add_custom_target(
      ${UT_TARGET}_all_mem ALL
      DEPENDS ${UT_TARGET}_mem.c
    )
  endif()

  if(NOT ${UT_SHOULDFAIL})
    
  endif()

  add_custom_target(
    ${UT_TARGET}_all ALL
    DEPENDS ${UT_TARGET}.c
  )
endfunction()

function(build_experiment_test)
  set(options NO_TEST)
  set(oneValueArgs TARGET DIR)
  set(multiValueArgs LABELS SCHEDULES)
  cmake_parse_arguments(UT "${options}" "${oneValueArgs}"
                          "${multiValueArgs}" ${ARGN} )

  
  if(NOT TARGET ${UT_TARGET})
    add_executable(${UT_TARGET} tests/${UT_DIR}/${UT_TARGET}.cpp)
  endif()

  target_link_libraries(${UT_TARGET} PRIVATE Halide::Halide)

  if(NOT UT_SCHEDULES)
    set(UT_SCHEDULES "0" "1" "2" "3")
  endif()

  if(NOT TARGET ${UT_TARGET}_pvl)
    add_custom_command(
      OUTPUT ${UT_TARGET}_front.pvl
      COMMAND ./${UT_TARGET} ${UT_TARGET}_front front
      DEPENDS ${UT_TARGET}
      VERBATIM
    )
    if(NOT ${UT_NO_TEST})
      add_test(NAME ${UT_TARGET}_front.pvl
        COMMAND ${VERCORS} ${CMAKE_BINARY_DIR}/${UT_TARGET}_front.pvl
      )
      set_tests_properties(${UT_TARGET}_front.pvl PROPERTIES
        LABELS experiment:${UT_DIR}:front:${UT_LABELS}
      )
    endif()

    add_custom_target(
      ${UT_TARGET}_pvl ALL
      DEPENDS ${UT_TARGET}_front.pvl
    )
  endif()

  foreach(V IN LISTS UT_SCHEDULES)
    add_custom_command(
      OUTPUT ${UT_TARGET}_${V}.c
      COMMAND ./${UT_TARGET} ${UT_TARGET}_${V} schedule ${V}
      DEPENDS ${UT_TARGET}
      VERBATIM
    )

    add_custom_command(
      OUTPUT ${UT_TARGET}_${V}_non_unique.c
      COMMAND ./${UT_TARGET} ${UT_TARGET}_${V}_non_unique schedule ${V} non_unique
      DEPENDS ${UT_TARGET}
      VERBATIM
    )

    if(NOT ${UT_NO_TEST})
      add_test(NAME ${UT_TARGET}_${V}.c
        COMMAND ${VERCORS} ${CMAKE_BINARY_DIR}/${UT_TARGET}_${V}.c
      )
      set_tests_properties(${UT_TARGET}_${V}.c PROPERTIES
        LABELS experiment:${UT_DIR}:back:${UT_LABELS}
      )
    endif()

  endforeach()

  foreach(V IN LISTS UT_SCHEDULES)
    add_custom_command(
      OUTPUT ${UT_TARGET}_${V}_mem.c
      COMMAND ./${UT_TARGET} ${UT_TARGET}_${V}_mem schedule ${V} mem
      DEPENDS ${UT_TARGET}
      VERBATIM
    )

    add_custom_command(
      OUTPUT ${UT_TARGET}_${V}_mem_non_unique.c
      COMMAND ./${UT_TARGET} ${UT_TARGET}_${V}_mem_non_unique schedule ${V} mem non_unique
      DEPENDS ${UT_TARGET}
      VERBATIM
    )

    if(NOT ${UT_NO_TEST})
      add_test(NAME ${UT_TARGET}_${V}_mem.c
        COMMAND ${VERCORS} ${CMAKE_BINARY_DIR}/${UT_TARGET}_${V}_mem.c
      )
      set_tests_properties(${UT_TARGET}_${V}_mem.c PROPERTIES
        LABELS experiment:${UT_DIR}:back:${UT_LABELS}:mem
      )
    endif()

    add_custom_target(
      ${UT_TARGET}_${V} ALL
      DEPENDS ${UT_TARGET}_${V}.c ${UT_TARGET}_${V}_mem.c
        ${UT_TARGET}_${V}_non_unique.c ${UT_TARGET}_${V}_mem_non_unique.c
    )

  endforeach()
endfunction()

function(build_single_experiment_test)
  set(options)
  set(oneValueArgs TARGET DIR)
  set(multiValueArgs LABELS)
  cmake_parse_arguments(UT "${options}" "${oneValueArgs}"
                          "${multiValueArgs}" ${ARGN} )

  add_executable(${UT_TARGET} tests/${UT_DIR}/${UT_TARGET}.cpp)

  target_link_libraries(${UT_TARGET} PRIVATE Halide::Halide)

  add_custom_command(
    OUTPUT ${UT_TARGET}_mem.c
    COMMAND ./${UT_TARGET} ${UT_TARGET}_mem
    DEPENDS ${UT_TARGET}
    VERBATIM
  )

  add_custom_target(
    ${UT_TARGET}_pvl ALL
    DEPENDS ${UT_TARGET}_mem.c
  )

  add_test(NAME ${UT_TARGET}_mem.c
    COMMAND ${VERCORS} ${CMAKE_BINARY_DIR}/${UT_TARGET}_mem.c
  )
  set_tests_properties(${UT_TARGET}_mem.c PROPERTIES
    LABELS ${UT_DIR}:back:${UT_LABELS}
  )
endfunction()

function(build_padre)
  set(options CONCRETE_BOUNDS)
  cmake_parse_arguments(UT "${options}" "${oneValueArgs}"
                          "${multiValueArgs}" ${ARGN} )

  set(CB "")
  if(${UT_CONCRETE_BOUNDS})
    set(CB "CB")
  endif()

  add_executable(GenerateHalideDiagonal${CB} tests/padre/GenerateHalideDiagonal.cpp)
  add_executable(GenerateHalideFull${CB} tests/padre/GenerateHalideFull.cpp)
  target_link_libraries(GenerateHalideDiagonal${CB} PRIVATE Halide::Halide)
  target_link_libraries(GenerateHalideFull${CB} PRIVATE Halide::Halide)
  if(${UT_CONCRETE_BOUNDS})
    target_compile_definitions(GenerateHalideDiagonal${CB} PUBLIC CONCRETE_BOUNDS)
    target_compile_definitions(GenerateHalideFull${CB} PUBLIC CONCRETE_BOUNDS)
  endif()
  
  add_custom_command(
    OUTPUT SubDirectionHalide${CB}.c SolveDirectionHalide${CB}.c StepHalide${CB}.c
    COMMAND ./GenerateHalideDiagonal${CB}
    DEPENDS GenerateHalideDiagonal${CB}
  )
  
  add_custom_target(
    GenerateHalideDiagonal${CB}_output ALL
    DEPENDS SubDirectionHalide${CB}.c SolveDirectionHalide${CB}.c StepHalide${CB}.c
  )
  
  add_custom_command(
    OUTPUT PerformIterationHalide${CB}.c
    COMMAND ./GenerateHalideFull${CB}
    DEPENDS GenerateHalideFull${CB}
  )
  
  add_custom_target(GenerateHalideFull${CB}_output ALL
    DEPENDS PerformIterationHalide${CB}.c
    GenerateHalideDiagonal${CB}_output
  )
  
  foreach(FILE SubDirectionHalide${CB}.c SolveDirectionHalide${CB}.c StepHalide${CB}.c PerformIterationHalide${CB}.c)
    add_test(NAME ${FILE}
      COMMAND ${VERCORS_PADRE} ${CMAKE_BINARY_DIR}/${FILE}
    )
    set_tests_properties(${FILE} PROPERTIES
      LABELS padre:back:${CB}
      TIMEOUT 3600
    )
  endforeach()
endfunction()
# Unit tests: simple Halide programs
build_unit_test(TARGET pure_func DIR alg AND_FRONT)
build_unit_test(TARGET update DIR alg AND_FRONT)
build_unit_test(TARGET update_full DIR alg AND_FRONT)
build_unit_test(TARGET update_full_fail DIR alg SHOULDFAIL AND_FRONT)
build_unit_test(TARGET reduction DIR alg AND_FRONT)
build_unit_test(TARGET reduction_in_arg DIR alg AND_FRONT)
build_unit_test(TARGET reduction_multi DIR alg AND_FRONT)
build_unit_test(TARGET reduction_multi_fail DIR alg SHOULDFAIL AND_FRONT)
build_unit_test(TARGET input_buffer DIR alg AND_FRONT)
# build_unit_test(TARGET floyd DIR alg)

build_unit_test(TARGET tuple DIR alg)
build_unit_test(TARGET tuple_pipeline DIR alg)
build_unit_test(TARGET multidim DIR alg)
build_unit_test(TARGET reduction_where DIR alg)
build_unit_test(TARGET reduction_rfactor DIR alg ONLY_MEM)
build_unit_test(TARGET specialize DIR alg)

# Floats / Rationals
build_unit_test(TARGET input_floats DIR alg AND_FRONT)

# Parallel schedules
build_unit_test(TARGET par_par DIR parallel)
build_unit_test(TARGET par_red DIR parallel)
build_unit_test(TARGET par_seq DIR parallel)
build_unit_test(TARGET seq_par DIR parallel)

# Unroll schedules
build_unit_test(TARGET unroll_outer DIR unroll)
build_unit_test(TARGET unroll_par DIR unroll)
build_unit_test(TARGET unroll_red DIR unroll)
build_unit_test(TARGET unroll_ser DIR unroll)

# Split schedules
build_unit_test(TARGET split_exact DIR split)
build_unit_test(TARGET split_if DIR split)
build_unit_test(TARGET split_round DIR split)
# build_unit_test(TARGET split_shift DIR split)

# Fuse schedules
build_unit_test(TARGET fuse_par DIR fuse)
build_unit_test(TARGET fuse_ser DIR fuse)

# Compute_at
build_unit_test(TARGET compute_inline DIR compute_at)
build_unit_test(TARGET compute_half DIR compute_at)
build_unit_test(TARGET compute_root DIR compute_at)
build_unit_test(TARGET compute_update DIR compute_at)

# Compute_with
build_unit_test(TARGET compute_with_half DIR compute_with)
build_unit_test(TARGET compute_with_inline DIR compute_with)
build_unit_test(TARGET compute_with_reduction DIR compute_with AND_FRONT)

# Store_at
build_unit_test(TARGET store_half DIR store_at)
build_unit_test(TARGET store_root DIR store_at)
build_unit_test(TARGET store_update DIR store_at)

# Reorder
build_unit_test(TARGET reorder_par_red DIR reorder)
build_unit_test(TARGET reorder_par_red_2 DIR reorder)
build_unit_test(TARGET reorder_par_red_3 DIR reorder)
build_unit_test(TARGET reorder_par_ser DIR reorder)
build_unit_test(TARGET reorder_seq_par DIR reorder)
build_unit_test(TARGET reorder_seq_red DIR reorder)
build_unit_test(TARGET reorder DIR reorder)

# Known limitations
build_unit_test(TARGET pure_func_no_bounds DIR limitations ONLY_MEM)
build_unit_test(TARGET pure_func_no_bounds_xyz DIR limitations ONLY_MEM)
build_unit_test(TARGET pure_func_no_bounds_yzx DIR limitations ONLY_MEM NO_TEST)

# Experiments: involved Halide programs
build_experiment_test(TARGET blur DIR experiment)
build_experiment_test(TARGET hist DIR experiment)
build_experiment_test(TARGET conv_layer DIR experiment)
build_experiment_test(TARGET auto_viz DIR experiment SCHEDULES 1 2)
build_experiment_test(TARGET auto_viz DIR experiment SCHEDULES 0 3 NO_TEST)

build_experiment_test(TARGET gemm DIR experiment SCHEDULES 0 1 2 )
build_experiment_test(TARGET gemm DIR experiment SCHEDULES 3 NO_TEST)

# build_single_experiment_test(TARGET bgu DIR bgu)
build_single_experiment_test(TARGET bilateral_grid DIR experiment)
build_single_experiment_test(TARGET camera_pipe DIR experiment)
build_single_experiment_test(TARGET depthwise_separable_conv DIR experiment)

## Build padre files
build_padre()
build_padre(CONCRETE_BOUNDS)

# Tutorial
function(build_lesson)
  set(options)
  set(oneValueArgs TARGET DIR)
  set(multiValueArgs)
  cmake_parse_arguments(UT "${options}" "${oneValueArgs}"
                          "${multiValueArgs}" ${ARGN} )

  add_executable(${UT_TARGET} ${UT_DIR}/${UT_TARGET}.cpp)

  target_link_libraries(${UT_TARGET} PRIVATE Halide::Halide)

  add_custom_command(
    OUTPUT ${UT_TARGET}_all_files
    COMMAND ./${UT_TARGET}
    DEPENDS ${UT_TARGET}
    VERBATIM
  )

  add_custom_target(
    ${UT_TARGET}_all ALL
    DEPENDS ${UT_TARGET}_all_files
  )
endfunction()


build_lesson(TARGET lesson1 DIR tutorial)
build_lesson(TARGET lesson2 DIR tutorial)
build_lesson(TARGET lesson3 DIR tutorial)


# add_executable(haliver tests/haliver.cpp)

# target_link_libraries(haliver PRIVATE Halide::Halide)

# add_custom_command(
#   OUTPUT haliver.c
#   COMMAND ./haliver
#   DEPENDS haliver
#   VERBATIM
# )

# add_custom_target(
#   haliver_examples ALL
#   DEPENDS haliver.c
# )