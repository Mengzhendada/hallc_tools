#include "calibration/CalorimeterCalibration.h"

#include <cmath>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstdlib>
#include <array>
#ifdef __cpp_lib_filesystem
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

#include "nlohmann/json.hpp"

#include "TObject.h"

namespace hallc {
  namespace calibration {

    CalorimeterCalibration::CalorimeterCalibration(int rn) : run_number(rn) {}

    std::string CalorimeterCalibration::PrepareJson() const {
      nlohmann::json j;
      j["thresholds"]["delta_low"]  = fDeltaMin;
      j["thresholds"]["delta_high"] = fDeltaMax;
      j["thresholds"]["beta_low"]  = fBetaMin;
      j["thresholds"]["beta_high"] = fBetaMax;
      j["thresholds"]["min_hits"]  = fMinHitCount;
      j["thresholds"]["hgc_min"]   = fHGCerMin;
      j["thresholds"]["ngc_min"]   = fNGCerMin;
      j["cal"]["neg_gain_cor"]     = neg_gain_cor;
      j["cal"]["pos_gain_cor"]     = pos_gain_cor;
      j["cal"]["arr_gain_cor"]     = arr_gain_cor;
      return j.dump();
    }

    void CalorimeterCalibration::BuildTester(int run_num) const {
      nlohmann::json j;
      // nlohmann::json a_run;
      std::string a_run          = PrepareJson();
      j["0"]                     = nlohmann::json::parse(a_run);
      j["1111"]                  = nlohmann::json::parse(a_run);
      j["2222"]                  = nlohmann::json::parse(a_run);
      j["3333"]                  = nlohmann::json::parse(a_run);
      j[std::to_string(run_num)] = nlohmann::json::parse(a_run);

      std::ofstream json_out_file("test.json");
      json_out_file << j.dump(1) << std::endl;
    }

    void CalorimeterCalibration::LoadJsonCalibration(const std::string& fname, int rn) {
      using nlohmann::json;
      std::ifstream json_input_file(fname);
      //std::istream& in_stream = is_piped ? std::cin : jinput;
      nlohmann::json j_database;
      json_input_file >> j_database;

      std::cout << " runs : ";
      std::vector<int> runs;
      for( json::iterator it = j_database.begin(); it != j_database.end(); ++it ) {
        std::cout << it.key() << ", " ;
        runs.push_back(std::stoi(it.key()));
      }
      std::cout << "\n";

      auto closest = [](std::vector<int> const& vec, int value) {
        auto it = std::lower_bound(vec.begin(), vec.end(), value);
        if(*it > value) { it = it-1 ; }
        if (it == vec.end()) {
          return -1;
        }
        return *it;
      };
      //auto best_run = std::upper_bound(runs.begin(), runs.end(), rn);
      //std::cout << "using run upper " << rn << " :::: " << *best_run << "\n";
      //auto best_run = std::lower_bound(runs.begin(), runs.end(), rn);
      //std::cout << "using run lower " << rn << " :::: " << *(best_run) << "\n";

      int best_run = closest(runs, rn);
      std::cout << " clostest " << best_run << std::endl;
      json  j = j_database[std::to_string(best_run)];
      //std::cout << j_database[std::to_string(best_run)] << "\n";;
      std::cout << j.dump(-1) << "\n";

      if (j.find("thresholds") != j.end()) {
        std::cout << j["thresholds"] << "\n";
        json j_thresh = j["thresholds"];
        if (j_thresh.find("delta_low") != j_thresh.end()) {
          fDeltaMin = j_thresh["delta_low"].get<double>();
        } 
        if (j_thresh.find("delta_high") != j_thresh.end()) {
          fDeltaMax = j_thresh["delta_high"].get<double>();
        }
        if (j_thresh.find("beta_low") != j_thresh.end()) {
          fBetaMin = j["thresholds"]["beta_low"].get<double>();
        }
        if (j_thresh.find("beta_high") != j_thresh.end()) {
          fBetaMax = j["thresholds"]["beta_high"].get<double>();
        }
        if (j_thresh.find("min_hits") != j_thresh.end()) {
          fMinHitCount = j["thresholds"]["min_hits"].get<double>();
        }
        if (j_thresh.find("hgc_min") != j_thresh.end()) {
          fHGCerMin = j["thresholds"]["hgc_min"].get<double>();
        }
        if (j_thresh.find("ngc_min") != j_thresh.end()) {
          fNGCerMin = j["thresholds"]["ngc_min"].get<double>();
        }
      } else {
        std::cout << " thresholds not found !!! \n";
      }

      if (j.find("histogram") != j.end()) {
      }
      if (j.find("cal") != j.end()) {
        if (j.find("neg_gain_cor") != j.end()) {
          neg_gain_cor = j["cal"]["neg_gain_cor"].get<std::vector<double>>();
        }
        if (j.find("pos_gain_cor") != j.end()) {
          pos_gain_cor = j["cal"]["pos_gain_cor"].get<std::vector<double>>();
        }
        if (j.find("arr_gain_cor") != j.end()) {
          arr_gain_cor = j["cal"]["arr_gain_cor"].get<std::vector<double>>();
        }
      }
    }

