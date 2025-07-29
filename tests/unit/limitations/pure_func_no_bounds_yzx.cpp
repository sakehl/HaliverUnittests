#include "Halide.h"
#include <stdio.h>
#include <vector>

using namespace Halide;

int main(int argc, char *argv[]) {

  Func f("f"), out("out");
  Var x("x"), y("y"), z("z");
  Param<int> nx("nx"), ny("ny"), nz("nz"), minx("minx"), miny("miny"), minz("minz");

  out(x, y, z) = x + y + z;
  out.ensures(out(x,y,z) == x + y + z);

  out.output_buffer().dim(0).set_bounds(0, nx);
  out.output_buffer().dim(1).set_bounds(0, ny);
  out.output_buffer().dim(2).set_bounds(0, nz);
  out.output_buffer().dim(1).set_stride(nx);
  out.output_buffer().dim(2).set_stride(nx*ny);

  Target target = Target();
  Target new_target = target
    .with_feature(Target::NoAsserts)
    .with_feature(Target::NoBoundsQuery)
    ;

  out.reorder(y, z, x);
  
  std::vector<Annotation> pipeline_anns = {
    context(nx > 0),
    context(ny > 0),
    context(nz > 0)
  };

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
  out.compile_to_c(name + "_non_unique"+ ".c" , {nx, ny, nz, minx, miny, minz}, pipeline_anns, name, new_target, mem_only, false);
  out.compile_to_c(name + ".c" , {nx, ny, nz, minx, miny, minz}, pipeline_anns, name, new_target, mem_only, true);
}