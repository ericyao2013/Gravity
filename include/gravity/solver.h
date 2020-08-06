//
//  solver.h
//  Gravity
//
//  Created by Hassan

#ifndef __Gravity____Solver__
#define __Gravity____Solver__

#include <stdio.h>

#include <gravity/GravityConfig.h>

#include <gravity/model.h>
#ifdef USE_MPI
#include <mpi.h>
#endif
#ifdef USE_IPOPT
#include <gravity/IpoptProgram.h>
//#include "IpoptInterfaceCommon.h"
#include <coin/IpRegOptions.hpp>
#include <coin/IpJournalist.hpp>
#include <coin/IpIpoptApplication.hpp>
#include <coin/IpSolveStatistics.hpp>
#include <future>
#include <thread>


using Ipopt::IsValid;
using Ipopt::RegisteredOption;
using Ipopt::EJournalLevel;
using Ipopt::Journal;
using Ipopt::IpoptApplication;
using Ipopt::SmartPtr;
using Ipopt::TNLP;
using Ipopt::ApplicationReturnStatus;
using Ipopt::SolveStatistics;
#endif
#ifdef USE_GUROBI
#include <gravity/GurobiProgram.h>
#endif
#ifdef USE_BONMIN
#include <gravity/BonminProgram.h>
#endif
#ifdef USE_CPLEX
#include <gravity/CplexProgram.h>
#endif
#ifdef USE_CLP
#include <gravity/ClpProgram.h>
#endif
//#ifdef USE_SDPA
//#include "SdpaProgram.h"
//#endif
#ifdef USE_MOSEK
#include "MosekProgram.h"
#endif


void gurobiNotAvailable();

void cplexNotAvailable();

void bonminNotAvailable();

void ipoptNotAvailable();

void mosekNotAvailable();

void ClpNotAvailable();

namespace gravity {
    
    
    template<typename type = double>
    class solver {
    public:
        gravity::Model<type>*                     _model = nullptr;
        shared_ptr<Program<type>>                 _prog = nullptr;
        SolverType                                _stype;
        type                                      _tol = 1e-6; /*<< Solver tolerance. */
        unsigned                                  _nb_iterations = 0;
        map<string,string>                        _str_options;
        map<string,int>                           _int_options;
        map<string,double>                        _double_options;
        map<string,bool>                          _bool_options;
        
        /** Constructor */
        //@{
        ~solver(){
            if(_stype==gurobi)
            {
#ifdef USE_GUROBI
                auto grb_prog = static_pointer_cast<GurobiProgram>(_prog);
                delete grb_prog->grb_mod;
                delete grb_prog->grb_env;
                grb_prog->grb_mod = nullptr;
                grb_prog->grb_env = nullptr;
#else
                gurobiNotAvailable();
#endif
            }
        }
        solver();
        
        solver(shared_ptr<gravity::Model<type>> model, SolverType stype){
            _stype = stype;
            _model = (Model<type>*)(&(*model));
            _model->_built = true;
            init();
        }
        
        solver(const Model<type>& model, SolverType stype){
            _stype = stype;
            _model = (Model<type>*)&model;
            _model->_built = true;
            init();
        }
        void set_option(const string& option, const string& val){
            _str_options[option] = val;
        }
        
        void set_option(const string& option, const int& val){
            _int_options[option] = val;
        }
        
        void set_option(const string& option, const double& val){
            _double_options[option] = val;
        }
        
        void set_option(const string& option, const bool& val){
            _bool_options[option] = val;
        }
        
        unsigned get_nb_iterations(){
            return _nb_iterations;
        }
        
