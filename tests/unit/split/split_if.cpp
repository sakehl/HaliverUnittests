#include "Halide.h"
#include <stdio.h>
#include <vector>

using namespace Halide;

int main(int argc, char *argv[]) {
  Func f("f"), out("out");
  Var x("x"), y("y");
  ImageParam input(type_of<int>(), 2, "input");
  input.requires(input(_0, _1) >= 0);

  out(x, y) = input(y, x) + 1;
  out.ensures(out(x,y) == input(y, x) + 1);
  out.ensures(out(x,y) > 0);

  Var xo("xo"), xi("xi");
  out.split(x, xo, xi, 7, TailStrategy::GuardWithIf);

  int nx = 100, ny = 42;
  input.dim(0).set_bounds(0, ny);
  input.dim(1).set_bounds(0, nx);
  input.dim(1).set_stride(ny);

  out.output_buffer().dim(0).set_bounds(0,nx);
  out.output_buffer().dim(1).set_bounds(0,ny);
  out.output_buffer().dim(1).set_stride(nx);
  
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
  out.compile_to_c(name + "_non_unique"+ ".c" , {input}, pipeline_anns, name, new_target, mem_only, false);
  out.compile_to_c(name + ".c" , {input}, pipeline_anns, name, new_target, mem_only, true);
}