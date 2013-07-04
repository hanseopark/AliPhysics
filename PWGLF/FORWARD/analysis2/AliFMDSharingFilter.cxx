//
// Class to do the sharing correction.  That is, a filter that merges 
// adjacent strip signals presumably originating from a single particle 
// that impinges on the detector in such a way that it deposite energy 
// into two or more strips. 
//
// Input: 
//    - AliESDFMD object  - from reconstruction
//
// Output: 
//    - AliESDFMD object  - copy of input, but with signals merged 
//
// Corrections used: 
//    - AliFMDCorrELossFit
//
// Histograms: 
//    - For each ring (FMD1i, FMD2i, FMD2o, FMD3i, FMD3o) the distribution of 
//      signals before and after the filter.  
//    - For each ring (see above), an array of distributions of number of 
//      hit strips for each vertex bin (if enabled - see Init method)
// 
//
//
#include "AliFMDSharingFilter.h"
#include "AliFMDStripIndex.h"
#include <AliESDFMD.h>
#include <TAxis.h>
#include <TList.h>
#include <TH1.h>
#include <TMath.h>
#include "AliForwardCorrectionManager.h"
#include "AliFMDCorrELossFit.h"
#include <AliLog.h>
#include <TROOT.h>
#include <THStack.h>
#include <TParameter.h>
#include <iostream>
#include <iomanip>

ClassImp(AliFMDSharingFilter)
#if 0
; // This is for Emacs
#endif 

#define DBG(L,M)							\
  do { if (L>fDebug)break; std::cout << (M) << std::flush;} while(false) 
#define DBGL(L,M)							\
  do { if (L>fDebug)break; std::cout << (M) << std::endl;} while(false) 
			      


//____________________________________________________________________
AliFMDSharingFilter::AliFMDSharingFilter()
  : TNamed(), 
    fRingHistos(),
    fCorrectAngles(kFALSE), 
    // fSummed(0),
    fHighCuts(0),
    fLowCuts(0),
    // fOper(0),
    fDebug(0),
    fZeroSharedHitsBelowThreshold(false),
    fLCuts(),
    fHCuts(),
    fUseSimpleMerging(false),
    fThreeStripSharing(true),
    fRecalculateEta(false),
    // fExtraDead(0),
    fXtraDead(0),
    fInvalidIsEmpty(false)
{
  // 
  // Default Constructor - do not use 
  //
  DGUARD(fDebug,1, "Default CTOR for AliFMDSharingFilter");
}

//____________________________________________________________________
AliFMDSharingFilter::AliFMDSharingFilter(const char* title)
  : TNamed("fmdSharingFilter", title), 
    fRingHistos(), 
    fCorrectAngles(kFALSE), 
    // fSummed(0),
    fHighCuts(0),
    fLowCuts(0),
    // fOper(0),
    fDebug(0),
    fZeroSharedHitsBelowThreshold(false),
    fLCuts(),
    fHCuts(),
    fUseSimpleMerging(false),
    fThreeStripSharing(true),
    fRecalculateEta(false), 
    // fExtraDead(51200),
    fXtraDead(AliFMDStripIndex::Pack(3,'O',19,511)+1),
    fInvalidIsEmpty(false)
{
  // 
  // Constructor 
  // 
  // Parameters:
  //    title Title of object  - not significant 
  //
  DGUARD(fDebug,1, "Named CTOR for AliFMDSharingFilter: %s", title);
  fRingHistos.SetName(GetName());
  fRingHistos.SetOwner();
  
  fRingHistos.Add(new RingHistos(1, 'I'));
  fRingHistos.Add(new RingHistos(2, 'I'));
  fRingHistos.Add(new RingHistos(2, 'O'));
  fRingHistos.Add(new RingHistos(3, 'I'));
  fRingHistos.Add(new RingHistos(3, 'O'));

  fHCuts.SetNXi(1);
  fHCuts.SetIncludeSigma(1);
  fLCuts.SetMultCuts(.15);

  // fExtraDead.Reset(-1);
}

//____________________________________________________________________
AliFMDSharingFilter::AliFMDSharingFilter(const AliFMDSharingFilter& o)
  : TNamed(o), 
    fRingHistos(), 
    fCorrectAngles(o.fCorrectAngles), 
    // fSummed(o.fSummed),
    fHighCuts(o.fHighCuts),
    fLowCuts(o.fLowCuts),
    // fOper(o.fOper),
    fDebug(o.fDebug),
    fZeroSharedHitsBelowThreshold(o.fZeroSharedHitsBelowThreshold),
    fLCuts(o.fLCuts),
    fHCuts(o.fHCuts),
    fUseSimpleMerging(o.fUseSimpleMerging),
    fThreeStripSharing(o.fThreeStripSharing),
    fRecalculateEta(o.fRecalculateEta), 
    //fExtraDead(o.fExtraDead),
    fXtraDead(o.fXtraDead),
    fInvalidIsEmpty(o.fInvalidIsEmpty)
{
  // 
  // Copy constructor 
  // 
  // Parameters:
  //    o Object to copy from 
  //
  DGUARD(fDebug,1, "Copy CTOR for AliFMDSharingFilter");
  TIter    next(&o.fRingHistos);
  TObject* obj = 0;
  while ((obj = next())) fRingHistos.Add(obj);
}

//____________________________________________________________________
AliFMDSharingFilter::~AliFMDSharingFilter()
{
  // 
  // Destructor
  //
  DGUARD(fDebug,3, "DTOR for AliFMDSharingFilter");
  // fRingHistos.Delete();
}

