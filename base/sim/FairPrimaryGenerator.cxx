#include "FairPrimaryGenerator.h"

#include "FairGenerator.h"              // for FairGenerator
#include "FairGenericStack.h"           // for FairGenericStack
#include "FairLogger.h"                 // for FairLogger, MESSAGE_ORIGIN
#include "FairMCEventHeader.h"          // for FairMCEventHeader

#include "TDatabasePDG.h"               // for TDatabasePDG
#include "TF1.h"                        // for TF1
#include "TIterator.h"                  // for TIterator
#include "TMCProcess.h"                 // for TMCProcess::kPPrimary
#include "TMath.h"                      // for Sqrt
#include "TObject.h"                    // for TObject
#include "TParticlePDG.h"               // for TParticlePDG
#include "TRandom.h"                    // for TRandom, gRandom
#include "TString.h"                    // for TString

#include <stddef.h>                     // for NULL
#include <iostream>                     // for operator<<, basic_ostream, etc

using std::cout;
using std::cerr;
using std::endl;

Int_t FairPrimaryGenerator::fTotPrim=0;

// -----   Default constructor   -------------------------------------------
FairPrimaryGenerator::FairPrimaryGenerator()
  :TNamed(),
   fBeamX0(0),
   fBeamY0(0),
   fBeamSigmaX(0),
   fBeamSigmaY(0),
   fBeamGradX(0),
   fBeamGradY(0),
   fBeamGradX0(0),
   fBeamGradY0(0),
   fBeamGradSigmaX(0),
   fBeamGradSigmaY(0),
   fTargetZ (new Double_t[1]),
   fNrTargets(1),
   fTargetDz(0),
   fVertex(TVector3(0.,0.,0.)),
   fNTracks(0),
   fSmearVertexZ(kFALSE),
   fSmearGausVertexZ(kFALSE),
   fSmearVertexXY(kFALSE),
   fStack(NULL),
   fGenList(new TObjArray()),
   fListIter(fGenList->MakeIterator()),
   fEvent(NULL),
   fdoTracking(kTRUE),
   fEventTimeMin(0),
   fEventTimeMax(0),
   fEventTime(0),
   fEventMeanTime(0),
   fTimeProb(0),
   fMCIndexOffset(0),
   fLogger(FairLogger::GetLogger()),
   fEventNr(0)
{
  fTargetZ[0] = 0.;
}
// -------------------------------------------------------------------------



// -----   Constructor with title and name   -------------------------------
FairPrimaryGenerator::FairPrimaryGenerator(const char* name, const char* title)
  :TNamed(name,title),
   fBeamX0(0),
   fBeamY0(0),
   fBeamSigmaX(0),
   fBeamSigmaY(0),
   fBeamGradX(0),
   fBeamGradY(0),
   fBeamGradX0(0),
   fBeamGradY0(0),
   fBeamGradSigmaX(0),
   fBeamGradSigmaY(0),
   fTargetZ (new Double_t[1]),
   fNrTargets(1),
   fTargetDz(0),
   fVertex(TVector3(0.,0.,0.)),
   fNTracks(0),
   fSmearVertexZ(kFALSE),
   fSmearGausVertexZ(kFALSE),
   fSmearVertexXY(kFALSE),
   fStack(NULL),
   fGenList(new TObjArray()),
   fListIter(fGenList->MakeIterator()),
   fEvent(NULL),
   fdoTracking(kTRUE),
   fEventTimeMin(0),
   fEventTimeMax(0),
   fEventTime(0),
   fEventMeanTime(0),
   fTimeProb(NULL),
   fMCIndexOffset(0),
   fLogger(FairLogger::GetLogger()),
   fEventNr(0)
{
  fTargetZ[0] = 0.;
}
// -------------------------------------------------------------------------
Bool_t FairPrimaryGenerator::Init()
{
  /** Initialize list of generators*/
  for(Int_t i=0; i<fGenList->GetEntries(); i++ ) {
    FairGenerator* gen= (FairGenerator*) fGenList->At(i);
    if(gen) { gen->Init(); }
  }
  return kTRUE;
}


// -----   Destructor   ----------------------------------------------------
FairPrimaryGenerator::~FairPrimaryGenerator()
{
  //  cout<<"Enter Destructor of FairPrimaryGenerator"<<endl;
  // the stack is deleted by FairMCApplication
  if (1 == fNrTargets) {
    delete fTargetZ;
  } else {
    delete [] fTargetZ;
  }
  fGenList->Delete();
  delete fGenList;
  delete fListIter;

  delete fTimeProb;
  //  cout<<"Leave Destructor of FairPrimaryGenerator"<<endl;
}
// -------------------------------------------------------------------------



