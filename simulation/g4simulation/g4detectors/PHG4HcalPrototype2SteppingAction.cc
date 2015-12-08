// This is the steppingaction code for the 2nd hcal prototype
// created on 11/23/2015, HeXC
//
#include "PHG4HcalPrototype2SteppingAction.h"
#include "PHG4HcalPrototype2Detector.h"

#include <g4main/PHG4HitContainer.h>
#include <g4main/PHG4Hit.h>
#include <g4main/PHG4Hitv1.h>

#include <g4main/PHG4TrackUserInfoV1.h>

#include <phool/getClass.h>

#include <Geant4/G4Step.hh>

#include <iostream>

using namespace std;
//____________________________________________________________________________..
PHG4HcalPrototype2SteppingAction::PHG4HcalPrototype2SteppingAction( PHG4HcalPrototype2Detector* detector ):
  PHG4SteppingAction(NULL),
  detector_( detector ),
  hits_(NULL),
  absorberhits_(NULL),
  hit(NULL)
{}

//____________________________________________________________________________..
bool PHG4HcalPrototype2SteppingAction::UserSteppingAction( const G4Step* aStep, bool )
{

  G4TouchableHandle touch = aStep->GetPreStepPoint()->GetTouchableHandle();
  // get volume of the current step
  G4VPhysicalVolume* volume = touch->GetVolume();

  // We simply use ourown scintID to test on this condition
  //  int whichactive = detector_->IsInHcalPrototype2(volume);

  /*
  if (!whichactive)
    {
      return false;
    }
  */

  // collect energy and track length step by step
  G4double edep = aStep->GetTotalEnergyDeposit() / GeV;
  G4double eion = (aStep->GetTotalEnergyDeposit()-aStep->GetNonIonizingEnergyDeposit()) / GeV;

  const G4Track* aTrack = aStep->GetTrack();

  // if this block stops everything, just put all kinetic energy into edep
  if (detector_->IsBlackHole())
    {
      edep = aTrack->GetKineticEnergy() / GeV;
      G4Track* killtrack = const_cast<G4Track *> (aTrack);
      killtrack->SetTrackStatus(fStopAndKill);
    }

  // Add detector element ID into to the hit

  G4int scintSheetCopyNumber, scintSheetLayerNumber;
  
  G4int sectionID = 0;
  G4int scintID = 0x0;
  //  G4int absID = 0;
  
  if (volume->GetName() == "hcal1AbsLayer")
    {
      sectionID = 1;
      //      absID = volume->GetCopyNo()+100;   // Inner section absorber layer ID's are 100, 101, ..., 115
      scintID = -1;                      // scintID is not valid, we set it to -1
    }
  else if (volume->GetName() == "hcal2AbsLayer")
    {
      sectionID = 2;
      //      absID = volume->GetCopyNo()+200;   // Outer section absorber layer ID's are 200, 201, ..., 215
      scintID = -1;                      // scintID is not valid, we set it to -1
    }
  else if (volume->GetName() == "inner1USheet")
    {
      sectionID = 1;
      scintSheetCopyNumber = volume->GetCopyNo();   // either 1 or 0
      scintSheetLayerNumber = aStep->GetPreStepPoint()->GetTouchableHandle()->GetVolume(1)->GetCopyNo();
      scintID = (scintSheetLayerNumber << 2) + (1 << 1) + scintSheetCopyNumber;
      //      absID = -1;                        // absID is not valid, we set it to -1
      //G4cout << " **************** scintID: " << scintID << G4endl;
    }
  else if (volume->GetName() == "inner2USheet")
    {
      sectionID = 1;
      scintSheetCopyNumber = volume->GetCopyNo();   // either 1 or 0
      scintSheetLayerNumber = aStep->GetPreStepPoint()->GetTouchableHandle()->GetVolume(1)->GetCopyNo();
      scintID = (scintSheetLayerNumber << 2) + (0 << 1) + scintSheetCopyNumber;
      //      absID = -1;                        // absID is not valid, we set it to -1
      //G4cout << " **************** scintID: " << scintID << G4endl;
    }
  else if (volume->GetName() == "outer1USheet")
    {
      sectionID = 2;
      scintSheetCopyNumber = volume->GetCopyNo();   // either 1 or 0
      scintSheetLayerNumber = aStep->GetPreStepPoint()->GetTouchableHandle()->GetVolume(1)->GetCopyNo();
      scintID = (scintSheetLayerNumber << 2) + (1 << 1) + scintSheetCopyNumber;
      //      absID = -1;                        // absID is not valid, we set it to -1
      //G4cout << " **************** scintID: " << scintID << G4endl;
    }
  else if (volume->GetName() == "outer2USheet")
    {
      sectionID = 2;
      scintSheetCopyNumber = volume->GetCopyNo();   // either 1 or 0
      scintSheetLayerNumber = aStep->GetPreStepPoint()->GetTouchableHandle()->GetVolume(1)->GetCopyNo();
      scintID = (scintSheetLayerNumber << 2) + (0 << 1) + scintSheetCopyNumber;
      //      absID = -1;                        // absID is not valid, we set it to -1
      //G4cout << " **************** scintID: " << scintID << G4endl;
    }
  else {
    return false;
  }
  
  // make sure we are in a volume
  if ( detector_->IsActive() )
    {
      bool geantino = false;
      // the check for the pdg code speeds things up, I do not want to make
      // an expensive string compare for every track when we know
      // geantino or chargedgeantino has pid=0
      if (aTrack->GetParticleDefinition()->GetPDGEncoding() == 0 &&
	  aTrack->GetParticleDefinition()->GetParticleName().find("geantino") != string::npos)
	{
	  geantino = true;
	}

      G4StepPoint * prePoint = aStep->GetPreStepPoint();
      G4StepPoint * postPoint = aStep->GetPostStepPoint();
      //       cout << "track id " << aTrack->GetTrackID() << endl;
      //       cout << "time prepoint: " << prePoint->GetGlobalTime() << endl;
      //       cout << "time postpoint: " << postPoint->GetGlobalTime() << endl;
      switch (prePoint->GetStepStatus())
	{
	case fGeomBoundary:
	case fUndefined:
	  hit = new PHG4Hitv1();
	  hit->set_layer((unsigned int)sectionID);
	  hit->set_scint_id(scintID); 
	  //here we set the entrance values in cm
	  hit->set_x( 0, prePoint->GetPosition().x() / cm);
	  hit->set_y( 0, prePoint->GetPosition().y() / cm );
	  hit->set_z( 0, prePoint->GetPosition().z() / cm );
	  // time in ns
	  hit->set_t( 0, prePoint->GetGlobalTime() / nanosecond );
	  //set the track ID
	  {
	    int trkoffset = 0;
	    // Commented out by hexc 12/8/2015
	    // Not sure why it is needed here.
	    /*
	    if ( G4VUserTrackInformation* p = aTrack->GetUserInformation() )
	      {
		if ( PHG4TrackUserInfoV1* pp = dynamic_cast<PHG4TrackUserInfoV1*>(p) )
		  {
		    trkoffset = pp->GetTrackIdOffset();
		  }
	      }
	    */
	    hit->set_trkid(aTrack->GetTrackID() + trkoffset);
	  }
	  
	  //set the initial energy deposit
	  hit->set_edep(0);
	  hit->set_eion(0);
	  if (scintID > 0) // return of isinHcalTestDetector, > 0 hit in scintillator, < 0 hit in absorber
	    {
	      // Now add the hit
	      hits_->AddHit(sectionID, hit);
	    }
	  else
	    {
	      absorberhits_->AddHit(sectionID, hit);
	    }
	  break;
	default:
	  break;
	}

      // here we just update the exit values, it will be overwritten
      // for every step until we leave the volume or the particle
      // ceases to exist
      hit->set_x( 1, postPoint->GetPosition().x() / cm );
      hit->set_y( 1, postPoint->GetPosition().y() / cm );
      hit->set_z( 1, postPoint->GetPosition().z() / cm );

      hit->set_t( 1, postPoint->GetGlobalTime() / nanosecond );
      //sum up the energy to get total deposited
      hit->set_edep(hit->get_edep() + edep);
      hit->set_eion(hit->get_eion() + eion);
      if (geantino)
	{
	  hit->set_edep(-1); // only energy=0 g4hits get dropped, this way geantinos survive the g4hit compression
          hit->set_eion(-1);
	}
      if (edep > 0)
	{
	  if ( G4VUserTrackInformation* p = aTrack->GetUserInformation() )
	    {
	      if ( PHG4TrackUserInfoV1* pp = dynamic_cast<PHG4TrackUserInfoV1*>(p) )
		{
		  pp->SetKeep(1); // we want to keep the track
		}


	    }
	}

      //       hit->identify();
      // return true to indicate the hit was used
      return true;

    }
  else
    {
      return false;
    }
}