//____________________________________________________________________
AliFMDSharingFilter&
AliFMDSharingFilter::operator=(const AliFMDSharingFilter& o)
{
  // 
  // Assignment operator 
  // 
  // Parameters:
  //    o Object to assign from 
  // 
  // Return:
  //    Reference to this 
  //
  DGUARD(fDebug,3, "Assigment for AliFMDSharingFilter");
  if (&o == this) return *this;
  TNamed::operator=(o);

  fCorrectAngles                = o.fCorrectAngles;
  fDebug                        = o.fDebug;
  // fOper                         = o.fOper;
  // fSummed                       = o.fSummed;
  fHighCuts                     = o.fHighCuts;
  fLowCuts                      = o.fLowCuts;
  fZeroSharedHitsBelowThreshold = o.fZeroSharedHitsBelowThreshold;
  fLCuts                        = o.fLCuts;
  fHCuts                        = o.fHCuts;
  fUseSimpleMerging             = o.fUseSimpleMerging;
  fThreeStripSharing            = o.fThreeStripSharing;
  fRecalculateEta               = o.fRecalculateEta;
  fInvalidIsEmpty               = o.fInvalidIsEmpty;
  
  fRingHistos.Delete();
  TIter    next(&o.fRingHistos);
  TObject* obj = 0;
  while ((obj = next())) fRingHistos.Add(obj);
  
  return *this;
}

//____________________________________________________________________
AliFMDSharingFilter::RingHistos*
AliFMDSharingFilter::GetRingHistos(UShort_t d, Char_t r) const
{
  // 
  // Get the ring histogram container 
  // 
  // Parameters:
  //    d Detector
  //    r Ring 
  // 
  // Return:
  //    Ring histogram container 
  //
  Int_t idx = -1;
  switch (d) { 
  case 1: idx = 0; break;
  case 2: idx = 1 + (r == 'I' || r == 'i' ? 0 : 1); break;
  case 3: idx = 3 + (r == 'I' || r == 'i' ? 0 : 1); break;
  }
  if (idx < 0 || idx >= fRingHistos.GetEntries()) return 0;
  
  return static_cast<RingHistos*>(fRingHistos.At(idx));
}

//____________________________________________________________________
void
AliFMDSharingFilter::AddDead(UShort_t d, Char_t r, UShort_t s, UShort_t t)
{
  if (d < 1 || d > 3) {
    Warning("AddDead", "Invalid detector FMD%d", d);
    return;
  }
  Bool_t inner = (r == 'I' || r == 'i');
  if (d == 1 && !inner) { 
    Warning("AddDead", "Invalid ring FMD%d%c", d, r);
    return;
  }
  if ((inner && s >= 20) || (!inner && s >= 40)) { 
    Warning("AddDead", "Invalid sector FMD%d%c[%02d]", d, r, s);
    return;
  }
  if ((inner && t >= 512) || (!inner && t >= 256)) { 
    Warning("AddDead", "Invalid strip FMD%d%c[%02d,%03d]", d, r, s, t);
    return;
  }
    
  Int_t id = AliFMDStripIndex::Pack(d, r, s, t);
  // Int_t i  = 0;
  fXtraDead.SetBitNumber(id, true);
#if 0
  for (i = 0; i < fExtraDead.GetSize(); i++) {
    Int_t j = fExtraDead.At(i);
    if (j == id) return; // Already there 
    if (j <  0) break; // Free slot 
  }
  if (i >= fExtraDead.GetSize()) { 
    Warning("AddDead", "No free slot to add FMD%d%c[%02d,%03d] at", 
	    d, r, s, t);
    return;
  }
  fExtraDead[i] = id;
#endif
}
//____________________________________________________________________
void
AliFMDSharingFilter::AddDeadRegion(UShort_t d,  Char_t r, 
				   UShort_t s1, UShort_t s2, 
				   UShort_t t1, UShort_t t2)
{
  // Add a dead region spanning from FMD<d><r>[<s1>,<t1>] to 
  // FMD<d><r>[<s2>,<t2>] (both inclusive)
  for (Int_t s = s1; s <= s2; s++) 
    for (Int_t t = t1; t <= t2; t++) 
      AddDead(d, r, s, t);
}
//____________________________________________________________________
void
AliFMDSharingFilter::AddDead(const Char_t* script)
{
  if (!script || script[0] == '\0') return;
  
  gROOT->Macro(Form("%s((AliFMDSharingFilter*)%p);", script, this));
}

