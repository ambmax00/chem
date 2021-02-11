#include <mpi.h>
#include <omp.h>
#include <random>
#include <stdexcept>
#include <string>
#include <dbcsr_matrix.hpp>
#include <dbcsr_conversions.hpp>
#include <dbcsr_matrix_ops.hpp>
#include "io/parser.h"
#include "io/data_handler.h"
#include "hf/hfmod.h"
#include "mp/mpmod.h"
#include "adc/adcmod.h"
#include "utils/mpi_time.h"
#include "utils/unique.h"
#include "extern/scalapack.h"

#include <filesystem>
#include <chrono>
#include <thread>

#include "math/solvers/davidson.h"
#include <Eigen/Eigenvalues>
#include <Eigen/Core>

int main(int argc, char** argv) {

	MPI_Init(&argc, &argv);
	MPI_Comm comm = MPI_COMM_WORLD;
	
	dbcsr::init();
	dbcsr::world wrd(comm);
	
	util::mpi_time time(comm, "Megalochem");
	util::mpi_log LOG(comm ,0);
	
	//std::cout << std::scientific;

#ifndef DO_TESTING
	if (argc != 3) {
		LOG.os<>("Usage: ./chem [filename] [working_directory]\n");
		exit(0);
	}
#else
	LOG.os<>("MEGALOCHEM TEST RUN.\n");
#endif
	
	std::string s = R"(|  \/  ||  ___|  __ \ / _ \ | |   |  _  /  __ \| | | ||  ___|  \/  |)""\n"
					R"(| .  . || |__ | |  \// /_\ \| |   | | | | /  \/| |_| || |__ | .  . |)""\n"
					R"(| |\/| ||  __|| | __ |  _  || |   | | | | |    |  _  ||  __|| |\/| |)""\n"
					R"(| |  | || |___| |_\ \| | | || |___\ \_/ / \__/\| | | || |___| |  | |)""\n"
					R"(\_|  |_/\____/ \____/\_| |_/\_____/\___/ \____/\_| |_/\____/\_|  |_/)";
	
	LOG.banner(s,90,'*');
	LOG.os<>('\n');
	
#ifndef NDEBUG
	LOG.os<>("Build type: DEBUG\n\n");
#else 
	LOG.os<>("Build type: RELEASE\n\n");
#endif

	LOG.os<>("Authors: \n \t M. A. Ambroise\n\n");
	
	std::string filename(argv[1]);
	std::string workdir(argv[2]);
	
	if (!std::filesystem::exists(workdir))
		throw std::runtime_error("Working directory does not exist.");
	
	//set working directory
	std::filesystem::current_path(workdir);
	
	// set up files
	std::string hdf5file = filename + ".hdf5";
	std::string hdf5backup = filename + ".back.hdf5";
	
	filio::data_handler *dh, *dh_back;
	
	if (std::filesystem::exists(hdf5file)) {
		if (wrd.rank() == 0 && std::filesystem::exists(hdf5backup))
			std::filesystem::remove(hdf5backup);
		
		if (wrd.rank() == 0) std::filesystem::copy(hdf5file, hdf5backup);
		
		dh_back = new filio::data_handler(hdf5backup, 
			filio::create_mode::append, comm);
	}
	
	dh = new filio::data_handler(hdf5file, filio::create_mode::truncate, 
		comm);
	
	LOG.os<>("Running ", wrd.size(), " MPI processes with ", omp_get_max_threads(), " threads each.\n\n");
	
	time.start();
	
	char* emergency_buffer = new char[1000];

	try { // BEGIN MAIN CONTEXT
	
	int sysctxt = -1;
	c_blacs_get(0, 0, &sysctxt);
	int gridctxt = sysctxt;
	c_blacs_gridinit(&gridctxt, 'R', wrd.nprow(), wrd.npcol());
	scalapack::global_grid.set(gridctxt);
	
	std::string input_file = filename + ".json";
	if (!std::filesystem::exists(input_file)) {
		throw std::runtime_error("Could not find input file!");
	}
	
	nlohmann::json data;
	std::ifstream ifstr(input_file);
	ifstr >> data;
	
	auto mol = filio::parse_molecule(data,comm,0);
	auto opt = filio::parse_options(data,comm,0);
	
	desc::write_molecule("molecule", *mol, *dh);
	
	if (wrd.rank() == 0) {
		opt.print();
	}
	
	auto hfopt = opt.subtext("hf");
	
	hf::shared_hf_wfn myhfwfn;
	hf::hfmod myhf(wrd,mol,hfopt);

	bool skip_hf = hfopt.get<bool>("skip", false);
	bool do_hf = opt.get<bool>("do_hf", true);
	
	if (do_hf && !skip_hf) {
	
		myhf.compute();
		myhfwfn = myhf.wfn();
		hf::write_hfwfn("hf_wfn", *myhfwfn, *dh);
		
	} else {
		
		LOG.os<>("Reading HF info from files...\n");
		myhfwfn = hf::read_hfwfn("hf_wfn", mol, wrd, *dh_back);
		hf::write_hfwfn("hf_wfn", *myhfwfn, *dh);
		LOG.os<>("Done.\n");
		
	}
	
	MPI_Barrier(MPI_COMM_WORLD);
	
	auto mpopt = opt.subtext("mp");
	
	bool do_mp = opt.get<bool>("do_mp");
	
	if (do_mp) {
	
		mp::mpmod mymp(wrd,myhfwfn,mpopt);
		mymp.compute();
		
	}

	auto adcopt = opt.subtext("adc");
	bool do_adc = opt.get<bool>("do_adc");
	
	if (do_adc) {
		
		adc::adcmod myadc(wrd,myhfwfn,adcopt);
		myadc.compute();
	}
	
	scalapack::global_grid.free();

   } catch (std::exception& e) {
	
	   	delete [] emergency_buffer;
		if (wrd.rank() == 0) {
			std::cout << "ERROR: ";
			std::cout << e.what() << std::endl;
			std::string skull = R"(
			
			
                  _( )                 ( )_
                 (_, |      __ __      | ,_)
                    \'\    /  ^  \    /'/
                     '\'\,/\      \,/'/'
                       '\| []   [] |/'
                         (_  /^\  _)
                           \  ~  /
                           /HHHHH\
                         /'/{^^^}\'\
                     _,/'/'  ^^^  '\'\,_
                    (_, |           | ,_)
                      (_)           (_)

            =====================================
            =      MEGALOCHEM WAS ABORTED       =
            =====================================
		)";
			std::cout << skull << std::endl;
			MPI_Abort(wrd.comm(), MPI_ERR_OTHER);
		} else {
		
			std::this_thread::sleep_for(std::chrono::milliseconds(10000));
			MPI_Abort(wrd.comm(), MPI_ERR_OTHER);
			
		}
	   
   }
    
    LOG.os<>("Wrapping up...\n");
    
    time.finish();
	
	time.print_info();
	
	dbcsr::print_statistics(true);

	//dbcsr::finalize();

	LOG.os<>("========== FINISHED WITHOUT CRASHING ! =========\n");

#ifdef DO_TESTING
	LOG.os<>("Now comparing reference to output\n");
	std::string output_ref(argv[3]);
	
	if (wrd.rank() == 0) {
		bool is_same = filio::compare_outputs(
			filename + "_data/out.json", output_ref);
		if (!is_same) {
			LOG.os<>("Not the same\n");
			MPI_Abort(wrd.comm(), MPI_ERR_OTHER);
		}
	}
#endif 

	MPI_Finalize();

	return 0;

}