    void CalorimeterCalibration::ReadLegacyCalibration(const std::string& fname) {
      // Read in threshold parameters and initial gains.
      // File is assumed to have the following structure:
      // -10 22	Delta range, %
      // 0.5 1.5	Beta range
      // 2.  	Heavy Gas Cherenkov, threshold on signals in p.e.
      // 0.  	Noble Gas Cherenkov, threshold on signals in p.e.
      // 5	Minimum number of hits per channel required to be calibrated
      // 0. 3.	Range of uncalibrated energy deposition histogram
      // 500	Binning of uncalibrated energy deposition histogram
      // 0.5 2.	Gaussian fit range around e- peak in the uncalibrated Edep histogram
      // (blank line)
      //; Calibration constants for run 1791_300000, 38067 events processed (dec. 17 defocused run)
      // (blank line)
      //pcal_neg_gain_cor =  ( 14 numbers , comma separated)
      //pcal_pos_gain_cor = ( 14 numbers , comma separated)
      //pcal_arr_gain_cor = ( 16 x 14  comma separted values) 

      using namespace std;
      fDeltaMin    = 0.;
      fDeltaMax    = 0.;
      fBetaMin     = 0.;
      fBetaMax     = 0.;
      fHGCerMin    = 999.;
      fNGCerMin    = 999.;
      fMinHitCount = 999999;

      for (UInt_t ipmt = 0; ipmt < fNpmts; ipmt++) {
        falpha0[ipmt] = 0.;
      }

      /// \todo add file checks
      ifstream fin(fname);

      string        line;
      istringstream iss;

      getline(fin, line);
      iss.str(line);
      iss >> fDeltaMin >> fDeltaMax;
      getline(fin, line);
      iss.str(line);
      iss >> fBetaMin >> fBetaMax;
      getline(fin, line);
      iss.str(line);
      iss >> fHGCerMin;
      getline(fin, line);
      iss.str(line);
      iss >> fNGCerMin;
      getline(fin, line);
      iss.str(line);
      iss >> fMinHitCount;
      getline(fin, line);
      iss.str(line);
      iss >> fEuncLoLo >> fEuncHiHi;
      getline(fin, line);
      iss.str(line);
      iss >> fEuncNBin;
      getline(fin, line);
      iss.str(line);
      iss >> fEuncGFitLo >> fEuncGFitHi;

      getline(fin, line);
      getline(fin, line);
      getline(fin, line);

      unsigned iblk = 0;

      // Preshower
      //for (unsigned k = 0; k < fNcols_pr; k++) {
        for (unsigned j = 0; j < fNrows_pr; j++) {

          getline(fin, line, ',');
          //	cout << "line=" << line << endl;
          iss.str(line);
          if (j == 0) {
            string name;
            iss >> name >> name >> falpha0[iblk];
          } else {
            iss >> falpha0[iblk];
          }
          neg_gain_cor.push_back(falpha0[iblk]);

          iss.clear();
          iblk++;
        }

        for (unsigned j = 0; j < fNrows_pr; j++) {

          getline(fin, line, ',');
          //	cout << "line=" << line << endl;
          iss.str(line);
          if (j == 0) {
            string name;
            iss >> name >> name >> falpha0[iblk];
          } else {
            iss >> falpha0[iblk];
          }
          pos_gain_cor.push_back(falpha0[iblk]);

          iss.clear();
          iblk++;
        }
      //}

      // Shower
      for (unsigned k = 0; k < fNcols_sh; k++) {
        for (unsigned j = 0; j < fNrows_sh; j++) {

          getline(fin, line, ',');
          //	cout << "line=" << line << endl;
          iss.str(line);
          if (k == 0 && j == 0) {
            string name;
            iss >> name >> name >> falpha0[iblk];
          } else {
            iss >> falpha0[iblk];
          }

          arr_gain_cor.push_back(falpha0[iblk]);
          iss.clear();
          iblk++;
        }
      }

      fin.close();

      cout << "=================================================================\n";
      cout << "Thresholds:" << endl;
      cout << "  Delta min, max   = " << fDeltaMin << "  " << fDeltaMax << endl;
      cout << "  Beta min, max    = " << fBetaMin << "  " << fBetaMax << endl;
      cout << "  Heavy Gas Cerenkov min = " << fHGCerMin << endl;
      cout << "  Noble Gas Cerenkov min = " << fNGCerMin << endl;
      cout << "  Min. hit count   = " << fMinHitCount << endl;
      cout << "  Uncalibrated histo. range and binning: " << fEuncLoLo << "  " << fEuncHiHi << "  "
           << fEuncNBin << endl;
      cout << "  Uncalibrated histo. fit range: " << fEuncGFitLo << "  " << fEuncGFitHi << endl;
      cout << endl;

      cout << "Initial gain constants:\n";

      UInt_t j = 0;
      for (UInt_t k = 0; k < fNcols_pr; k++) {
        k == 0 ? cout << "pcal_neg_gain_cor =" : cout << "pcal_pos_gain_cor =";
        for (UInt_t i = 0; i < fNrows_pr; i++)
          cout << fixed << setw(6) << setprecision(2) << falpha0[j++] << ",";
        cout << endl;
      }
      for (UInt_t k = 0; k < fNcols_sh; k++) {
        k == 0 ? cout << "pcal_arr_gain_cor =" : cout << "                   ";
        for (UInt_t i = 0; i < fNrows_sh; i++)
          cout << fixed << setw(6) << setprecision(2) << falpha0[j++] << ",";
        cout << endl;
      }

      cout << "=================================================================\n";

      //  getchar();
    }
  }
} // namespace hallc