//____________________________________________________________________
Bool_t
AliFMDSharingFilter::IsDead(UShort_t d, Char_t r, UShort_t s, UShort_t t) const
{
  Int_t id = AliFMDStripIndex::Pack(d, r, s, t);
  return fXtraDead.TestBitNumber(id); 
#if 0
  for (Int_t i = 0; i < fExtraDead.GetSize(); i++) {
    Int_t j = fExtraDead.At(i);
    if (j == id) {
      //Info("IsDead", "FMD%d%c[%02d,%03d] marked as dead here", d, r, s, t);
      return true;
    }
    if (j < 0) break; // High water mark 
  }
  return false;
#endif
}
//____________________________________________________________________
void
AliFMDSharingFilter::SetupForData(const TAxis& axis)
{
  // Initialise - called on first event
  DGUARD(fDebug,1, "Initialize for AliFMDSharingFilter");
  AliForwardCorrectionManager& fcm  = AliForwardCorrectionManager::Instance();
  const AliFMDCorrELossFit*    fits = fcm.GetELossFit();

  // Compactify the xtra dead bits 
  fXtraDead.Compact();

  // Get the high cut.  The high cut is defined as the 
  // most-probably-value peak found from the energy distributions, minus 
  // 2 times the width of the corresponding Landau.
  
  TAxis eAxis(axis.GetNbins(),
	      axis.GetXmin(),
	      axis.GetXmax());
  if(fits) 
    eAxis.Set(fits->GetEtaAxis().GetNbins(), 
	      fits->GetEtaAxis().GetXmin(),
	      fits->GetEtaAxis().GetXmax());

  UShort_t nEta = eAxis.GetNbins();
	
  fHighCuts->SetBins(nEta, eAxis.GetXmin(), eAxis.GetXmax(), 5, .5, 5.5);
  fHighCuts->GetYaxis()->SetBinLabel(1, "FMD1i");
  fHighCuts->GetYaxis()->SetBinLabel(2, "FMD2i");
  fHighCuts->GetYaxis()->SetBinLabel(3, "FMD2o");
  fHighCuts->GetYaxis()->SetBinLabel(4, "FMD3i");
  fHighCuts->GetYaxis()->SetBinLabel(5, "FMD3o");

  fLowCuts->SetBins(nEta, eAxis.GetXmin(), eAxis.GetXmax(), 5, .5, 5.5);
  fLowCuts->GetYaxis()->SetBinLabel(1, "FMD1i");
  fLowCuts->GetYaxis()->SetBinLabel(2, "FMD2i");
  fLowCuts->GetYaxis()->SetBinLabel(3, "FMD2o");
  fLowCuts->GetYaxis()->SetBinLabel(4, "FMD3i");
  fLowCuts->GetYaxis()->SetBinLabel(5, "FMD3o");

  UShort_t ybin = 0;
  for (UShort_t d = 1; d <= 3; d++) {
    UShort_t nr = (d == 1 ? 1 : 2);
    for (UShort_t q = 0; q < nr; q++) { 
      Char_t r = (q == 0 ? 'I' : 'O');
      ybin++;
      for (UShort_t e = 1; e <= nEta; e++) { 
	Double_t eta = eAxis.GetBinCenter(e);
	
	if (fDebug > 3) fHCuts.Print();

	Double_t hcut = GetHighCut(d, r, eta, false);
	Double_t lcut = GetLowCut(d, r, eta);
	
	if (hcut > 0) fHighCuts->SetBinContent(e, ybin, hcut);
	if (lcut > 0) fLowCuts ->SetBinContent(e, ybin, lcut);
      }
    }
  }
}

//____________________________________________________________________
#define ETA2COS(ETA)						\
  TMath::Cos(2*TMath::ATan(TMath::Exp(-TMath::Abs(ETA))))

