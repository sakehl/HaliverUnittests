#include "Halide.h"
#include "HalideComplex.h"
#include <math.h>
#define HAVE_HALIVER
// #define CONCRETE_BOUNDS

// using dp3::ddecal;
using namespace Halide;

void set_bounds(std::vector<std::tuple<Expr, Expr>> dims, Halide::OutputImageParam p){
    Expr stride = 1;
    for(size_t i = 0; i < dims.size(); i++){
        p.dim(i).set_bounds(std::get<0>(dims[i]), std::get<1>(dims[i]));
        p.dim(i).set_stride(stride);
        stride *= std::get<1>(dims[i]);
    }
}

class HalideDiagionalSolver{
public:
    // Inputs
    ImageParam ant1;
    ImageParam ant2;
    ImageParam solution_map;
    ImageParam v_res_in;
    ImageParam model_;
    ImageParam sol_;
    ImageParam next_sol_;
    Param<int> solution_index0;
#ifdef CONCRETE_BOUNDS
    Expr n_dir_sol;
    Expr n_solutions;
    Expr n_vis;
    Expr n_antennas;
#else
    Param<int> n_dir_sol;
    Param<int> n_solutions;
    Param<int> n_vis;
    Param<int> n_antennas;
#endif
    Param<double> step_size;
    Param<bool> phase_only;
    
    Func model, sol, solutions, solD, next_sol;
    Var x, y, i, j, v, si, a, pol, c;

    Func antenna_1, antenna_2, solution_index, v_res0;
    Func sol_ann, sol_ann_;
    std::vector<Argument> args;

    int schedule;

    HalideDiagionalSolver() :
        ant1(type_of<int32_t>(), 1, "ant1"), // <1>[n_ant] uint32_t
        ant2(type_of<int32_t>(), 1, "ant2"), // <1>[n_ant] uint32_t
        solution_map(type_of<int32_t>(), 1, "solution_map"), // <1>[n_vis] uint32_t
        v_res_in(type_of<float>(), 4, "v_res_in"), // <4>[n_vis], Complex 2x2 Float (+3)
        
        model_(type_of<float>(), 4, "model_"), // <4>[n_vis], Complex 2x2 Float (+3)
        sol_(type_of<double>(), 4, "sol_"), // <4> [n_ant][n_dir_sol][2] Complex Double (+1)
        next_sol_(type_of<double>(), 4, "next_sol_"), // <4> [n_ant][n_dir_sol][2] Complex Double (+1)
        solution_index0("solution_index0"),
#ifndef CONCRETE_BOUNDS
        n_dir_sol("n_dir_sol"), 
        n_solutions("n_solutions"),
        n_vis("n_vis"),
        n_antennas("n_antennas"),
#endif
        step_size("step_size"), 
        phase_only("phase_only"),
        model("model"), sol("sol"), solutions("solutions"), solD("solD"), next_sol("next_sol"),
        x("x"), y("y"), i("i"), j("j"), v("v"), si("si"), a("a"), pol("pol"), c("c"),
        antenna_1("antenna_1"), antenna_2("antenna_2"), solution_index("solution_index"),
        v_res0("v_res0"), sol_ann("sol_ann"), sol_ann_("sol_ann_"){

        schedule = 2;

#ifdef CONCRETE_BOUNDS
        // solution_index0 = 0;
        n_dir_sol = 3;
        n_solutions = 8;
        n_vis = 230930;
        n_antennas = 50;
#endif
        
        set_bounds({{0, n_vis}}, ant1);
        set_bounds({{0, n_vis}}, ant2);
        set_bounds({{0, n_vis}}, solution_map);
        set_bounds({{0, 2}, {0, 2}, {0, 2}, {0, n_vis}}, v_res_in);
        set_bounds({{0, 2}, {0, 2}, {0, 2}, {0, n_vis}}, model_);
        set_bounds({{0, 2}, {0, 2}, {0, n_solutions}, {0, n_antennas}}, sol_);
        set_bounds({{0, 2}, {0, 2}, {0, n_solutions}, {0, n_antennas}}, next_sol_);
        
        
        model(v) = toComplexMatrix(model_, {v});
        sol(i, si, a) = castT<float>(Tuple(sol_(0, i, si, a), sol_(1, i, si, a)));
        solD(i, si, a) = Tuple(sol_(0, i, si, a), sol_(1, i, si, a));
        next_sol(i, si, a) = Tuple(next_sol_(0, i, si, a), next_sol_(1, i, si, a));

        antenna_1(v) = unsafe_promise_clamped(cast<int>(ant1(v)), 0, n_antennas-1);
        antenna_2(v) = unsafe_promise_clamped(cast<int>(ant2(v)), 0, n_antennas-1);
        solution_index(v) = unsafe_promise_clamped(cast<int>(solution_map(v)), solution_index0, solution_index0+n_dir_sol-1);

        sol_ann_(i,v,a) = sol(i,solution_index(v),a);
        sol_ann(v, a) = toDiagMatrix(sol_ann_, {v, a});
        solutions(si, a) = toDiagMatrix(sol, {si, a});

        v_res0(v) = toComplexMatrix(v_res_in, {v});
#ifdef CONCRETE_BOUNDS
        args = {sol_, solution_map, ant1, ant2, model_, v_res_in, solution_index0};
#else
        args = {sol_, solution_map, ant1, ant2, model_, v_res_in, solution_index0, n_dir_sol, n_vis, n_solutions, n_antennas};
#endif
    }

