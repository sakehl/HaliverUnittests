cmake_minimum_required(VERSION 3.16)
project(HaliVerExperiments)

include(CTest)

set(CMAKE_CXX_STANDARD 11)  # or newer
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)
set(CMAKE_PREFIX_PATH ../../halide/install)

find_package(Halide REQUIRED)

set(OUTFILE halide.out)
set(VERCORS /home/lars/data/vercors/bin/vct --silicon-quiet --no-infer-heap-context-into-frame --dev-total-timeout=600)


function(build_unit_test)
  set(options SHOULDFAIL AND_FRONT)
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
      COMMAND ${VERCORS} ${UT_TARGET}_front.pvl
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

  add_test(NAME ${UT_TARGET}.c
    COMMAND ${VERCORS} ${UT_TARGET}.c
  )
  set_tests_properties(${UT_TARGET}.c PROPERTIES
    WILL_FAIL ${UT_SHOULDFAIL}
    LABELS unit:${UT_DIR}:back:${UT_LABELS}
  )

  if(NOT ${UT_SHOULDFAIL})
    add_custom_command(
      OUTPUT ${UT_TARGET}_mem.c
      COMMAND ./${UT_TARGET} ${UT_TARGET} -mem_only
      DEPENDS ${UT_TARGET}
      VERBATIM
    )

    add_test(NAME ${UT_TARGET}_mem.c
      COMMAND ${VERCORS} ${UT_TARGET}_mem.c
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

  add_custom_target(
    ${UT_TARGET}_all ALL
    DEPENDS ${UT_TARGET}.c
  )
endfunction()

function(build_experiment_test)
  set(options)
  set(oneValueArgs TARGET DIR)
  set(multiValueArgs LABELS)
  cmake_parse_arguments(UT "${options}" "${oneValueArgs}"
                          "${multiValueArgs}" ${ARGN} )

  add_executable(${UT_TARGET} tests/${UT_DIR}/${UT_TARGET}.cpp)

  target_link_libraries(${UT_TARGET} PRIVATE Halide::Halide)

  add_custom_command(
    OUTPUT ${UT_TARGET}_front.pvl
    COMMAND ./${UT_TARGET} ${UT_TARGET}_front front
    DEPENDS ${UT_TARGET}
    VERBATIM
  )

  add_test(NAME ${UT_TARGET}_front.pvl
    COMMAND ${VERCORS} ${UT_TARGET}_front.pvl
  )
  set_tests_properties(${UT_TARGET}_front.pvl PROPERTIES
    LABELS experiment:${UT_DIR}:front:${UT_LABELS}
  )

  foreach(V RANGE 3)
    add_custom_command(
      OUTPUT ${UT_TARGET}_${V}.c
      COMMAND ./${UT_TARGET} ${UT_TARGET}_${V} ${V}
      DEPENDS ${UT_TARGET}
      VERBATIM
    )

    add_test(NAME ${UT_TARGET}_${V}.c
      COMMAND ${VERCORS} ${UT_TARGET}_${V}.c
    )
    set_tests_properties(${UT_TARGET}_${V}.c PROPERTIES
      LABELS experiment:${UT_DIR}:back:${UT_LABELS}
    )

  endforeach()

  foreach(V RANGE 3)
    add_custom_command(
      OUTPUT ${UT_TARGET}_${V}_mem.c
      COMMAND ./${UT_TARGET} ${UT_TARGET}_${V}_mem ${V} mem
      DEPENDS ${UT_TARGET}
      VERBATIM
    )

    add_test(NAME ${UT_TARGET}_${V}_mem.c
      COMMAND ${VERCORS} ${UT_TARGET}_${V}_mem.c
    )
    set_tests_properties(${UT_TARGET}_${V}_mem.c PROPERTIES
      LABELS experiment:${UT_DIR}:back:${UT_LABELS}:mem
    )

  endforeach()


  add_custom_target(
    ${UT_TARGET}_pvl ALL
    DEPENDS ${UT_TARGET}_front.pvl ${UT_TARGET}_0.c
    ${UT_TARGET}_1.c ${UT_TARGET}_2.c ${UT_TARGET}_3.c
  )

  add_custom_target(
    ${UT_TARGET}_pvl_mem ALL
    DEPENDS ${UT_TARGET}_0_mem.c
    ${UT_TARGET}_1_mem.c ${UT_TARGET}_2_mem.c ${UT_TARGET}_3_mem.c
  )
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
    COMMAND ${VERCORS} ${UT_TARGET}_mem.c
  )
  set_tests_properties(${UT_TARGET}_mem.c PROPERTIES
    LABELS ${UT_DIR}:back:${UT_LABELS}
  )