Bool_t
AliFMDSharingFilter::Filter(const AliESDFMD& input, 
			    Bool_t           /*lowFlux*/,
			    AliESDFMD&       output, 
			    Double_t         zvtx)
{
  // 
  // Filter the input AliESDFMD object
  // 
  // Parameters:
  //    input     Input 
  //    lowFlux   If this is a low-flux event 
  //    output    Output AliESDFMD object 
  // 
  // Return:
  //    True on success, false otherwise 
  //
  DGUARD(fDebug,1, "Filter event in AliFMDSharingFilter");
  output.Clear();
  TIter    next(&fRingHistos);
  RingHistos* o      = 0;
  while ((o = static_cast<RingHistos*>(next()))) o->Clear();

  Int_t nSingle    = 0;
  Int_t nDouble    = 0;
  Int_t nTriple    = 0;

  for(UShort_t d = 1; d <= 3; d++) {
    Int_t nRings = (d == 1 ? 1 : 2);
    for (UShort_t q = 0; q < nRings; q++) {
      Char_t      r      = (q == 0 ? 'I' : 'O');
      UShort_t    nsec   = (q == 0 ?  20 :  40);
      UShort_t    nstr   = (q == 0 ? 512 : 256);
      RingHistos* histos = GetRingHistos(d, r);
      
      for(UShort_t s = 0; s < nsec;  s++) {	
	// `used' flags if the _current_ strip was used by _previous_ 
	// iteration. 
	Bool_t   used            = kFALSE;
	// `eTotal' contains the current sum of merged signals so far 
	Double_t eTotal          = -1;
	// Int_t    nDistanceBefore = -1;
	// Int_t    nDistanceAfter  = -1;
	// `twoLow' flags if we saw two consequtive strips with a 
	// signal between the two cuts. 
	Bool_t   twoLow          = kFALSE;
	for(UShort_t t = 0; t < nstr; t++) {
	  // nDistanceBefore++;
	  // nDistanceAfter++;
	  
	  output.SetMultiplicity(d,r,s,t,0.);
	  Float_t mult         = SignalInStrip(input,d,r,s,t);
	  Float_t multNext     = (t<nstr-1) ? SignalInStrip(input,d,r,s,t+1) :0;
	  Float_t multNextNext = (t<nstr-2) ? SignalInStrip(input,d,r,s,t+2) :0;
	  if (multNext     ==  AliESDFMD::kInvalidMult) multNext     = 0;
	  if (multNextNext ==  AliESDFMD::kInvalidMult) multNextNext = 0;
	  if(!fThreeStripSharing) multNextNext = 0;

	  // Get the pseudo-rapidity 
	  Double_t eta = input.Eta(d,r,s,t);
	  Double_t phi = input.Phi(d,r,s,t) * TMath::Pi() / 180.;
	  if (s == 0) output.SetEta(d,r,s,t,eta);
	  
	  if(fRecalculateEta) { 
	    Double_t etaOld  = eta;
	    Double_t etaCalc = AliForwardUtil::GetEtaFromStrip(d,r,s,t,zvtx);
	    eta              = etaCalc;
	    
	    if (mult > 0 && mult != AliESDFMD::kInvalidMult ) {
	      Double_t cosOld =  ETA2COS(etaOld);
	      Double_t cosNew =  ETA2COS(etaCalc);
	      Double_t corr   =  cosNew / cosOld;
	      mult            *= corr;
	      multNext        *= corr;
	      multNextNext    *= corr;
	    }
	  }

	  // Special case for pre revision 43611 AliFMDReconstructor.
	  // If fInvalidIsEmpty and we get an invalid signal from the
	  // ESD, then we need to set this signal to zero.  Note, dead
	  // strips added in the ForwardAODConfig.C file are not
	  // effected by this, and we can use that to set the proper
	  // dead strips.
	  if (mult == AliESDFMD::kInvalidMult && fInvalidIsEmpty) 
	    mult = 0;

	  // Keep dead-channel information - either from the ESD (but
	  // see above for older data) or from the settings in the
	  // ForwardAODConfig.C file.
	  if (mult == AliESDFMD::kInvalidMult || IsDead(d,r,s,t)) {
	    output.SetMultiplicity(d,r,s,t,AliESDFMD::kInvalidMult);
	    histos->fBefore->Fill(-1);
	    mult = AliESDFMD::kInvalidMult;
	  }
	  
	  // If no signal or dead strip, go on. 
	  if (mult == AliESDFMD::kInvalidMult || mult == 0) {
	    if (mult == 0) histos->fSum->Fill(eta,phi,mult);
	    // Flush a possible signal 
	    if (eTotal > 0 && t > 0) 
	      output.SetMultiplicity(d,r,s,t-1,eTotal);
	    // Reset states so we do not try to merge over a dead strip. 
	    eTotal = -1;
	    used   = false;
	    twoLow = false;
	    continue;
	  }

	  // Fill the diagnostics histogram 
	  histos->fBefore->Fill(mult);
	  
	  Double_t mergedEnergy = 0;
	  
	  // The current sum
	  Float_t etot = 0;
	  
	  // Fill in neighbor information
	  if (t < nstr-1) histos->fNeighborsBefore->Fill(mult,multNext);

	  Bool_t thisValid = mult     > GetLowCut(d, r, eta);
	  Bool_t nextValid = multNext > GetLowCut(d, r, eta);
	  Bool_t thisSmall = mult     < GetHighCut(d, r, eta ,false);
	  Bool_t nextSmall = multNext < GetHighCut(d, r, eta ,false);
	  
	  // If this strips signal is above the high cut, reset distance
	  // if (!thisSmall) {
	  //    histos->fDistanceBefore->Fill(nDistanceBefore);
	  //    nDistanceBefore = -1;
	  // }
	  
	  // If the total signal in the past 1 or 2 strips are non-zero
	  // we need to check 
	  if (eTotal > 0) {
	    // Here, we have already flagged one strip as a candidate 
	    
	    // If 3-strip merging is enabled, then check the next 
	    // strip to see that it falls within cut, or if we have 
	    // two low signals 
	    if (fThreeStripSharing && nextValid && (nextSmall || twoLow)) {
	      eTotal = eTotal + multNext;
	      used = kTRUE;
	      histos->fTriple->Fill(eTotal);
	      nTriple++;
	      twoLow = kFALSE;
	    }
	    // Otherwise, we got a double hit before, and that 
	    // should be stored. 
	    else {
	      used = kFALSE;
	      histos->fDouble->Fill(eTotal);
	      nDouble++;
	    }
	    // Store energy loss and reset sum 
	    etot   = eTotal;
	    eTotal = -1;
	  } // if (eTotal>0)
	  else {
	    // If we have no current sum 
	    
	    // Check if this is marked as used, and if so, continue
	    if (used) {used = kFALSE; continue; }
	    
	    // If the signal is abvoe the cut, set current
	    if (thisValid) etot = mult;
	    
	    // If the signal is abiove the cut, and so is the next 
	    // signal and either of them are below the high cut, 
	    if (thisValid  && nextValid  && (thisSmall || nextSmall)) {
	      
	      // If this is below the high cut, and the next is too, then 
	      // we have two low signals 
	      if (thisSmall && nextSmall) twoLow = kTRUE;
	      
	      // If this signal is bigger than the next, and the 
	      // one after that is below the low-cut, then update 
	      // the sum
	      if (mult>multNext && multNextNext < GetLowCut(d, r, eta)) {
		etot = mult + multNext;
		used = kTRUE;
		histos->fDouble->Fill(etot);
		nDouble++;
	      }
	      // Otherwise, we may need to merge with a third strip
	      else {
		etot   = 0;
		eTotal = mult + multNext;
	      }
	    }
	    // This is a signle hit 
	    else if(etot > 0) {
	      histos->fSingle->Fill(etot);
	      histos->fSinglePerStrip->Fill(etot,t);
	      nSingle++;
	    }
	  } // else if (etotal >= 0)
	  
	  mergedEnergy = etot;
	  // if (mergedEnergy > GetHighCut(d, r, eta ,false)) {
	  //   histos->fDistanceAfter->Fill(nDistanceAfter);
	  //   nDistanceAfter    = -1;
	  // }
	  //if(mult>0 && multNext >0)
	  //  std::cout<<mult<<"  "<<multNext<<"  "<<mergedEnergy<<std::endl;
	  
	  if (!fCorrectAngles)
	    mergedEnergy = AngleCorrect(mergedEnergy, eta);
	  // if (mergedEnergy > 0) histos->Incr();
	  
	  if (t != 0) 
	    histos->fNeighborsAfter->Fill(output.Multiplicity(d,r,s,t-1), 
					  mergedEnergy);
	  histos->fBeforeAfter->Fill(mult, mergedEnergy);
	  if(mergedEnergy > 0)
	    histos->fAfter->Fill(mergedEnergy);
	  histos->fSum->Fill(eta,phi,mergedEnergy);
	  
	  output.SetMultiplicity(d,r,s,t,mergedEnergy);
	} // for strip
      } // for sector
    } // for ring 
  } // for detector
  DMSG(fDebug, 3,"single=%9d, double=%9d, triple=%9d", 
       nSingle, nDouble, nTriple);
  next.Reset();
  // while ((o = static_cast<RingHistos*>(next()))) o->Finish();

  return kTRUE;
}