//____________________________________________________________________________..
void PHG4HcalPrototype2SteppingAction::SetInterfacePointers( PHCompositeNode* topNode )
{

  string hitnodename;
  string absorbernodename;
  if (detector_->SuperDetector() != "NONE")
    {
      hitnodename = "G4HIT_" + detector_->SuperDetector();
      absorbernodename =  "G4HIT_ABSORBER_" + detector_->SuperDetector();
    }
  else
    {
      hitnodename = "G4HIT_" + detector_->GetName();
      absorbernodename =  "G4HIT_ABSORBER_" + detector_->GetName();
    }

  //now look for the map and grab a pointer to it.
  hits_ =  findNode::getClass<PHG4HitContainer>( topNode , hitnodename.c_str() );
  absorberhits_ =  findNode::getClass<PHG4HitContainer>( topNode , absorbernodename.c_str() );

  // if we do not find the node it's messed up.
  if ( ! hits_ )
    {
      std::cout << "PHG4HcalPrototype2SteppingAction::SetTopNode - unable to find " << hitnodename << std::endl;
    }
  if ( ! absorberhits_)
    {
      if (verbosity > 0)
	{
	  cout << "PHG4HcalSteppingAction::SetTopNode - unable to find " << absorbernodename << endl;
	}
    }
}