// -----   Public method GenerateEvent   -----------------------------------
Bool_t FairPrimaryGenerator::GenerateEvent(FairGenericStack* pStack)
{
  // Check for MCEventHeader
  if ( ! fEvent) {
    fLogger->Fatal(MESSAGE_ORIGIN,"FairPrimaryGenerator::GenerateEvent: No MCEventHeader branch!");
    Fatal("GenerateEvent", "No MCEventHeader branch");
  }

  // Initialise
  fStack   = pStack;
  fNTracks = 0;
  fEvent->Reset();

  // Create event vertex
  MakeVertex();
  fEvent->SetVertex(fVertex);

  // Create beam gradiant
  // Here we only random generate two gradiants (gradx, grady) for the event and later on (in AddTrack())
  // all our particles will be rotated accordingly.
  MakeBeamGradiant();

  // Call the ReadEvent methods from all registered generators
  fListIter->Reset();
  TObject* obj = 0;
  FairGenerator* gen = 0;
  while( (obj = fListIter->Next()) ) {
    gen = dynamic_cast<FairGenerator*> (obj);
    if ( ! gen ) { return kFALSE; }
    const char* genName = gen->GetName();
    fMCIndexOffset = fNTracks;// number tracks before generator is called
    Bool_t test = gen->ReadEvent(this);
    if ( ! test ) {
      fLogger->Error(MESSAGE_ORIGIN,"FairPrimaryGenerator: ReadEvent failed for generator ", genName );
      return kFALSE;
    }
  }

  if(fTimeProb!=0) {
    fEventTime = fEventTime + fTimeProb->GetRandom();
  } else {
    fEventTime = fEventTime + gRandom->Uniform( fEventTimeMin,  fEventTimeMax);
  }

  fEvent->SetTime(fEventTime);

  fTotPrim += fNTracks;
  // Screen output

  // Set the event number if not set already by one of the dedicated generators
  if (-1 == fEvent->GetEventID()) {
    fEventNr++;
    fEvent->SetEventID(fEventNr);
  }

  fLogger->Info(MESSAGE_ORIGIN,"FairPrimaryGenerator: (Event %i) %i  primary tracks from vertex (%f, %f, %f ) with beam gradiant (%f, %f) Event Time = %f (ns)" ,fEvent->GetEventID(), fNTracks, fVertex.X(), fVertex.Y(), fVertex.Z(), fBeamGradX, fBeamGradY, fEventTime);

  fEvent->SetNPrim(fNTracks);


  return kTRUE;
}
// -------------------------------------------------------------------------



// -----   Public method AddTrack   ----------------------------------------
void FairPrimaryGenerator::AddTrack(Int_t pdgid, Double_t px_raw, Double_t py_raw,
                                    Double_t pz_raw, Double_t vx, Double_t vy,
                                    Double_t vz, Int_t parent,Bool_t wanttracking,Double_t e)
{

  // ---> Add event vertex to track vertex
  vx += fVertex.X();
  vy += fVertex.Y();
  vz += fVertex.Z();

  TVector3 mom(px_raw, py_raw, pz_raw);
  mom.RotateX(-fBeamGradY);
  mom.RotateY(fBeamGradX);

  // ---> Convert K0 and AntiK0 into K0s and K0L
  if ( pdgid == 311 || pdgid == -311 ) {
    Double_t test = gRandom->Uniform(0.,1.);
    if (test >= 0.5) { pdgid = 310; }    // K0S
    else { pdgid = 130; }   // K0L
  }

  // ---> Check whether particle type is in PDG Database
  TDatabasePDG* pdgBase = TDatabasePDG::Instance();
  if ( ! pdgBase ) {
    Fatal("FairPrimaryGenerator",
          "No TDatabasePDG instantiated");
  } else {
    TParticlePDG* pdgPart = pdgBase->GetParticle(pdgid);
    if ( ! pdgPart ) {
      if( e<0) {
        cerr << "\033[5m\033[31m -E FairPrimaryGenerator: PDG code " << pdgid << " not found in database.\033[0m " << endl;
        cerr << "\033[5m\033[31m -E FairPrimaryGenerator: Discarding particle \033[0m " << endl;
        cerr << "\033[5m\033[31m -E FairPrimaryGenerator: now MC Index is corrupted \033[0m " << endl;
        return;
      } else {
        cout << "\033[5m\033[31m -W FairPrimaryGenerator: PDG code " << pdgid << " not found in database. This warning can be savely ignored.\033[0m " << endl;
      }
    }
  }
  // ---> Get mass and calculate energy of particle
  if(e<0) {
    Double_t mass = pdgBase->GetParticle(pdgid)->Mass();
    e = TMath::Sqrt( mom.Mag2()+ mass*mass );
  }// else, use the value of e given to the function

  // ---> Set all other parameters required by PushTrack
  Int_t    doTracking =  0;   // Go to tracking
  if(fdoTracking && wanttracking) { doTracking = 1 ; }
  Int_t    dummyparent     = -1;   // Primary particle (now the value is -1 by default)
  Double_t tof        =  0.;  // Time of flight
  Double_t polx       =  0.;  // Polarisation
  Double_t poly       =  0.;
  Double_t polz       =  0.;
  Int_t    ntr        =  0;   // Track number; to be filled by the stack
  Double_t weight     =  1.;  // Weight
  Int_t    status     =  0;   // Generation status

  if( parent!=-1) { parent+=fMCIndexOffset; }// correct for tracks which are in list before generator is called
  // Add track to stack
  fStack->PushTrack(doTracking, dummyparent, pdgid, mom.X(), mom.Y(), mom.Z(), e, vx, vy, vz,
                    tof, polx, poly, polz, kPPrimary, ntr, weight, status, parent);
  fNTracks++;

}
// -------------------------------------------------------------------------