//_____________________________________________________________________
Double_t 
AliFMDSharingFilter::SignalInStrip(const AliESDFMD& input, 
				   UShort_t         d,
				   Char_t           r,
				   UShort_t         s,
				   UShort_t         t) const
{
  // 
  // Get the signal in a strip 
  // 
  // Parameters:
  //    fmd   ESD object
  //    d     Detector
  //    r     Ring 
  //    s     Sector 
  //    t     Strip
  // 
  // Return:
  //    The energy signal 
  //
  Double_t mult = input.Multiplicity(d,r,s,t);
  // In case of 
  //  - bad value (invalid or 0) 
  //  - we want angle corrected and data is 
  //  - we don't want angle corrected and data isn't 
  // just return read value  
  if (mult == AliESDFMD::kInvalidMult               || 
      mult == 0                                     ||
      (fCorrectAngles && input.IsAngleCorrected()) || 
      (!fCorrectAngles && !input.IsAngleCorrected()))
    return mult;

  // If we want angle corrected data, correct it, 
  // otherwise de-correct it 
  if (fCorrectAngles) mult = AngleCorrect(mult, input.Eta(d,r,s,t));
  else                mult = DeAngleCorrect(mult, input.Eta(d,r,s,t));
  return mult;
}
//_____________________________________________________________________
Double_t 
AliFMDSharingFilter::GetLowCut(UShort_t d, Char_t r, Double_t eta) const
{
  //
  // Get the low cut.  Normally, the low cut is taken to be the lower
  // value of the fit range used when generating the energy loss fits.
  // However, if fLowCut is set (using SetLowCit) to a value greater
  // than 0, then that value is used.
  //
  return fLCuts.GetMultCut(d,r,eta,false);
}
			
//_____________________________________________________________________
Double_t 
AliFMDSharingFilter::GetHighCut(UShort_t d, Char_t r, 
				Double_t eta, Bool_t errors) const
{
  //
  // Get the high cut.  The high cut is defined as the 
  // most-probably-value peak found from the energy distributions, minus 
  // 2 times the width of the corresponding Landau.
  //
  return fHCuts.GetMultCut(d,r,eta,errors); 
}

//____________________________________________________________________
Double_t
AliFMDSharingFilter::AngleCorrect(Double_t mult, Double_t eta) const
{
  // 
  // Angle correct the signal 
  // 
  // Parameters:
  //    mult Angle Un-corrected Signal 
  //    eta  Pseudo-rapidity 
  // 
  // Return:
  //    Angle corrected signal 
  //
  Double_t theta =  2 * TMath::ATan(TMath::Exp(-eta));
  if (eta < 0) theta -= TMath::Pi();
  return mult * TMath::Cos(theta);
}
//____________________________________________________________________
Double_t
AliFMDSharingFilter::DeAngleCorrect(Double_t mult, Double_t eta) const
{
  // 
  // Angle de-correct the signal 
  // 
  // Parameters:
  //    mult Angle corrected Signal 
  //    eta  Pseudo-rapidity 
  // 
  // Return:
  //    Angle un-corrected signal 
  //
  Double_t theta =  2 * TMath::ATan(TMath::Exp(-eta));
  if (eta < 0) theta -= TMath::Pi();
  return mult / TMath::Cos(theta);
}

//____________________________________________________________________
void
AliFMDSharingFilter::Terminate(const TList* dir, TList* output, Int_t nEvents)
{
  // 
  // Scale the histograms to the total number of events 
  // 
  // Parameters:
  //    dir     Where the output is 
  //    nEvents Number of events 
  //
  DGUARD(fDebug,1, "Scale histograms in AliFMDSharingFilter");
  if (nEvents <= 0) return;
  TList* d = static_cast<TList*>(dir->FindObject(GetName()));
  if (!d) return;

  TList* out = new TList;
  out->SetName(d->GetName());
  out->SetOwner();

  TParameter<int>* nFiles = 
    static_cast<TParameter<int>*>(dir->FindObject("nFiles"));

  TH2* lowCuts  = static_cast<TH2*>(dir->FindObject("lowCuts"));
  TH2* highCuts = static_cast<TH2*>(dir->FindObject("highCuts"));
  if (lowCuts && nFiles) {
    lowCuts->Scale(1. / nFiles->GetVal());
    out->Add(lowCuts->Clone());
  }
  else 
    AliWarning("low cuts histogram not found in input list");
  if (highCuts && nFiles) {
    highCuts->Scale(1. / nFiles->GetVal());
    out->Add(highCuts->Clone());
  }
  else 
    AliWarning("high cuts histogram not found in input list");
  
  TIter    next(&fRingHistos);
  RingHistos* o = 0;
  THStack* sums = new THStack("sums", "Sum of ring signals");
  while ((o = static_cast<RingHistos*>(next()))) {
    o->Terminate(d, nEvents);
    if (!o->fSum) { 
      Warning("Terminate", "No sum histogram found for ring %s", o->GetName());
      continue;
    }
    TH1D* sum = o->fSum->ProjectionX(o->GetName(), 1, o->fSum->GetNbinsY(),"e");
    sum->Scale(1., "width");
    sum->SetTitle(o->GetName());
    sum->SetDirectory(0);
    sum->SetYTitle("#sum #Delta/#Delta_{mip}");
    sums->Add(sum);
  }
  out->Add(sums);
  output->Add(out);
}

