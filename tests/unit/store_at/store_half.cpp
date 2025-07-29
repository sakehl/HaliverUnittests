#include "Halide.h"
#include <stdio.h>
#include <vector>

using namespace Halide;

int main(int argc, char *argv[]) {
  Func f("f"), out("out");
  Var x("x"), y("y");
  f(x, y) = x + y;
  f.ensures(f(x,y) == x+y);

  out(x, y) = f(x,y) + 1;
  out.ensures(out(x,y) == x+y + 1);
  out.ensures(out(x,y) > 0);

  f.store_at(out, y);
  f.compute_at(out, x);

  Var xo("xo"), xi("xi");

  int nx = 100, ny = 42;

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
  out.compile_to_c(name + "_non_unique"+ ".c" , {}, pipeline_anns, name, new_target, mem_only, false);
  out.compile_to_c(name + ".c" , {}, pipeline_anns, name, new_target, mem_only, true);
}