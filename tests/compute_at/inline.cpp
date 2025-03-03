#include "Halide.h"
#include <stdio.h>
#include <vector>

using namespace Halide;

int main(int argc, char *argv[]) {
  // TODO: It seems that this file can produce code for the back-end, that can sometimes `hang`
  // We should investigate this
  Func f("f"), out("out"), input_temp("input_temp"), input("input");
  Var x("x"), y("y"), z("z");
  Param<int> nx("nx"), ny("ny");
  input(x, y, z) = x+y+z;
  input.compute_root();

  input_temp(x, y) = Tuple(input(0, x, y), input(1, x, y));

  out(x, y) = input_temp(x, y)[0] + input_temp(x, y)[1];

  // input.dim(0).set_bounds(0, 2);
  // input.dim(1).set_bounds(0, nx);
  // input.dim(1).set_stride(2);
  // input.dim(2).set_bounds(0, ny);
  // input.dim(2).set_stride(nx*2);

  out.output_buffer().dim(0).set_bounds(0,nx);
  out.output_buffer().dim(1).set_bounds(0,ny);
  out.output_buffer().dim(1).set_stride(nx);
  Var xo("xo"), xi("xi");
  out.split(x, xi, xo, 2, TailStrategy::GuardWithIf);
  
  Target target = Target();
  Target new_target = target
    .with_feature(Target::NoAsserts)
    .with_feature(Target::NoBoundsQuery)
    ;
  
  std::vector<Annotation> pipeline_anns;

  std::string name = argv[1];
  std::string mem_only_s = "";
  if(argc >= 3){
    mem_only_s = argv[2];
  }
  bool mem_only = mem_only_s == "-mem_only";
  if(mem_only){
    name += "_mem";
  }
  out.compile_to_c(name + ".c" , {nx, ny}, pipeline_anns, name, new_target, mem_only);
}