//____________________________________________________________________
void
AliFMDSharingFilter::CreateOutputObjects(TList* dir)
{
  // 
  // Define the output histograms.  These are put in a sub list of the
  // passed list.   The histograms are merged before the parent task calls 
  // AliAnalysisTaskSE::Terminate 
  // 
  // Parameters:
  //    dir Directory to add to 
  //
  DGUARD(fDebug,1, "Define output in AliFMDSharingFilter");
  TList* d = new TList;
  d->SetName(GetName());
  dir->Add(d);

#if 0
  fSummed = new TH2I("operations", "Strip operations", 
		     kMergedInto, kNone-.5, kMergedInto+.5,
		     51201, -.5, 51200.5);
  fSummed->SetXTitle("Operation");
  fSummed->SetYTitle("# of strips");
  fSummed->SetZTitle("Events");
  fSummed->GetXaxis()->SetBinLabel(kNone,            "None");
  fSummed->GetXaxis()->SetBinLabel(kCandidate,       "Candidate");
  fSummed->GetXaxis()->SetBinLabel(kMergedWithOther, "Merged w/other");
  fSummed->GetXaxis()->SetBinLabel(kMergedInto,      "Merged into");
  fSummed->SetDirectory(0);
  d->Add(fSummed);
#endif

  fHighCuts = new TH2D("highCuts", "High cuts used", 1,0,1, 1,0,1);
  fHighCuts->SetXTitle("#eta");
  fHighCuts->SetDirectory(0);
  d->Add(fHighCuts);

  fLowCuts = new TH2D("lowCuts", "Low cuts used", 1,0,1, 1,0,1);
  fLowCuts->SetXTitle("#eta");
  fLowCuts->SetDirectory(0);
  d->Add(fLowCuts);

  // d->Add(lowCut);
  // d->Add(nXi);
  // d->Add(sigma);
  d->Add(AliForwardUtil::MakeParameter("angle", fCorrectAngles));
  d->Add(AliForwardUtil::MakeParameter("lowSignal", 
				       fZeroSharedHitsBelowThreshold));
  d->Add(AliForwardUtil::MakeParameter("simple", fUseSimpleMerging));
  d->Add(AliForwardUtil::MakeParameter("sumThree", fThreeStripSharing));
  TParameter<int>* nFiles = new TParameter<int>("nFiles", 1);
  nFiles->SetMergeMode('+');
  d->Add(nFiles);
  
  TObjArray* extraDead = new TObjArray;
  extraDead->SetOwner();
  extraDead->SetName("extraDead");
#if 0
  for (Int_t i = 0; i < fExtraDead.GetSize(); i++) { 
    if (fExtraDead.At(i) < 0) break;
    UShort_t dd, s, t;
    Char_t  r;
    Int_t   id = fExtraDead.At(i);
    AliFMDStripIndex::Unpack(id, dd, r, s, t);
    extraDead->Add(AliForwardUtil::MakeParameter(Form("FMD%d%c[%02d,%03d]",
						      dd, r, s, t), id));
  }
#endif
  fXtraDead.Compact();
  UInt_t firstBit = fXtraDead.FirstSetBit();
  UInt_t nBits    = fXtraDead.GetNbits();
  for (UInt_t i = firstBit; i < nBits; i++) {
    if (!fXtraDead.TestBitNumber(i)) continue;
    UShort_t dd, s, t;
    Char_t  r;
    AliFMDStripIndex::Unpack(i, dd, r, s, t);
    extraDead->Add(AliForwardUtil::MakeParameter(Form("FMD%d%c[%02d,%03d]",
						      dd, r, s, t), 
						 UShort_t(i)));
  }
  // d->Add(&fXtraDead);
  d->Add(extraDead);
  fLCuts.Output(d,"lCuts");
  fHCuts.Output(d,"hCuts");

  TIter    next(&fRingHistos);
  RingHistos* o = 0;
  while ((o = static_cast<RingHistos*>(next()))) {
    o->CreateOutputObjects(d);
  }
}
//____________________________________________________________________
void
AliFMDSharingFilter::Print(Option_t* /*option*/) const
{
  // 
  // Print information
  // 
  // Parameters:
  //    option Not used 
  //
  char ind[gROOT->GetDirLevel()+1];
  for (Int_t i = 0; i < gROOT->GetDirLevel(); i++) ind[i] = ' ';
  ind[gROOT->GetDirLevel()] = '\0';
  std::cout << ind << ClassName() << ": " << GetName() << '\n'
	    << std::boolalpha 
	    << ind << " Debug:                  " << fDebug << "\n"
	    << ind << " Use corrected angles:   " << fCorrectAngles << '\n'
	    << ind << " Zero below threshold:   " 
	    << fZeroSharedHitsBelowThreshold << '\n'
	    << ind << " Use simple sharing:     " << fUseSimpleMerging << '\n'
	    << ind << " Consider invalid null:  " << fInvalidIsEmpty << '\n'
	    << ind << " Allow 3 strip merging:  " << fThreeStripSharing
	    << std::noboolalpha << std::endl;
  std::cout << ind << " Low cuts: " << std::endl;
  fLCuts.Print();
  std::cout << ind << " High cuts: " << std::endl;
  fHCuts.Print();
}
  
//====================================================================
AliFMDSharingFilter::RingHistos::RingHistos()
  : AliForwardUtil::RingHistos(), 
    fBefore(0), 
    fAfter(0), 
    fSingle(0),
    fDouble(0),
    fTriple(0),
    fSinglePerStrip(0),
    // fDistanceBefore(0),
    // fDistanceAfter(0),
    fBeforeAfter(0),
    fNeighborsBefore(0),
    fNeighborsAfter(0),
    fSum(0) // ,
    // fHits(0),
    // fNHits(0)
{
  // 
  // Default CTOR
  //

}