    Func toComplex(Func f){
        std::vector<Var> args(f.args().begin()+1, f.args().end());
        std::vector<Expr> args_real(f.args().begin()+1, f.args().end());
        args_real.insert(args_real.begin(), 0);

        std::vector<Expr> args_imag(f.args().begin()+1, f.args().end());
        args_imag.insert(args_imag.begin(), 1);

        Func out("toComplex");
        out(args) = Complex(f(args_real), f(args_imag));
        return out;
    }

    Func fromComplex(Func f){
        std::vector<Var> args = f.args();
        args.insert(args.begin(), i);
        
        Complex c = Complex(f(f.args()));

        Func out("fromComplex");
        out(args) = mux(i, c);
        return out;
    }
    
    template<typename T>
    std::vector<T> concat(std::vector<T> a, std::vector<T> b){
        std::vector<T> out;
        out.reserve(a.size() + b.size());
        out.insert(out.end(), a.begin(), a.end());
        out.insert(out.end(), b.begin(), b.end());
        return out;
    }
    
    // Makes a function for which 2 dimensions are matrix dimensions
    // towards a function of Matrix type.
    // E.g. f(i,j, ..args) 
    // -> f(..args) = Matrix(f(0,0, ..args), f(1,0, ..args),
    //                       f(1,0, ..args), f(1,1, ..args))
    Matrix toMatrix(Func f, std::vector<Expr> args){
        Complex c1, c2, c3, c4;
        c1 = f(concat({0,0}, args));
        c2 = f(concat({1,0}, args));
        c3 = f(concat({0,1}, args));
        c4 = f(concat({1,1}, args));

        return Matrix(c1, c2, c3, c4);
    }

    MatrixDiag toDiagMatrix(Func f, std::vector<Expr> args){
        Complex c00, c11;
        c00 = f(concat({0}, args));
        c11 = f(concat({1}, args));

        return MatrixDiag(c00, c11);
    }

    MatrixDiag toComplexDiagMatrix(Func f, std::vector<Expr> args){
        Complex c00, c11;
        c00 = Complex(f(concat({0,0}, args)), f(concat({1,0}, args)));
        c11 = Complex(f(concat({0,1}, args)), f(concat({1,1}, args)));

        return MatrixDiag(c00, c11);
    }

    Matrix toComplexMatrix(Func f, std::vector<Expr> args){
        Complex c1, c2, c3, c4;
        c1 = Complex(f(concat({0,0,0}, args)), f(concat({1,0,0}, args)));
        c2 = Complex(f(concat({0,1,0}, args)), f(concat({1,1,0}, args)));
        c3 = Complex(f(concat({0,0,1}, args)), f(concat({1,0,1}, args)));
        c4 = Complex(f(concat({0,1,1}, args)), f(concat({1,1,1}, args)));

        return Matrix(c1, c2, c3, c4);
    }

