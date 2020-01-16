#include "input/reader.h"
#include "utils/json.hpp"
#include "input/valid_keys.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <sstream>
#include <utility>
#include "utils/ele_to_int.h"
#include "utils/constants.h"
#include "desc/molecule.h"
#include <libint2/basis.h>

void validate(const json& j, const json& compare) {
	
	for (auto it = j.begin(); it != j.end(); ++it) {
		std::cout << it.key() << std::endl;
		if (compare.find(it.key()) == compare.end()){
			throw std::runtime_error("Invalid keyword: "+it.key());
		}
		if (it->is_structured() && !it->is_array() && it.key() != "gen_basis") {
			validate(*it, compare[it.key()]);
		}
	}
}

std::vector<libint2::Atom> get_geometry(const json& j) {
	
	std::vector<libint2::Atom> out;
	
	if (j.find("file") == j.end()) {
		// reading from input file
		auto geometry = j["geometry"];
		auto symbols = j["symbols"];
		
		if ((geometry.size() % 3 != 0) || (geometry.size() / 3 != symbols.size())) {
			throw std::runtime_error("Missing coordinates.");
		}
		
		for (int i = 0; i != geometry.size()/3; ++i) {
			libint2::Atom a;
			a.x = geometry.at(3*i);
			a.y = geometry.at(3*i+1);
			a.z = geometry.at(3*i+2);
			a.atomic_number = util::ele_to_int[symbols.at(i)];
			
			out.push_back(a);
			
		}
		
	} else {
		// read from xyz file
		std::ifstream in;
		in.open(j["file"]);
		
		if (!in) {
			throw std::runtime_error("XYZ data file not found.");
		}
		
		std::string line;
		
		std::string ele_name;
		int nline = 0;
		double x, y, z;
		
		while(std::getline(in,line)) {
			
			if (nline > 1) {
				std::stringstream ss(line);
				std::cout << line << std::endl;
				
				libint2::Atom atom;
				atom.x = x;
				atom.y = y;
				atom.z = z;
				atom.atomic_number = util::ele_to_int[ele_name];
				
				out.push_back(atom);
			}
			
			++nline;
			
		}
		
	}
	
	double factor;
	
	if (j.find("unit") != j.end()) {
		if (j["unit"] == "angstrom") {
			factor = BOHR_RADIUS;
		} else {
			throw std::runtime_error("Unknown length unit.");
		}
	} else {
		factor = BOHR_RADIUS;
	}
	
	for (auto& a : out) {
		a.x /= factor;
		a.y /= factor;
		a.z /= factor;
		
		std::cout << a.x << " " << a.y << " " << a.z << std::endl;
		
	}
	
	return out;
	
}

