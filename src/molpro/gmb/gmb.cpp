#include "gmb.h"
#include "amplitudes.h"
#include "expressions/energy_hf.h"
#include "hamiltonian.h"
#include "init.h"
#include "run_method.h"
#include "hamiltonian.h"
#include "amplitudes.h"
#include "expressions/energy_hf.h"
#include "utils.h"
#include <chrono>
#include <ctime>
#include <memory>
#include <molpro/Options.h>
#include <molpro/iostream.h>
#include <molpro/linalg/itsolv/IterativeSolver.h>
#include <molpro/linalg/itsolv/SolverFactory.h>
#include <regex>

using namespace gmb;

extern molpro::Profiler prof;
extern "C" void general_many_body(int64_t &nstate, double *energies) {
  auto ev = molpro::gmb::gmb();
  nstate = ev.size();
  for (size_t i = 0; i < nstate; ++i)
    energies[i] = ev[i];
}

std::vector<double> molpro::gmb::gmb(const molpro::Options &options) {

  std::vector<double> energies;

  std::string filename = options.parameter("dump", std::string{""});
  auto expected_results = options.parameter("results", std::vector<double>{});
  std::vector<bool> found_expected_results(expected_results.size(), false);

  std::ios_base::sync_with_stdio(false);
  auto start = std::chrono::system_clock::now();

  // parse arguments
  auto method = options.parameter("method", "eom-ccsd");
  auto nroots = options.parameter("states", 3);
  auto ncav = options.parameter("polariton_modes", 0);

  std::vector<std::shared_ptr<polariton>> v_ppol(ncav);
  if (ncav > 0) {
    std::vector<int> v_option_polariton_nmax(ncav, 1);
    v_option_polariton_nmax =
        options.parameter("polariton_nmax", v_option_polariton_nmax);
    std::vector<double> v_option_polariton_gamma(ncav, 0.01);
    v_option_polariton_gamma =
        options.parameter("polariton_gamma", v_option_polariton_gamma);
    std::vector<double> v_option_polariton_omega(ncav, 1.028);
    v_option_polariton_omega =
        options.parameter("polariton_omega", v_option_polariton_omega);
    for (size_t i = 0; i < ncav; i++) {
      v_ppol[i] = std::make_shared<polariton>(v_option_polariton_nmax[i],
                                              v_option_polariton_gamma[i],
                                              v_option_polariton_omega[i]);
      v_ppol[i]->fname_dm = options.parameter(
          "dipole",
          std::regex_replace(filename, std::regex{"\\.[_[:alnum:]]*$"}, ".dm"));
      v_ppol[i]->fname_sm =
          std::regex_replace(filename, std::regex{"\\.[_[:alnum:]]*$"}, ".sm");
    }
  }

  // print arguments
  molpro::cout << "Required calculation: "
               << "\n\n";
  check_file(filename, "fcidump");
  molpro::cout << " fcidump = " << filename << "\n";
  molpro::cout << " method = " << method << "\n";
  molpro::cout << " roots = " << nroots << "\n";
  if (v_ppol.size() > 0) {
    check_file(v_ppol[0]->fname_dm, "dipole");
    molpro::cout << " dipole file = " << v_ppol[0]->fname_dm << "\n";
    check_file(v_ppol[0]->fname_sm, "second moment of charges");
    molpro::cout << " second moment of charges file = " << v_ppol[0]->fname_sm
                 << "\n";

    molpro::cout << "\nPolariton parameters: \n";
    molpro::cout << "modes: " << ncav << "\n";
    for (size_t i = 0; i < ncav; i++) {
      molpro::cout << "\n mode " << i << "\n";
      molpro::cout << " nmax = " << v_ppol[i]->nmax << "\n";
      molpro::cout << " gamma = " << v_ppol[i]->gamma << "\n";
      molpro::cout << " omega = " << v_ppol[i]->omega << "\n\n";
    }
  }

  hamiltonian<> ham;
  std::unique_ptr<amplitudes<>> ptampl{std::make_unique<amplitudes<>>()};
  std::unique_ptr<problem_gen> problem;
  std::string method_gs, method_es;

  // initialise hamiltonian
  init(filename, method, ham, v_ppol);

  auto vnn = get_integral(filename);
  auto hf_energy = vnn + energy_hf(ham.m2get(f_oo),ham.m4get(i_oooo));
  #if 1 // self-energy
  for (size_t i = 0; i < ncav; i++) {
    auto rnuc = get_integral(v_ppol[i]->fname_dm);
    molpro::cout << " rnuc = " << rnuc << "\n";
    hf_energy += v_ppol[i]->gamma*v_ppol[i]->gamma*v_ppol[i]->omega*rnuc*rnuc;
  }
  #endif
  molpro::cout << "\nHF energy: " << std::setprecision(12) << hf_energy << "\n\n";

#if 1 // CCSD
  if (!(method.find("hf") != std::string::npos)) {
    run_gs(ham, method, problem, ptampl);
    // print results
    molpro::cout << *problem << " correlation energy: " << std::setprecision(12) << problem->get_energy()<< "\n";
    double ccsd_energy = problem->get_energy() + hf_energy;
    molpro::cout << *problem  << " total energy: " << std::setprecision(13)
              << ccsd_energy << "\n";
    for (int i=0; i<expected_results.size(); ++i)
      if (std::abs(problem->get_energy()+hf_energy-expected_results[i])<1e-10) found_expected_results[i]=true;

#if 1 // Excited State
    if (method.find("eom") != std::string::npos) {

      std::unique_ptr<problem_eom> problem_es;
      run_es(ham, method, problem_es, ptampl, nroots);
      for (const auto& ev : problem_es->get_energy())
        for (int i=0; i<expected_results.size(); ++i)
          if (std::abs(ev-expected_results[i])<1e-10) found_expected_results[i]=true;

      auto energies = problem_es->get_energy();
      // print results
      molpro::cout << "\n       Excitation energy                     Total energy  \n";
      molpro::cout << "       (Ha)        (eV)                    (Ha)        (eV)  \n";
      for (auto &i : energies)
        molpro::cout << std::setw(14) << std::setprecision(7) << i << "   "
                  << std::setw(14) << i*27.2114 << "   "
                  << std::setw(14) << ccsd_energy+i << "    "
                  << std::setw(14) << (ccsd_energy+i)*27.2114 << " \n";
    }
  #endif
  }
  #endif

  auto end = std::chrono::system_clock::now();
  std::chrono::duration<double> elapsed_seconds = end - start;
  std::time_t end_time = std::chrono::system_clock::to_time_t(end);
  molpro::cout << "\nFinished computation at " << std::ctime(&end_time)
               << "Elapsed time: " << elapsed_seconds.count() << "s"
               << std::endl;
  for (int i = 0; i < expected_results.size(); ++i)
    if (not found_expected_results[i])
      throw std::runtime_error("Did not match expected result " +
                               std::to_string(expected_results[i]));
  return energies;
}



#include <molpro/linalg/itsolv/SolverFactory-implementation.h>
template class molpro::linalg::itsolv::SolverFactory<amplitudes<>, amplitudes<>>;