    Func matrixId(Func in){
        Func v_res0("v_res0");

        v_res0(v) = Matrix(
            Complex(in(0,0,0,v), in(1,0,0,v)),
            Complex(in(0,1,0,v), in(1,1,0,v)),
            Complex(in(0,0,1,v), in(1,0,1,v)),
            Complex(in(0,1,1,v), in(1,1,1,v))
        );
        
        Var vis_i, vis_o;
        Func out = matrixToDimensions(v_res0, {v});
        return out;
    }

    Func matrixToDimensions(Func in, std::vector<Var> args){
        Matrix m = Matrix(in(args));
        Func v_res_out("v_res_out");

        v_res_out(concat({c, i, j}, args)) = select(
            c == 0 && i == 0 && j == 0, m.m00.real,
            c == 1 && i == 0 && j == 0, m.m00.imag,
            c == 0 && i == 1 && j == 0, m.m01.real,
            c == 1 && i == 1 && j == 0, m.m01.imag,
            c == 0 && i == 0 && j == 1, m.m10.real,
            c == 1 && i == 0 && j == 1, m.m10.imag,
            c == 0 && i == 1 && j == 1, m.m11.real,
            m.m11.imag
        );
        v_res_out.bound(c, 0, 2).bound(i, 0, 2).bound(j, 0, 2);

        return v_res_out;
    }

    Func diagMatrixToDimensions(Func in, std::vector<Var> args){
        MatrixDiag m = MatrixDiag(in(args));
        Func v_res_out("v_res_out");

        v_res_out(concat({c,i}, args)) = select(
            c == 0 && i == 0, m.m00.real,
            c == 1 && i == 0, m.m00.imag,
            c == 0 && i == 1, m.m11.real,
                              m.m11.imag
        );
        v_res_out.bound(c, 0, 2).bound(i, 0, 2);

        return v_res_out;
    }

    Func TestNumerator(Func v_res_in_local){
        Func numerator("numerator"), denominator("denominator");
        numerator(si,a) = MatrixDiag({0.0f, 0.0f, 0.0f, 0.0f});
        denominator(i,si,a) = 0.0f;

        v_res_in_local.compute_root();
        RDom rv2(0, 2, 0, n_vis, "rv2");

        Matrix modelM = Matrix(model(v));

        MatrixDiag solution_1 = sol_ann(v ,antenna_2(v));
        Matrix cor_model_transp_1 = solution_1 * HermTranspose(modelM);
        
        MatrixDiag solution_2 = sol_ann(v ,antenna_1(v));
        Matrix cor_model_2 = solution_2 * modelM;

        Func denominator_inter("denominator_inter");
        Func ant_i("ant_i");
        ant_i(a, v) = select(a == 0, antenna_1(v), antenna_2(v));
        
        denominator_inter(a, i, v)
            = select( a==0 && i==0, cor_model_transp_1.m00.norm() + cor_model_transp_1.m10.norm()
                    , a==0 && i==1, cor_model_transp_1.m01.norm() + cor_model_transp_1.m11.norm()
                    , a==1 && i==0, cor_model_2.m00.norm() + cor_model_2.m10.norm()
                    , cor_model_2.m01.norm() + cor_model_2.m11.norm()
                );
        
        Expr sol_index = solution_index(rv2.y);
        denominator(i, sol_index, ant_i(rv2.x, rv2.y)) += denominator_inter(rv2.x, i, rv2.y);

        Func numerator_inter("numerator_inter");
        numerator_inter(a, v)
            = tuple_select(a==0, Diagonal(Matrix(v_res_in_local(v)) * cor_model_transp_1),
                     Diagonal(HermTranspose(Matrix(v_res_in_local(v))) * cor_model_2));
    
        numerator(sol_index, ant_i(rv2.x, rv2.y)) += numerator_inter(rv2.x, rv2.y);

        denominator.update().reorder(i, rv2.x, rv2.y).unroll(i).unroll(rv2.x);
        numerator.update().reorder(rv2.x, rv2.y).unroll(rv2.x);
        RVar rv2_y_i("rv2_y_i"), rv2_y_o("rv2_y_o");
        
        numerator.compute_root();
        denominator.compute_root();

        return diagMatrixToDimensions(numerator, {si,a});
    }

    Func AddOrSubtractDirection(bool add, Func vis_in){
        Func vis_out("vis_out");

        MatrixDiag solution_1 = solutions(solution_index(v), antenna_1(v));
        MatrixDiag solution_2 = solutions(solution_index(v), antenna_2(v));

        Matrix contribution = solution_1 * Matrix(model(v)) * HermTranspose(solution_2);

        if(add){
            vis_out(v) = Matrix(vis_in(v)) + contribution;
        } else {
            vis_out(v) = Matrix(vis_in(v)) - contribution;
        }

        return vis_out;
    }