//____________________________________________________________________
AliFMDSharingFilter::RingHistos::RingHistos(UShort_t d, Char_t r)
  : AliForwardUtil::RingHistos(d,r), 
    fBefore(0), 
    fAfter(0),
    fSingle(0),
    fDouble(0),
    fTriple(0),    
    fSinglePerStrip(0),
    // fDistanceBefore(0),
    // fDistanceAfter(0),
    fBeforeAfter(0),
    fNeighborsBefore(0),
    fNeighborsAfter(0),
    fSum(0) //,
    // fHits(0),
    // fNHits(0)
{
  // 
  // Constructor
  // 
  // Parameters:
  //    d detector
  //    r ring 
  //
  fBefore = new TH1D("esdEloss", Form("Energy loss in %s (reconstruction)", 
				      GetName()), 640, -1, 15);
  fBefore->SetXTitle("#Delta E/#Delta E_{mip}");
  fBefore->SetYTitle("P(#Delta E/#Delta E_{mip})");
  fBefore->SetFillColor(Color());
  fBefore->SetFillStyle(3001);
  fBefore->SetLineColor(kBlack);
  fBefore->SetLineStyle(2);
  fBefore->SetDirectory(0);

  fAfter  = static_cast<TH1D*>(fBefore->Clone("anaEloss"));
  fAfter->SetTitle(Form("Energy loss in %s (sharing corrected)", GetName()));
  fAfter->SetFillColor(Color()+2);
  fAfter->SetLineStyle(1);
  fAfter->SetDirectory(0);
  
  fSingle = new TH1D("singleEloss", "Energy loss (single strips)", 
		     600, 0, 15);
  fSingle->SetXTitle("#Delta/#Delta_{mip}");
  fSingle->SetYTitle("P(#Delta/#Delta_{mip})");
  fSingle->SetFillColor(Color());
  fSingle->SetFillStyle(3001);
  fSingle->SetLineColor(kBlack);
  fSingle->SetLineStyle(2);
  fSingle->SetDirectory(0);

  fDouble = static_cast<TH1D*>(fSingle->Clone("doubleEloss"));
  fDouble->SetTitle("Energy loss (two strips)");
  fDouble->SetFillColor(Color()+1);
  fDouble->SetDirectory(0);
  
  fTriple = static_cast<TH1D*>(fSingle->Clone("tripleEloss"));
  fTriple->SetTitle("Energy loss (three strips)"); 
  fTriple->SetFillColor(Color()+2);
  fTriple->SetDirectory(0);
  
  //Int_t nBinsForInner = (r == 'I' ? 32 : 16);
  Int_t nBinsForInner = (r == 'I' ? 512 : 256);
  Int_t nStrips       = (r == 'I' ? 512 : 256);
  
  fSinglePerStrip = new TH2D("singlePerStrip", "SinglePerStrip", 
			     600,0,15, nBinsForInner,0,nStrips);
  fSinglePerStrip->SetXTitle("#Delta/#Delta_{mip}");
  fSinglePerStrip->SetYTitle("Strip #");
  fSinglePerStrip->SetZTitle("Counts");
  fSinglePerStrip->SetDirectory(0);

#if 0
  fDistanceBefore = new TH1D("distanceBefore", "Distance before sharing", 
			     nStrips , 0,nStrips );
  fDistanceBefore->SetXTitle("Distance");
  fDistanceBefore->SetYTitle("Counts");
  fDistanceBefore->SetFillColor(kGreen+2);
  fDistanceBefore->SetFillStyle(3001);
  fDistanceBefore->SetLineColor(kBlack);
  fDistanceBefore->SetLineStyle(2);
  fDistanceBefore->SetDirectory(0);

  fDistanceAfter = static_cast<TH1D*>(fDistanceBefore->Clone("distanceAfter"));
  fDistanceAfter->SetTitle("Distance after sharing"); 
  fDistanceAfter->SetFillColor(kGreen+1);
  fDistanceAfter->SetDirectory(0);
#endif


  
  Double_t max = 15;
  Double_t min = -1;
  Int_t    n   = int((max-min) / (max / 300));
  fBeforeAfter = new TH2D("beforeAfter", "Before and after correlation", 
			  n, min, max, n, min, max);
  fBeforeAfter->SetXTitle("#Delta E/#Delta E_{mip} before");
  fBeforeAfter->SetYTitle("#Delta E/#Delta E_{mip} after");
  fBeforeAfter->SetZTitle("Correlation");
  fBeforeAfter->SetDirectory(0);

  fNeighborsBefore = static_cast<TH2D*>(fBeforeAfter->Clone("neighborsBefore"));
  fNeighborsBefore->SetTitle("Correlation of neighbors before");
  fNeighborsBefore->SetXTitle("#Delta E_{i}/#Delta E_{mip}");
  fNeighborsBefore->SetYTitle("#Delta E_{i+1}/#Delta E_{mip}");
  fNeighborsBefore->SetDirectory(0);
  
  fNeighborsAfter = 
    static_cast<TH2D*>(fNeighborsBefore->Clone("neighborsAfter"));
  fNeighborsAfter->SetTitle("Correlation of neighbors after");
  fNeighborsAfter->SetDirectory(0);

  fSum = new TH2D("summed", "Summed signal", 200, -4, 6, 
		  (fRing == 'I' || fRing == 'i' ? 20 : 40), 0, 2*TMath::Pi());
  fSum->SetDirectory(0);
  fSum->Sumw2();
  fSum->SetMarkerColor(Color());
  // fSum->SetFillColor(Color());
  fSum->SetXTitle("#eta");
  fSum->SetYTitle("#varphi [radians]");
  fSum->SetZTitle("#sum #Delta/#Delta_{mip}(#eta,#varphi) ");

#if 0  
  fHits = new TH1D("hits", "Number of hits", 200, 0, 200000);
  fHits->SetDirectory(0);
  fHits->SetXTitle("# of hits");
  fHits->SetFillColor(kGreen+1);
#endif
}
//____________________________________________________________________
AliFMDSharingFilter::RingHistos::RingHistos(const RingHistos& o)
  : AliForwardUtil::RingHistos(o), 
    fBefore(o.fBefore), 
    fAfter(o.fAfter),
    fSingle(o.fSingle),
    fDouble(o.fDouble),
    fTriple(o.fTriple),
    fSinglePerStrip(o.fSinglePerStrip),
    // fDistanceBefore(o.fDistanceBefore),
    // fDistanceAfter(o.fDistanceAfter),    
    fBeforeAfter(o.fBeforeAfter),
    fNeighborsBefore(o.fNeighborsBefore),
    fNeighborsAfter(o.fNeighborsAfter),
    fSum(o.fSum) //,
    // fHits(o.fHits),
    // fNHits(o.fNHits)
{
  // 
  // Copy constructor 
  // 
  // Parameters:
  //    o Object to copy from 
  //
}

