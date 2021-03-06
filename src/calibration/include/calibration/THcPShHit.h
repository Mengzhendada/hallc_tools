#ifndef hallc_calibration_shms_shower_hit_HH
#define hallc_calibration_shms_shower_hit_HH

#include "TObject.h"
#include <iostream>

namespace hallc {
  namespace calibration {

    /** SHMS calorimeter hit class for calibration.
     */
    class THcPShHit {
    public:

      Double_t ADC;       // pedestal subtracted ADC signal.
      Double_t Edep;      // Energy deposition.
      UInt_t   BlkNumber; // Block number.

      THcPShHit();
      THcPShHit(Double_t adc, UInt_t blk_number);
      virtual ~THcPShHit();

      void SetADC(Double_t sig) { ADC = sig; }

      void SetEdep(Double_t e) { Edep = e; }

      void SetBlkNumber(UInt_t n) { BlkNumber = n; }

      Double_t GetADC() { return ADC; }

      Double_t GetEdep() { return Edep; }

      UInt_t GetBlkNumber() { return BlkNumber; }

      void Print(std::ostream& ostrm);
    };

    struct pmt_hit {
      Double_t signal;
      UInt_t   channel;
    };


  } // namespace calibration
} // namespace hallc
#endif