        void init(){
            if (_stype==ipopt) {
#ifdef USE_IPOPT
                _model->replace_integers();
                SmartPtr<IpoptApplication> iapp = IpoptApplicationFactory();
                iapp->RethrowNonIpoptException(true);
                ApplicationReturnStatus status = iapp->Initialize();
                
                if (status != Solve_Succeeded) {
                    throw invalid_argument("*** Error during initialization!\n");
                }
                
                if(_model->_objt==maximize){
                    *_model->_obj *= -1;
                }
                _prog = make_shared<IpoptProgram<type>>(_model);
                _bool_options["check_violation"] = false;
#else
                ipoptNotAvailable();
#endif
            }
            else if(_stype==gurobi)
            {
#ifdef USE_GUROBI
                _prog = make_shared<GurobiProgram>(_model);
                _bool_options["gurobi_crossover"] = false;
#else
                gurobiNotAvailable();
#endif
            }
            else if(_stype==cplex)
            {
#ifdef USE_CPLEX
                _prog = make_shared<CplexProgram>(_model);
#else
                cplexNotAvailable();
#endif
            }
            else if(_stype == _mosek)
            {
#ifdef USE_MOSEK
                _prog = make_shared<MosekProgram>(_model);
#else
                mosekNotAvailable();
#endif
            }
            else if(_stype==bonmin) {
#ifdef USE_BONMIN
                _model->replace_integers();
                if(_model->_objt==maximize){
                    *_model->_obj *= -1;
                }
                _prog = make_shared<BonminProgram>(_model);
#else
                bonminNotAvailable();
#endif
            }
            else if (_stype == clp){
#ifdef USE_CLP
                _model->replace_integers();
                _prog = make_shared<ClpProgram>(_model);
#else
                ClpNotAvailable();
#endif
            }
        }
        //@}
        void set_model(gravity::Model<type>& m);
        int run(bool relax){
            return run(5, 1e-6, 10000, 1e-6, relax, {false,""}, 1e+6);
        }
        int run(int output, type tol , const string& lin_solver){
            return run(output, tol, 10000, 1e-6, true, {lin_solver!="",lin_solver}, 1e+6);
        }
        
        int run(type tol , double time_limit){
            return run(5, tol, 10000, 1e-6, true, {false,""}, time_limit);
        }
        
        //OPF.run(tol,time_limit,"ma97");
        int run(type tol , double time_limit, const string& lin_solver){
            return run(5, tol, 10000, 1e-6, true, {lin_solver!="",lin_solver}, time_limit);
        }
        
        int run(int output, type tol , const string& lin_solver, int max_iter){
            return run(output, tol, max_iter, 1e-6, true, {lin_solver!="",lin_solver}, 1e6);
        }
        
        int run(int output, type tol , double time_limit, const string& lin_solver, int max_iter){
            return run(output, tol, max_iter, 1e-6, true, {lin_solver!="",lin_solver}, time_limit);
        }
        