//____________________________________________________________________
AliFMDSharingFilter::RingHistos&
AliFMDSharingFilter::RingHistos::operator=(const RingHistos& o)
{
  // 
  // Assignment operator 
  // 
  // Parameters:
  //    o Object to assign from 
  // 
  // Return:
  //    Reference to this 
  //
  if (&o == this) return *this;
  AliForwardUtil::RingHistos::operator=(o);
  fDet = o.fDet;
  fRing = o.fRing;
  
  if (fBefore) 	       delete  fBefore;
  if (fAfter)  	       delete  fAfter;
  if (fSingle) 	       delete  fSingle;
  if (fDouble) 	       delete  fDouble;
  if (fTriple)         delete  fTriple;
  if (fSinglePerStrip) delete fSinglePerStrip;
  // if (fDistanceBefore) delete fDistanceBefore;
  // if (fDistanceAfter)  delete fDistanceAfter;
  // if (fHits)   	       delete fHits;
  
  
  fBefore          = static_cast<TH1D*>(o.fBefore->Clone());
  fAfter           = static_cast<TH1D*>(o.fAfter->Clone());
  fSingle          = static_cast<TH1D*>(o.fSingle->Clone());
  fDouble          = static_cast<TH1D*>(o.fDouble->Clone());
  fTriple          = static_cast<TH1D*>(o.fTriple->Clone());
  fSinglePerStrip  = static_cast<TH2D*>(o.fSinglePerStrip->Clone());
  // fDistanceBefore  = static_cast<TH1D*>(o.fDistanceBefore->Clone());
  // fDistanceAfter   = static_cast<TH1D*>(o.fDistanceAfter->Clone());
  fBeforeAfter     = static_cast<TH2D*>(o.fBeforeAfter->Clone());
  fNeighborsBefore = static_cast<TH2D*>(o.fNeighborsBefore->Clone());
  fNeighborsAfter  = static_cast<TH2D*>(o.fNeighborsAfter->Clone());
  // fHits            = static_cast<TH1D*>(o.fHits->Clone());
  fSum             = static_cast<TH2D*>(o.fSum->Clone());

  return *this;
}
//____________________________________________________________________
AliFMDSharingFilter::RingHistos::~RingHistos()
{
  // 
  // Destructor 
  //
}
#if 0
//____________________________________________________________________
void
AliFMDSharingFilter::RingHistos::Finish()
{
  // 
  // Finish off 
  // 
  //
  // fHits->Fill(fNHits);
}
#endif
//____________________________________________________________________
void
AliFMDSharingFilter::RingHistos::Terminate(const TList* dir, Int_t nEvents)
{
  // 
  // Scale the histograms to the total number of events 
  // 
  // Parameters:
  //    nEvents Number of events 
  //    dir     Where the output is 
  //
  TList* l = GetOutputList(dir);
  if (!l) return; 

#if 0
  TH1D* before = static_cast<TH1D*>(l->FindObject("esdEloss"));
  TH1D* after  = static_cast<TH1D*>(l->FindObject("anaEloss"));
  if (before) before->Scale(1./nEvents);
  if (after)  after->Scale(1./nEvents);
#endif

  TH2D* summed = static_cast<TH2D*>(l->FindObject("summed"));
  if (summed) summed->Scale(1./nEvents);
  fSum = summed;
}

//____________________________________________________________________
void
AliFMDSharingFilter::RingHistos::CreateOutputObjects(TList* dir)
{
  // 
  // Make output 
  // 
  // Parameters:
  //    dir where to store 
  //
  TList* d = DefineOutputList(dir);

  d->Add(fBefore);
  d->Add(fAfter);
  d->Add(fSingle);
  d->Add(fDouble);
  d->Add(fTriple);
  d->Add(fSinglePerStrip);
  // d->Add(fDistanceBefore);
  // d->Add(fDistanceAfter);
  d->Add(fBeforeAfter);
  d->Add(fNeighborsBefore);
  d->Add(fNeighborsAfter);
  // d->Add(fHits);
  d->Add(fSum);

  // Removed to avoid doubly adding the list which destroys 
  // the merging
  //dir->Add(d);
}

//____________________________________________________________________
//
// EOF
//
