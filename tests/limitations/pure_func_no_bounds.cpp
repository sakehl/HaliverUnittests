#include "Halide.h"
#include <stdio.h>
#include <vector>

using namespace Halide;

int main(int argc, char *argv[]) {

  Func f("f"), out("out");
  Var x("x"), y("y");
  Param<int> nx("nx"), ny("ny"), minx("minx"), miny("miny");

  out(x, y) = x + y;
  out.ensures(out(x,y) == x + y);

  out.output_buffer().dim(0).set_bounds(minx, nx);
  out.output_buffer().dim(1).set_bounds(miny, ny);
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
  out.translate_to_pvl(name +"_front.pvl", {}, pipeline_anns); 
  out.compile_to_c(name + ".c" , {nx, ny, minx, miny}, pipeline_anns, name, new_target, mem_only);
}