    Func SolveDirection(Func vis_in){
        Func cor_model_transp_1("cor_model_transp_1"), cor_model_2("cor_model_2");
        Func ant_i("ant_i");
        Func denominator_inter("denominator_inter"), numerator_inter("numerator_inter");
        Func numerator("numerator"), denominator("denominator");
        Func next_solutions_inter("next_solutions_inter");
        Func next_solutions("next_solutions");

        Func vis_in_add = AddOrSubtractDirection(true, vis_in);
        
        MatrixDiag solution_1 = solutions(solution_index(v), antenna_2(v));
        MatrixDiag solution_2 = solutions(solution_index(v), antenna_1(v));
        cor_model_transp_1(v) = solution_1 * HermTranspose(Matrix(model(v)));
        cor_model_2(v) = solution_2 * Matrix(model(v));
        
        numerator_inter(a, v) = {undef<float>(), undef<float>(), undef<float>(), undef<float>()};
        numerator_inter(0, v) = Diagonal(Matrix(vis_in_add(v)) * Matrix(cor_model_transp_1(v)));
        numerator_inter(1, v) = Diagonal(HermTranspose(Matrix(vis_in_add(v))) * Matrix(cor_model_2(v)));

        denominator_inter(a, i, v) = undef<float>();
        denominator_inter(0, 0, v) = Matrix(cor_model_transp_1(v)).m00.norm() + Matrix(cor_model_transp_1(v)).m10.norm();
        denominator_inter(0, 1, v) = Matrix(cor_model_transp_1(v)).m01.norm() + Matrix(cor_model_transp_1(v)).m11.norm();
        denominator_inter(1, 0, v) = Matrix(cor_model_2(v)).m00.norm() + Matrix(cor_model_2(v)).m10.norm();
        denominator_inter(1, 1, v) = Matrix(cor_model_2(v)).m01.norm() + Matrix(cor_model_2(v)).m11.norm();

        ant_i(a, v) = select(a == 0, antenna_1(v), antenna_2(v));
        RDom rv2(0, 2, 0, n_vis, "rv2");
        numerator(si,a) = MatrixDiag({0.0f, 0.0f, 0.0f, 0.0f});
        numerator(solution_index(rv2.y), ant_i(rv2.x, rv2.y)) += numerator_inter(rv2.x, rv2.y);
        denominator(i,si,a) = 0.0f;
        denominator(i, solution_index(rv2.y), ant_i(rv2.x, rv2.y)) += denominator_inter(rv2.x, i, rv2.y);

        Expr nan = Expr(std::numeric_limits<double>::quiet_NaN());
        Complex cnan = Complex(nan, nan);

        // next_solutions_inter(pol ,si,a) = {undef<double>(), undef<double>()};
        next_solutions_inter(pol, si,a) = tuple_select(
            denominator(pol,si,a) == 0.0f, cnan,
            pol == 0, Tuple(castC<double>(MatrixDiag(numerator(si,a)).m00) / cast<double>(denominator(0,si,a))),
            castC<double>(MatrixDiag(numerator(si,a)).m11) / cast<double>(denominator(1,si,a))
        );
        next_solutions(c, pol, si, a) = mux(c, 
            {Complex(next_solutions_inter(pol, si, a)).real, 
             Complex(next_solutions_inter(pol, si, a)).imag});


        next_solutions.bound(c,0,2).bound(pol, 0, 2).bound(a, 0, n_antennas).bound(si, solution_index0, n_dir_sol);
        set_bounds({{0, 2}, {0, 2}}, next_solutions.output_buffer());
        next_solutions.output_buffer().dim(2).set_stride(2*2);
        next_solutions.output_buffer().dim(2).set_bounds(solution_index0, n_dir_sol);
        next_solutions.output_buffer().dim(3).set_stride(2*2*n_solutions);
        next_solutions.output_buffer().dim(3).set_bounds(0, n_antennas);
        if(schedule == 0) {
            numerator.compute_root();
            denominator.compute_root();
            cor_model_transp_1.compute_root();
            cor_model_2.compute_root();
        } else if(schedule == 1) {
            next_solutions.compute_root()
                .unroll(pol)
                .unroll(c)
                ;
            RVar r_out("r_out"), r_in("r_in");
            denominator.compute_root()
                .update()
                .reorder(i, rv2.x, rv2.y)
                .unroll(i)
                .unroll(rv2.x)
                ;
            numerator.compute_root()
                .update()
                .unroll(rv2.x)
                .compute_with(denominator.update(), rv2.y);
                ;
        } else if(schedule == 2) {
            Var a_out("a_out"), a_in("a_in");
            next_solutions.compute_root()
                .split(a, a_out, a_in, n_antennas/8, TailStrategy::GuardWithIf)
                .unroll(pol)
                .unroll(c)
                ;
            RVar r_out("r_out"), r_in("r_in");
            denominator.compute_root()
                // .split(a, a_out, a_in, n_antennas/8, TailStrategy::GuardWithIf)
                // .parallel(a_out)
                .unroll(i)
                .update()
                .reorder(i, rv2.x, rv2.y)
                .unroll(i)
                .unroll(rv2.x)
                .split(rv2.y, r_out, r_in, n_vis/8, TailStrategy::GuardWithIf)
                ;
            
            Var par("par");
            Func denom_inter("denom_inter");
            denom_inter = denominator.update().rfactor(r_out, par);
            denom_inter.compute_root()
                .parallel(par)
                .unroll(i)
                .update()
                .parallel(par)
                ;
            
            numerator.compute_root()
                // .split(a, a_out, a_in, n_antennas/8, TailStrategy::GuardWithIf)
                // .parallel(a_out)
                .update()
                .unroll(rv2.x)
                .split(rv2.y, r_out, r_in, n_vis/8, TailStrategy::GuardWithIf)
                ;

            Func num_inter("denom_inter");
            num_inter = numerator.update().rfactor(r_out, par);
            num_inter.compute_root()
                .parallel(par)
                .compute_with(denom_inter, si)
                .update()
                .parallel(par)
                .compute_with(denom_inter.update(), r_in);
                ;
            numerator.compute_with(denominator, si)
                .update()
                .compute_with(denominator.update(), r_out)
                ;
        }
            
        return next_solutions;
    }