        int run(int output=5, type tol=1e-6 , int max_iter=2000){
            return run(output, tol, max_iter, 1e-6, false, {false,""}, 1e+6);
        }
        /* run model */
        int run(int output, type tol , int max_iter, double mipgap, bool relax, pair<bool,string> lin_solver, double time_limit ){
            int return_status = -1, dyn_max_iter = 20;
            bool violated_constraints = true, optimal = true, solver_violated=false;
            unsigned nb_it = 0;
            while(violated_constraints && optimal){
                if (_stype==ipopt) {
#ifdef USE_IPOPT
                    auto nb_eq = _model->get_nb_eq();
                    auto nb_vars = _model->get_nb_vars();
                    if(nb_vars<nb_eq){
                        auto nb_aux = nb_eq-nb_vars+1;
                        auto aux = var<>("aux_var");
                        try{
                            _model->add(aux.in(R(nb_aux)));
                        }
                        catch(invalid_argument){
                            throw invalid_argument("aux_var is a reserved variable name, please rename your variable");
                        }
                    }
                    double mu_init = std::exp(5)/std::exp(1);
                    SmartPtr<IpoptApplication> iapp = IpoptApplicationFactory();
                    iapp->RethrowNonIpoptException(true);
                    ApplicationReturnStatus status = iapp->Initialize();
                    
                    if (status != Solve_Succeeded) {
                        std::cout << std::endl << std::endl << "*** Error during initialization!" << std::endl;
                        return (int) status;
                    }
                    if(lin_solver.first){
                        iapp->Options()->SetStringValue("linear_solver", lin_solver.second);
                    }
                    //                    iapp->Options()->SetStringValue("mehrotra_algorithm", mehrotra);
                    iapp->Options()->SetNumericValue("tol", tol);
                    iapp->Options()->SetIntegerValue("print_level", output);
                    iapp->Options()->SetStringValue("honor_original_bounds", "no");
                    /** Bonmin options */
                    //            iapp->Options()->SetStringValue("mu_strategy", "adaptive");
                    //            iapp->Options()->SetStringValue("mu_oracle", "probing");
                    //            iapp->Options()->SetNumericValue("gamma_phi", 1e-8);
                    //            iapp->Options()->SetNumericValue("gamma_theta", 1e-4);
                    //                                iapp->Options()->SetNumericValue("bound_push", 1e-12);
                    //                                iapp->Options()->SetNumericValue("bound_frac", 1e-12);
                    //                                iapp->Options()->SetIntegerValue("acceptable_iter", 0);
                    //            iapp->Options()->SetNumericValue("slack_bound_push", 1e-12);
                    iapp->Options()->SetNumericValue("constr_viol_tol", tol);
                    //            iapp->Options()->SetNumericValue("dual_inf_tol", 1);
                    //            iapp->Options()->SetNumericValue("compl_inf_tol", 1e-3);
                    iapp->Options()->SetNumericValue("bound_relax_factor", tol*0.1);
                    //            iapp->Options()->SetNumericValue("bound_relax_factor", 0);
                    //                    iapp->Options()->SetStringValue("derivative_test", "second-order");
                    //            iapp->Options()->SetNumericValue("mu_init", mu_init);
                    //            iapp->Options()->SetNumericValue("obj_scaling_factor", 1e-2);
                    /** Hot start if already solved */
                    if (!_model->_first_run) {
                        mu_init = std::exp(-1)/std::exp(2);
                        DebugOn("Using Hot Start!\n");
                        iapp->Options()->SetNumericValue("mu_init", mu_init);
                        iapp->Options()->SetStringValue("warm_start_init_point", "yes");
                    }
                    iapp->Options()->SetStringValue("sb", "yes");
//                    _model->_first_run = false;
                    iapp->Options()->SetIntegerValue("max_iter", max_iter);
                    
                    //                        iapp->Options()->SetStringValue("hessian_approximation", "limited-memory");
                    //                        iapp->Options()->SetStringValue("hessian_constant", "yes");
                    //                        iapp->Options()->SetStringValue("derivative_test", "only-second-order");
                    //                iapp->Options()->SetNumericValue("ma57_pre_alloc", 10);
                    //            iapp->Options()->SetIntegerValue("ma57_small_pivot_flag", 1);
                    //            iapp->Options()->SetNumericValue("ma27_liw_init_factor", 20);
                    //            iapp->Options()->SetNumericValue("ma27_la_init_factor", 20);
                    //            iapp->Options()->SetNumericValue("ma27_meminc_factor", 3);
                    //            iapp->Options()->SetStringValue("ma57_automatic_scaling", "yes");
                    //                    iapp->Options()->SetStringValue("derivative_test", "second-order");
                    //                            iapp->Options()->SetNumericValue("derivative_test_perturbation", 1e-6);
                    //                        iapp->Options()->SetNumericValue("print_level", 10);
                    
                    //                                                iapp->Options()->SetStringValue("derivative_test", "second-order");
                    
                    
                    
                    //                        iapp->Options()->SetNumericValue("ma27_liw_init_factor", 50);
                    //                        iapp->Options()->SetNumericValue("ma27_pivtol", 1e-6);
                    //                        iapp->Options()->SetNumericValue("ma27_la_init_factor", 100);
                    //                        iapp->Options()->SetNumericValue("ma27_meminc_factor", 5);
                    //                        iapp->Options()->SetStringValue("mu_strategy", "adaptive");
                    iapp->Options()->SetNumericValue("tol", tol);
                    //                            iapp->Options()->SetNumericValue("dual_inf_tol", 1e-6);
                    //                                                        iapp->Options()->SetStringValue("derivative_test", "second-order");
                    //                            iapp->Options()->SetNumericValue("bound_relax_factor", 0);
                    //            iapp.Options()->SetIntegerValue("print_level", 5);
                    
                    //                        iapp->Options()->SetStringValue("derivative_test_print_all", "yes");
                    for(const auto &op: _str_options){
                        iapp->Options()->SetStringValue(op.first, op.second);
                    }
                    for(const auto &op: _int_options){
                        iapp->Options()->SetIntegerValue(op.first, op.second);
                    }
                    for(const auto &op: _double_options){
                        iapp->Options()->SetNumericValue(op.first, op.second);
                    }
                    if(!_model->_built){ /* Constraints have been added */
                        _model->reindex();
                    }
                    _model->fill_in_maps();
                    
                    SmartPtr<TNLP> tmp = new IpoptProgram<type>(_model);
                    status = iapp->OptimizeTNLP(tmp);
                    if (IsValid(iapp->Statistics())) {
                        SmartPtr<SolveStatistics> stats = iapp->Statistics();
                        _nb_iterations = stats->IterationCount();
                    }
                    if(_model->_objt==maximize){
                        *_model->_obj *= -1;
                        _model->_obj->_val->at(0) *= -1;
                    }
                    DebugOff("Return status = " << status << endl);
                    if (status == Solve_Succeeded) {
                        optimal = true;
                        _model->round_solution();
                        // Retrieve some statistics about the solve
                        //                printf("\n\nSolution of the primal variables:\n");
                        //                _model->print_solution();
                        //                return status;
                        //                    return_status = status;
                    }
                    else if (status == Solved_To_Acceptable_Level) {
                        //                return 150;
                        optimal = true;
                    }
                    else {
                        optimal = false;
                    }
                    return_status = optimal ? 0 : -1;
#else
                    ipoptNotAvailable();
#endif
                }
                else if (_stype == clp){
#ifdef USE_CLP
                    auto clp_prog = (ClpProgram*)(_prog.get());
                    clp_prog->prepare_model();
                    optimal = clp_prog->solve();
                    
#else
                    ClpNotAvailable();
#endif
                }
                else if(_stype==gurobi)
                {
#ifdef USE_GUROBI
                    try{
                        
                        auto grb_prog = (GurobiProgram*)(_prog.get());
                        grb_prog->_output = output;
                        //            prog.grb_prog->reset_model();
                        grb_prog->prepare_model();
                        optimal = grb_prog->solve(output, relax, tol, mipgap, _bool_options["gurobi_crossover"]);
                        return_status = optimal ? 0 : -1;
//                        delete grb_prog->grb_mod;
//                        delete grb_prog->grb_env;
//                        grb_prog->grb_mod = nullptr;
//                        grb_prog->grb_env = nullptr;
                    }catch(GRBException e) {
                        cerr << "\nError code = " << e.getErrorCode() << endl;
                        cerr << e.getMessage() << endl;
                    }
#else
                    gurobiNotAvailable();
#endif
                }
                else if(_stype==cplex)
                {
#ifdef USE_CPLEX
                    try{
                        if(_model->_type!=lin_m && _model->_type!=quad_m ){
                            throw invalid_argument("Only linear and quadratic models are supported by CPLEX");
                        }
                        auto cplex_prog = (CplexProgram*)(_prog.get());
                        cplex_prog->_output = output;
                        cplex_prog->prepare_model();
                        optimal = cplex_prog->solve(output=0, relax, tol, mipgap);
                        
                        return_status = optimal ? 0 : -1;
                    }
                    catch(IloException e) {
                        cerr << e.getMessage() << endl;
                    }
#else
                    cplexNotAvailable();
#endif
                }
                else if(_stype == _mosek)
                {
#ifdef USE_MOSEK
                    try{
                        auto mosek_prog = (MosekProgram*)(_prog.get());
                        mosek_prog->_output = output;
                        mosek_prog->prepare_model();
                        bool ok = mosek_prog->solve(relax);
                        return_status = ok ? 0 : -1;
                    }
                    catch(mosek::fusion::FusionException e) {
                        cerr << e.toString() << endl;
                    }
#else
                    mosekNotAvailable();
#endif
                }
                else if(_stype==bonmin) {
#ifdef USE_BONMIN
                    
                    //            using namespace Bonmin;
                    BonminSetup bonmin;
                    bonmin.initializeOptionsAndJournalist();
                    //            bonmin.options()->SetIntegerValue("max_consecutive_infeasible", 100);
                    //            bonmin.options()->SetIntegerValue("solution_limit", 1);
                    //            bonmin.options()->SetIntegerValue("bb_log_level", 3);
                    //            bonmin.options()->SetNumericValue("allowable_gap", -1e5);
                    //            bonmin.options()->SetNumericValue("allowable_fraction_gap", -1e5);
                    //            bonmin.options()->SetNumericValue("tol", tol);
                    //            bonmin.options()->SetIntegerValue("print_level", 5);
                    bonmin.options()->SetStringValue("tree_search_strategy", "top-node");
                    bonmin.options()->SetStringValue("warm_start", "optimum");
                    
                    //            bonmin.options()->SetStringValue("variable_selection", "random");
                    //            bonmin.options()->SetStringValue("linear_solver", "ma57");
                    //            bonmin.options()->SetIntegerValue("nlp_log_at_root", 5);
                    bonmin.options()->SetNumericValue("cutoff", 0.9);
                    //            bonmin.options()->SetNumericValue("resolve_on_small_infeasibility", INFINITY);
                    bonmin.options()->SetStringValue("heuristic_dive_MIP_fractional", "no");
                    bonmin.options()->SetStringValue("heuristic_dive_MIP_vectorLength", "no");
                    bonmin.options()->SetStringValue("heuristic_dive_fractional", "no");
                    bonmin.options()->SetStringValue("heuristic_dive_vectorLength", "no");
                    bonmin.options()->SetStringValue("heuristic_feasibility_pump", "no");
                    bonmin.options()->SetStringValue("pump_for_minlp", "no");
                    //            bonmin.options()->SetStringValue("algorithm", "B-iFP");
                    bonmin.options()->SetStringValue("node_comparison", "best-guess");
                    //            bonmin.options()->SetStringValue("dynamic_def_cutoff_decr", "yes");
                    bonmin.options()->SetIntegerValue("num_resolve_at_root", 5);
                    
                    _model->fill_in_maps();
                    SmartPtr<TMINLP> tmp = new BonminProgram(_model);
                    bonmin.initialize(tmp);
                    
                    
                    bool ok = false;
                    //        bonmin.initialize(prog.bonmin_prog);
                    try {
                        Bab bb;
                        bb(bonmin);
                        auto status = bb.mipStatus();
                        switch (bb.mipStatus())
                        {
                            case Bab::MipStatuses::FeasibleOptimal:
                            case Bab::MipStatuses::Feasible:
                                ok = true;
                                break;
                            default:
                                ok = false;
                        }
                    }
                    catch(TNLPSolver::UnsolvedError *E) {
                        //There has been a failure to solve a problem with Bonmin.
                        std::cerr << "Bonmin has failed to solve a problem" << std::endl;
                        ok = false;
                    }
                    catch(OsiTMINLPInterface::SimpleError &E) {
                        std::cerr << E.className() << "::" << E.methodName() << std::endl << E.message() << std::endl;
                        ok = false;
                    }
                    catch(CoinError &E) {
                        std::cerr << E.className() << "::" << E.methodName() << std::endl << E.message() << std::endl;
                        ok = false;
                    }
                    catch(...) {
                        std::cerr << "Unknown error: Bonmin failed to solve the problem." << std::endl;
                        ok = false;
                    }
                    
                    if(_model->_objt==maximize){
                        *_model->_obj *= -1;
                        _model->_obj->_val->at(0) *= -1;
                    }
                    
                    return_status = ok ? 0 : -1;
#else
                    bonminNotAvailable();
#endif
                }
                if (optimal) {
                    auto res=_model->has_violated_constraints(tol);
                    violated_constraints = res.first;
                    solver_violated=res.second;
                    if (violated_constraints||(_stype==ipopt && solver_violated && _bool_options["check_violation"])) {
                        _model->reindex();
                        //                        _model->print();
                        //                        _model->print_symbolic();
                        //                Constraint obj_cstr("obj_cstr_"+to_string(nb_it));
                        //                obj_cstr += _model->_obj - _model->_obj_ub;
                        //                if (_model->_objt==minimize) {
                        //                    _model->add(obj_cstr <= 0);
                        //                }
                        //                else {
                        //                    _model->add(obj_cstr >= 0);
                        //                }
                        //                _model->print();
                    }
                    if(_stype==ipopt && solver_violated && _bool_options["check_violation"]) {
                        violated_constraints = true;
                        if(_double_options.find("bound_relax_factor")!=_double_options.end()){
                            _double_options["bound_relax_factor"]=_double_options["bound_relax_factor"]*0.1;
                        }
                        else{
                            _double_options["bound_relax_factor"]=tol*0.01;
                        }
                    }
                    nb_it++;
                }
            }
            if (nb_it>1) {
                DebugOn(endl << "####################" << endl);
                DebugOn("Solved in " << nb_it << " constraint generation iterations" << endl);
                DebugOn("####################" << endl);
            }
            _model->_status = return_status;
            if(_model->_status == 0){
                DebugOff("Solved to Optimality/Acceptable "<< endl);
            }
            return return_status;
        }
        
    };
    