std::vector<libint2::Shell> read_basis(const json &jbas, std::vector<libint2::Atom> &atoms) {
	
	//libint2::Shell::do_enforce_unit_normalization(false);
	
	/*
	// check basis
	auto name = j["basis"];
	
	libint2::BasisSet basis(name, atoms);
	
	std::vector<libint2::Shell> out = std::move(basis);
	
	std::cout << "BASIS" << std::endl;
	
	
	// read input*/
	
	//read per atom basis
	//auto& jbas = j["gen_basis"];
	
	std::map<int,std::vector<libint2::Shell>> basis_map;
	
	// basis name?
	auto& jele = jbas["elements"];
	
	for (auto ele = jele.begin(); ele != jele.end(); ++ele) {
      
      auto strv_dv = [](std::vector<std::string>& str_v) {
		  libint2::svector<double> v(str_v.size());
		  for (int i = 0; i != v.size(); ++i) {
			  v[i] = std::stod(str_v[i]);
		  }
		  return v;
	  };
      
      auto& jshells = ele.value()["electron_shells"];
      
      std::vector<libint2::Shell> vecshell;
      
      for (auto& shell : jshells) {
		  
		  std::vector<int> ang_moms = shell["angular_momentum"];
	
		  std::vector<std::string> str_exp = shell["exponents"];
		  auto exp = strv_dv(str_exp);
		  
		  auto& jcoeffs = shell["coefficients"];
		  
		  libint2::svector<double> max_ln(jcoeffs.size());
		  libint2::svector<libint2::Shell::Contraction> veccons;
		  
		  for (int i = 0; i != jcoeffs.size(); ++i) {
			  
			std::vector<std::string> str_v = jcoeffs[i];
			auto v = strv_dv(str_v);
			
			auto max = std::max_element(v.begin(), v.end());
			max_ln[i] = log(*max);
			
			libint2::Shell::Contraction c;
			c.l = ang_moms[i];
			c.pure = true; 
			c.coeff = v;
			
			veccons.push_back(c);
			
		  }
		  
		  libint2::Shell s;
		  s.alpha = exp;
		  s.contr = veccons;
		  s.max_ln_coeff = max_ln;
		  
		  vecshell.push_back(std::move(s));
		  
	  }
		  
	  int ele_num = std::stoi(ele.key());
	  
	  basis_map[ele_num] = vecshell;
		  
	}
	
	std::vector<libint2::Shell> tot_basis;
	
	for (auto a : atoms) {
		auto& ele_basis = basis_map[a.atomic_number];
		std::array<double,3> new_o = {a.x, a.y, a.z};
		
		for (auto& s : ele_basis) s.move(new_o);
		
		tot_basis.insert(tot_basis.end(), ele_basis.begin(), ele_basis.end());
	}
	
	for (auto b : tot_basis) {
		std::cout << b << std::endl;
	}
	
	return tot_basis;
	
}

void unpack(const json& j_in, desc::options& opt, std::string root) {
	
	auto j = j_in[root];
	
	for (auto it = j.begin(); it != j.end(); ++it) {
		
		if (it->type() == json::value_t::boolean) {
			opt.set<bool>(root + "/" + it.key(), *it);
		}
		
		if (it->type() == json::value_t::number_integer) {
			opt.set<int>(root + "/" + it.key(), *it);
		}
		
		if (it->type() == json::value_t::number_float) {
			opt.set<double>(root + "/" + it.key(), *it);
		}
		
		if (it->type() == json::value_t::string) {
			opt.set<std::string>(root + "/" + it.key(), *it);
		}
		
		//if (it->type() == json::value_t::array) {
		//	opt.set<std::vector<int>>(root + it.key(), *it);
		//}
		
	}
}	

reader::reader(std::string filename) {
	
	std::ifstream in;
	in.open(filename);
	
	if (!in) {
		throw std::runtime_error("Input file not found.");
	}
	
	json data;
	
	in >> data;
	
	validate(data, valid_keys);
	
	json& jmol = data["molecule"];
	
	auto atoms = get_geometry(jmol);
	
	std::vector<libint2::Shell> basis;
	if (jmol.find("basis") != jmol.end()) {	
		libint2::BasisSet bas(jmol["basis"], atoms);
		basis = std::move(bas);
	} else { 
		basis = read_basis(jmol["gen_basis"],atoms);
	}
	
	optional<std::vector<libint2::Shell>,val> dfbasis;
	/*if (jmol.find("dfbasis") != jmol.end()) {	
		libint2::BasisSet bas(jmol["basis"], atoms);
		
		dfbasis = std::move(bas);
	} else { 
		basis = read_basis(jmol["gen_basis"],atoms);
	}
	
	
	if (jmol.find("gen_dfbasis") != jmol.end()) {
		 optional<std::vector<libint2::Shell>,val> temp(
			read_basis(jmol["gen_dfbasis"],atoms));
		dfbasis = std::move(temp);
	}*/
	
	int charge = jmol["charge"];
	int mult = jmol["mult"];
	
	desc::molecule mol({.atoms = atoms, .charge = charge,
		.mult = mult, .split = 20, .basis = basis, .dfbasis = dfbasis});
		
	mol.print_info(1);
	
	desc::options opt;
	
	unpack(data, opt, "hf");
	
	std::cout << opt.get<bool>("hf/diis") << std::endl;
	std::cout << opt.get<double>("hf/conv") << std::endl;
	
	m_mol = mol;
	m_opt = opt;
	
}