    Func Step(){
        Func next_solutions("next_solutions");
        Func next_solutions_("next_solutions_0");
        Expr pi = Expr(M_PI);
        Func distance("distance");

        Expr phase_from = arg(solD(i, si, a));
        distance(i, si, a) = phase_from - arg(next_sol(i, si, a));
        distance(i, si, a) = select(distance(i, si, a) > pi, distance(i, si, a) - 2*pi, distance(i, si, a) + 2*pi);
        Complex phase_only_true = polar(Expr(1.0), phase_from + step_size * distance(i, si, a));
        Complex phase_only_false = Complex(solD(i, si, a)) * (Expr(1.0) - step_size) + Complex(next_sol(i, si, a)) * Complex(step_size);
        
        next_solutions_(i, si, a) = tuple_select(phase_only, 
           phase_only_true,
           phase_only_false);
        next_solutions(c, i, si, a) = mux(c, 
            {Complex(next_solutions_(i, si, a)).real, 
             Complex(next_solutions_(i, si, a)).imag});
        next_solutions.bound(c,0,2).bound(i, 0, 2).bound(a, 0, n_antennas).bound(si, 0, n_solutions);
        set_bounds({{0, 2}, {0, 2}, {0, n_solutions}, {0, n_antennas}}, next_solutions.output_buffer());
        
        next_solutions
            .specialize(phase_only)
            .unroll(c)
        ;

        return next_solutions;
    }