    template<typename type>
    int run_models(const std::vector<shared_ptr<Model<type>>>& models, size_t start, size_t end, SolverType stype, type tol, const string& lin_solver="", unsigned max_iter = 1e6){
        int return_status = -1;
        for (auto i = start; i<end; i++) {
            return_status = solver<type>((models.at(i)),stype).run(0, tol, lin_solver, max_iter);
            DebugOff("Return status "<<return_status << endl);
            //            models.at(i)->print_solution(24);
        }
        return return_status;
    }
    
    //    template<typename type>
    //    int run_parallel(const initializer_list<shared_ptr<gravity::Model<type>>>& models, gravity::SolverType stype = ipopt, type tol = 1e-6, unsigned nr_threads=std::thread::hardware_concurrency(), const string& lin_solver="", unsigned max_iter = 1e6){
    //        return run_parallel(vector<shared_ptr<gravity::Model<type>>>(models), stype, tol, nr_threads, lin_solver, max_iter);
    //    }
    
    
    
    /* Runs models stored in the vector in parallel, using solver of stype and tolerance tol */
    //run_parallel,ref(vec),stype,tol,nr_threads,lin_solver,max_iter);
    int run_parallel(const vector<shared_ptr<gravity::Model<double>>>& models, gravity::SolverType stype = ipopt, double tol = 1e-6, unsigned nr_threads=std::thread::hardware_concurrency(), const string& lin_solver="", int max_iter=1e6);
    