// -----   Public method SetBeam   -----------------------------------------
void FairPrimaryGenerator::SetBeam(Double_t x0, Double_t y0,
                                   Double_t sigmaX, Double_t sigmaY)
{
  fBeamX0     = x0;
  fBeamY0     = y0;
  fBeamSigmaX = sigmaX;
  fBeamSigmaY = sigmaY;
}
// -------------------------------------------------------------------------

// -----   Public method SetBeamGrad   -------------------------------------
void FairPrimaryGenerator::SetBeamGrad(Double_t beamGradX0, Double_t beamGradY0,
                                       Double_t beamGradSigmaX, Double_t beamGradSigmaY)
{
  fBeamGradX0 = beamGradX0;
  fBeamGradY0 = beamGradY0;
  fBeamGradSigmaX = beamGradSigmaX;
  fBeamGradSigmaY = beamGradSigmaY;
}
// -------------------------------------------------------------------------


// -----   Public method SetTarget   ---------------------------------------
void FairPrimaryGenerator::SetTarget(Double_t z, Double_t dz)
{

  fTargetZ[0] = z;
  fTargetDz = dz;
}
// -------------------------------------------------------------------------

// -----   Public method SetMultTarget   -----------------------------------
void FairPrimaryGenerator::SetMultTarget(Int_t nroftargets, Double_t* targetpos, Double_t dz)
{

  if (1 == fNrTargets) {
    delete fTargetZ;
  } else {
    delete[] fTargetZ;
  }

  fNrTargets = nroftargets;

  fTargetZ = new Double_t[nroftargets];
  for (Int_t i=0; i<nroftargets; i++) {
    fTargetZ[i]  = targetpos[i];
  }
  fTargetDz = dz;

}
// -------------------------------------------------------------------------



// -----   Private method MakeVertex   -------------------------------------
void FairPrimaryGenerator::MakeVertex()
{
  Double_t vx = fBeamX0;
  Double_t vy = fBeamY0;
  Double_t vz;
  if (1 == fNrTargets) {
    vz = fTargetZ[0];
  } else {
    Int_t Target = (Int_t)gRandom->Uniform(fNrTargets);
    vz = fTargetZ[Target];
  }

  if (fSmearVertexZ) vz = gRandom->Uniform(vz - fTargetDz/2.,
                            vz + fTargetDz/2.);

  if (fSmearGausVertexZ) { vz = gRandom->Gaus(vz, fTargetDz); }
  if (fSmearVertexXY) {
    if (fBeamSigmaX != 0.) { vx = gRandom->Gaus(fBeamX0, fBeamSigmaX); }
    if (fBeamSigmaY != 0.) { vy = gRandom->Gaus(fBeamY0, fBeamSigmaY); }
  }

  fVertex = TVector3(vx,vy,vz);
}
// -------------------------------------------------------------------------

// -----   Private method MakeBeamGradiant   -------------------------------
void FairPrimaryGenerator::MakeBeamGradiant()
{
  if (fBeamGradSigmaX != 0.) { fBeamGradX = gRandom->Gaus(fBeamGradX0, fBeamGradSigmaX); }
  if (fBeamGradSigmaY != 0.) { fBeamGradY = gRandom->Gaus(fBeamGradY0, fBeamGradSigmaY); }
}
// -------------------------------------------------------------------------

void FairPrimaryGenerator::SetEventTimeInterval(Double_t min, Double_t max)
{
  fEventTimeMin=min;
  fEventTimeMax=max;

}
// -------------------------------------------------------------------------

void  FairPrimaryGenerator::SetEventMeanTime(Double_t mean)
{
  fEventMeanTime =mean;
  TString form="(1/";
  form+= mean;
  form+=")*exp(-x/";
  form+=mean;
  form+=")";
  fTimeProb= new TF1("TimeProb.", form.Data(), 0., mean*10);

}
// -------------------------------------------------------------------------


void  FairPrimaryGenerator::SetEventTime(TF1* timeProb)
{
  if (timeProb!=0) {
    fTimeProb= timeProb;
  } else {
    cout << " \033[5m\033[31m -E FairPrimaryGenerator: invalid time function, Event time is not SET \033[0m " << endl;
  }
}






ClassImp(FairPrimaryGenerator)