    void compile(bool non_unique){
        try{
            Target target = get_target_from_environment();
#ifndef HAVE_HALIVER
            target.set_feature(Target::CUDA);
            target.set_feature(Target::CLDoubles);
#endif
            
            target.set_feature(Target::AVX512);
            target.set_features({Target::NoAsserts, Target::NoBoundsQuery});
            // target.set_feature(Target::Debug);
            if(!host_supports_target_device(target)){ 
                std::cout << "The target " << target.to_string() << " is not supported on this host." << std::endl;
            } else {
                std::cout << "The target " << target.to_string() << " is supported on this host." << std::endl;
            }
#ifdef CONCRETE_BOUNDS
            std::vector<Argument> step_args = {phase_only, step_size, sol_, next_sol_};
#else
            std::vector<Argument> step_args = {n_vis, n_solutions, n_antennas, phase_only, step_size, sol_, next_sol_, n_dir_sol};
#endif
            
            // Bounds on input
#ifdef HAVE_HALIVER
            ant1.requires((ant1(_0) >= 0 && ant1(_0) < n_antennas));
            ant2.requires((ant2(_0) >= 0 && ant2(_0) < n_antennas));
            solution_map.requires((solution_map(_0) >= solution_index0 
                && solution_map(_0) < solution_index0+n_dir_sol));
            Annotation bounds = context_everywhere(solution_index0 >= 0 && n_dir_sol > 0 
                && solution_index0 + n_dir_sol < n_solutions
                && n_antennas>0 && n_vis>0 && n_solutions>0
                );
#ifdef CONCRETE_BOUNDS
            std::string cb = "CB";
#else
            std::string cb = "";
#endif
            std::string NU = non_unique ? "-non_unique" : "";
            
            Func solve_out = SolveDirection(v_res0);
            solve_out.compile_to_c("SolveDirectionHalide" + cb + NU + ".c", args, {bounds},
                 "SolveDirection" + cb + NU, target, false, non_unique);

            Func step_out = Step();
            Annotation step_bounds = context_everywhere(n_antennas>0 && n_vis>0 && n_solutions>0
                && n_antennas == 50 && n_solutions == 8 && n_vis == 230930 && n_dir_sol ==3);
            step_out.compile_to_c("StepHalide" + cb + NU + ".c", step_args, {step_bounds},
                 "StepHalide" + cb + NU, target, false, non_unique);
            
            Func v_sub_out = AddOrSubtractDirection(false, v_res0);
            Func v_sub_out_matrix = matrixToDimensions(v_sub_out, {v});
            v_sub_out_matrix.bound(v, 0, n_vis);
            set_bounds({{0, 2}, {0, 2}, {0, 2}, {0, n_vis}}, v_sub_out_matrix.output_buffer());
            v_sub_out_matrix.compile_to_c("SubDirectionHalide" + cb + NU + ".c", args, {bounds},
                "SubDirection"+ cb + NU, target, false, non_unique);

            // Func idFunc = matrixId(v_res_in);
            // set_bounds({{0, 2}, {0, 2}, {0, 2}, {0, n_vis}}, idFunc.output_buffer());
            // idFunc.compile_to_c("IdHalide.c", args, {bounds}, "IdHalide", target);

            // Func testNumerator = TestNumerator(v_res0);
            // set_bounds({{0, 2}, {0, 2}, {0, n_vis}, {0, n_antennas}}, testNumerator.output_buffer());
            // testNumerator.compile_to_c("TestNumerator.c", args, {bounds}, "TestNumerator", target);
#else
            idFunc.compile_to_c("IdHalide.cc", args, "IdHalide", target);
            testNumerator.compile_to_c("TestNumerator.cc", args, "TestNumerator", target);
            v_sub_out_matrix.compile_to_c("SubDirectionHalide.cc", args, "SubDirection", target);
            solve_out.compile_to_c("SolveDirectionHalide.cc", args, "SolveDirection", target);
            step_out.compile_to_c("StepHalide.cc", step_args, "StepHalide", target);
#endif

            // idFunc.compile_to_static_library("IdHalide", args, "IdHalide", target);
            // testNumerator.compile_to_static_library("TestNumerator", args, "TestNumerator", target);
            // v_sub_out_matrix.compile_to_static_library("SubDirectionHalide", args, "SubDirection", target);
            // solve_out.compile_to_static_library("SolveDirectionHalide", args, "SolveDirection", target);
            // step_out.compile_to_static_library("StepHalide", step_args, "StepHalide", target);

            // compile_standalone_runtime("HalideRuntime.o", target);
        } catch (Halide::Error &e){
            std::cerr << "Halide Error: " << e.what() << std::endl;
            __throw_exception_again;
        }
    }
};


int main(int argc, char **argv){

    HalideDiagionalSolver solver;
    solver.compile(false);

    HalideDiagionalSolver solver2;
    solver2.compile(true);
}