    int run_parallel(const vector<shared_ptr<gravity::Model<double>>>& models, gravity::SolverType stype, double tol, unsigned nr_threads, int max_iter);
    
    int run_parallel_new(const std::vector<std::string> objective_models, std::vector<double>& sol_obj, std::vector<int>& sol_status, const std::vector<shared_ptr<gravity::Model<double>>>& models, gravity::SolverType stype=ipopt, double tol=1e-6, unsigned nr_threads=std::thread::hardware_concurrency(), const string& lin_solver="", int max_iter=1e6);
    
    int run_parallel_new(const std::vector<std::string> objective_models, std::vector<double>& sol_obj, std::vector<int>& sol_status, const std::vector<shared_ptr<gravity::Model<double>>>& models, gravity::SolverType stype, double tol, unsigned nr_threads, const string& lin_solver, int max_iter);
    
    
#ifdef USE_MPI
    
    
    template<typename type>
    void send_status(const vector<shared_ptr<gravity::Model<type>>>& models, const vector<size_t>& limits){
        int worker_id, nb_workers;
        auto err_rank = MPI_Comm_rank(MPI_COMM_WORLD, &worker_id);
        auto err_size = MPI_Comm_size(MPI_COMM_WORLD, &nb_workers);
        auto nb_workers_ = std::min((size_t)nb_workers, models.size());
        DebugOff("nb_workers_ = " << nb_workers_ << ", models.size() = " << models.size() << endl);
        DebugOff("I'm worker ID: " << worker_id << ", I'm getting ready to broadcast my solutions " << endl);
        for (auto w_id = 0; w_id<nb_workers; w_id++) {
            if(w_id+1<limits.size()){
                for (auto i = limits[w_id]; i < limits[w_id+1]; i++) {
                    auto model = models[i];
                    DebugOff("I'm worker ID: " << worker_id << "I will call MPI_Bcasr with i = " << i << " and w_id =  " << w_id << endl);
                    MPI_Bcast(&model->_status, 1, MPI_INT, w_id, MPI_COMM_WORLD);
                }
            }
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }
    
    template<typename type>
    void send_status_new(const vector<shared_ptr<gravity::Model<type>>>& models, const vector<size_t>& limits, vector<int>& sol_status){
        int worker_id, nb_workers;
        auto err_rank = MPI_Comm_rank(MPI_COMM_WORLD, &worker_id);
        auto err_size = MPI_Comm_size(MPI_COMM_WORLD, &nb_workers);
        int count=0;
        DebugOff("I'm worker ID: " << worker_id << ", I'm getting ready to send my status " << endl);
        for (auto w_id = 0; w_id<nb_workers; w_id++) {
            if(w_id+1<limits.size()){
                count=0;
                if(worker_id==w_id){
                    for (auto i = limits[w_id]; i < limits[w_id+1]; i++) {
                        auto model = models[count++];
                        sol_status[i]=model->_status;
                    }
                }
                DebugOff("I'm worker ID: " << worker_id << "I will call MPI_Bcasr with i = " << i << " and w_id =  " << w_id << endl);
                MPI_Bcast(&sol_status[limits[w_id]], (limits[w_id+1]-limits[w_id]), MPI_INT, w_id, MPI_COMM_WORLD);
            }
        }
        //MPI_Barrier(MPI_COMM_WORLD);
    }
    
    /** Send model solutions to all workers
     @models vector of models with stored solutions
     @limits vector specifying which models are assigned to which workers
     */
    template<typename type>
    void send_solution_all(const vector<shared_ptr<gravity::Model<type>>>& models, const vector<size_t>& limits){
        int worker_id, nb_workers;
        auto err_rank = MPI_Comm_rank(MPI_COMM_WORLD, &worker_id);
        auto err_size = MPI_Comm_size(MPI_COMM_WORLD, &nb_workers);
        auto nb_workers_ = std::min((size_t)nb_workers, models.size());
        DebugOff("nb_workers_ = " << nb_workers_ << ", models.size() = " << models.size() << endl);
        DebugOff("I'm worker ID: " << worker_id << ", I'm getting ready to broadcast my solutions " << endl);
        for (auto w_id = 0; w_id<nb_workers; w_id++) {
            if(w_id+1<limits.size()){
                for (auto i = limits[w_id]; i < limits[w_id+1]; i++) {
                    auto model = models[i];
                    if(model->_status==0){
                        auto nb_vars = model->get_nb_vars();
                        vector<double> solution;
                        solution.resize(nb_vars);
                        if(worker_id==w_id){
                            model->get_solution(solution);
                        }
                        DebugOff("I'm worker ID: " << worker_id << "I will call MPI_Bcasr with i = " << i << " and w_id =  " << w_id << endl);
                        MPI_Bcast(&solution[0], nb_vars, MPI_DOUBLE, w_id, MPI_COMM_WORLD);
                        if(worker_id!=w_id){
                            model->set_solution(solution);
                        }
                    }
                }
            }
        }
        MPI_Barrier(MPI_COMM_WORLD);	
    }
    
    template<typename type>
    void send_solution_all_new(const vector<shared_ptr<gravity::Model<type>>>& models, const vector<size_t>& limits, const vector<int>& sol_status, std::vector<std::vector<double>>& sol_val){
        int worker_id, nb_workers;
        auto err_rank = MPI_Comm_rank(MPI_COMM_WORLD, &worker_id);
        auto err_size = MPI_Comm_size(MPI_COMM_WORLD, &nb_workers);
        int nb_vars;
        nb_vars=models[0]->_nb_vars;
        std::vector<double> solution(nb_vars);
        for (auto w_id = 0; w_id<nb_workers; w_id++) {
            if(w_id+1<limits.size()){
                for (auto i = limits[w_id]; i < limits[w_id+1]; i++) {
                    if(sol_status[i]==0){
                        if(worker_id==w_id){
                                models[i-limits[w_id]]->get_solution(solution);
                                sol_val.at(i)=solution;
                            }
                        MPI_Bcast(&sol_val[i][0], nb_vars, MPI_DOUBLE, w_id, MPI_COMM_WORLD);
                    }
                }
            }
        }
    }
    
    /** Send model objective value to all workers
     @models vector of models with stored solutions
     @limits vector specifying which models are assigned to which workers
     */
    template<typename type>
    void send_obj_all(const vector<shared_ptr<gravity::Model<type>>>& models, const vector<size_t>& limits){
        int worker_id, nb_workers;
        auto err_rank = MPI_Comm_rank(MPI_COMM_WORLD, &worker_id);
        auto err_size = MPI_Comm_size(MPI_COMM_WORLD, &nb_workers);
        auto nb_workers_ = std::min((size_t)nb_workers, models.size());
        DebugOff("nb_workers_ = " << nb_workers_ << ", models.size() = " << models.size() << endl);
        DebugOff("I'm worker ID: " << worker_id << ", I'm getting ready to broadcast my solutions " << endl);
        for (auto w_id = 0; w_id<nb_workers; w_id++) {
            if(w_id+1<limits.size()){
                for (auto i = limits[w_id]; i < limits[w_id+1]; i++) {
                    auto model = models[i];
                    if(model->_status==0){
                        DebugOff("I'm worker ID: " << worker_id << "I will call MPI_Bcasr with i = " << i << " and w_id =  " << w_id << endl);
                        MPI_Bcast(&model->_obj->_val->at(0), 1, MPI_DOUBLE, w_id, MPI_COMM_WORLD);
                    }
                }
            }
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }
    template<typename type>
    void send_obj_all_new(const vector<shared_ptr<gravity::Model<type>>>& models, const vector<size_t>& limits, vector<double>& sol_obj){
        int worker_id, nb_workers;
        auto err_rank = MPI_Comm_rank(MPI_COMM_WORLD, &worker_id);
        auto err_size = MPI_Comm_size(MPI_COMM_WORLD, &nb_workers);
        auto nb_workers_ = std::min((size_t)nb_workers, models.size());
        int count=0;
        DebugOff("nb_workers_ = " << nb_workers_ << ", models.size() = " << models.size() << endl);
        DebugOff("I'm worker ID: " << worker_id << ", I'm getting ready to broadcast my solutions " << endl);
        for (auto w_id = 0; w_id<nb_workers; w_id++) {
            if(w_id+1<limits.size()){
                count=0;
                if(worker_id==w_id){
                    for (auto i = limits[w_id]; i < limits[w_id+1]; i++) {
                        auto model = models[count++];
                        if(model->_status==0){
                            sol_obj[i]=model->_obj->_val->at(0);
                        }
                    }
                }
                DebugOff("I'm worker ID: " << worker_id << "I will call MPI_Bcasr with i = " << i << " and w_id =  " << w_id << endl);
                MPI_Bcast(&sol_obj[limits[w_id]], (limits[w_id+1]-limits[w_id]), MPI_DOUBLE, w_id, MPI_COMM_WORLD);
            }
        }
        //MPI_Barrier(MPI_COMM_WORLD);
    }
    
    /** Runs models stored in the vector in parallel using MPI
     @models vector of models to run in parallel
     @stype Solver type
     @tol numerical tolerance
     @max_iter max number of iterations per model
     @max_batch_time max wall clock time of each batch
     @nb_threads Number of parallel threads per worker
     @lin_solver linear system solver
     @share_all propagate model status and solutions to all workers, if false, only worker 0 has updated solutions and status flags for all models
     @share_all_obj propagate only objective values and model status to all workers
     */
    int run_MPI(const vector<shared_ptr<gravity::Model<double>>>& models, gravity::SolverType stype = ipopt, double tol = 1e-6, unsigned nr_threads=std::thread::hardware_concurrency(), const string& lin_solver="", int max_iter = 1e6, int max_batch_time = 1e6, bool share_all = false, bool share_all_obj = false);
    void run_MPI(const initializer_list<shared_ptr<gravity::Model<double>>>& models, gravity::SolverType stype = ipopt, double tol = 1e-6, unsigned nr_threads=std::thread::hardware_concurrency(), const string& lin_solver="", int max_iter = 1e6, int max_batch_time = 1e6, bool share_all = false, bool share_all_obj = false);
    int run_MPI_new(const std::vector<std::string> objective_models, std::vector<double>& sol_obj, std::vector<int>& sol_status, const vector<shared_ptr<gravity::Model<double>>>& models, gravity::SolverType stype = ipopt, double tol = 1e-6, unsigned nr_threads=std::thread::hardware_concurrency(), const string& lin_solver="", int max_iter = 1e6, int max_batch_time = 1e6, bool share_all_obj = false);
    
#endif
}
#endif /* defined(__Gravity____Solver__) */