endfunction()

# Unit tests: simple Halide programs
build_unit_test(TARGET pure_func DIR alg AND_FRONT)
build_unit_test(TARGET update DIR alg AND_FRONT)
build_unit_test(TARGET update_full DIR alg AND_FRONT)
build_unit_test(TARGET update_full_fail DIR alg SHOULDFAIL AND_FRONT)
build_unit_test(TARGET reduction DIR alg AND_FRONT)
build_unit_test(TARGET reduction_multi DIR alg AND_FRONT)
build_unit_test(TARGET reduction_multi_fail DIR alg SHOULDFAIL AND_FRONT)
build_unit_test(TARGET input_buffer DIR alg AND_FRONT)

build_unit_test(TARGET tuple DIR alg)
build_unit_test(TARGET tuple_pipeline DIR alg)
build_unit_test(TARGET multidim DIR alg)
build_unit_test(TARGET reduction_where DIR alg)

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
build_unit_test(TARGET compute_with_reduction DIR compute_with)

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
build_unit_test(TARGET pure_func_no_bounds DIR limitations)
build_unit_test(TARGET pure_func_no_bounds_xyz DIR limitations)
build_unit_test(TARGET pure_func_no_bounds_yzx DIR limitations)

# Experiments: involved Halide programs
build_experiment_test(TARGET blur DIR experiment)
build_experiment_test(TARGET hist DIR experiment)
build_experiment_test(TARGET conv_layer DIR experiment)
build_experiment_test(TARGET gemm DIR experiment)
build_experiment_test(TARGET auto_viz DIR experiment)

# build_single_experiment_test(TARGET bgu DIR bgu)
build_single_experiment_test(TARGET bilateral_grid DIR experiment)
build_single_experiment_test(TARGET camera_pipe DIR experiment)
build_single_experiment_test(TARGET depthwise_separable_conv DIR experiment)

## Build padre files
add_executable(GenerateHalideDiagonal tests/padre/GenerateHalideDiagonal.cc)
add_executable(GenerateHalideFull tests/padre/GenerateHalideFull.cc)
target_link_libraries(GenerateHalideDiagonal PRIVATE Halide::Halide)
target_link_libraries(GenerateHalideFull PRIVATE Halide::Halide)

add_custom_command(
  OUTPUT SubDirectionHalide.c SolveDirectionHalide.c StepHalide.c
  COMMAND ./GenerateHalideDiagonal
  DEPENDS GenerateHalideDiagonal
)

add_custom_target(
  GenerateHalideDiagonal_output ALL
  DEPENDS SubDirectionHalide.c SolveDirectionHalide.c StepHalide.c
)

add_custom_command(
  OUTPUT PerformIterationHalide.c
  COMMAND ./GenerateHalideFull
  DEPENDS GenerateHalideFull
)

add_custom_target(GenerateHalideFull_output ALL
  DEPENDS PerformIterationHalide.c
  GenerateHalideDiagonal_output
)

foreach(FILE SubDirectionHalide.c SolveDirectionHalide.c StepHalide.c PerformIterationHalide.c)
  add_test(NAME ${FILE}
    COMMAND ${VERCORS} ${FILE}
  )
  set_tests_properties(${FILE} PROPERTIES
    LABELS padre:back
  )
endforeach()