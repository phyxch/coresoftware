#include "PHG4CEmcTestBeamSteppingAction.h"
#include "PHG4CEmcTestBeamDetector.h"

#include <g4main/PHG4HitContainer.h>
#include <g4main/PHG4Hit.h>
#include <g4main/PHG4Hitv1.h>

#include <g4main/PHG4TrackUserInfoV1.h>

#include <phool/getClass.h>

#include <Geant4/G4Step.hh>
#include <Geant4/G4SystemOfUnits.hh>

#include <iostream>

using namespace std;
//____________________________________________________________________________..
PHG4CEmcTestBeamSteppingAction::PHG4CEmcTestBeamSteppingAction( PHG4CEmcTestBeamDetector* detector ):
  PHG4SteppingAction(NULL),
  detector_( detector ),
  hits_(NULL),
  absorberhits_(NULL),
  hit(NULL)
{}

//____________________________________________________________________________..
bool PHG4CEmcTestBeamSteppingAction::UserSteppingAction( const G4Step* aStep, bool )
{

  G4TouchableHandle touch = aStep->GetPreStepPoint()->GetTouchableHandle();
  // get volume of the current step
  G4VPhysicalVolume* volume = touch->GetVolume();

  int whichactive = detector_->IsInCEmcTestBeam(volume);

  if (!whichactive)
    {
      return false;
    }

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

  int tower_id = touch->GetCopyNumber(2); // tower number
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
	  hit->set_layer((unsigned int)tower_id);
	  hit->set_scint_id(touch->GetCopyNumber(1)); // the copy number of the sandwich
	  //here we set the entrance values in cm
	  hit->set_x( 0, prePoint->GetPosition().x() / cm);
	  hit->set_y( 0, prePoint->GetPosition().y() / cm );
	  hit->set_z( 0, prePoint->GetPosition().z() / cm );
	  // time in ns
	  hit->set_t( 0, prePoint->GetGlobalTime() / nanosecond );
	  //set the track ID
	  {
	    int trkoffset = 0;
	    if ( G4VUserTrackInformation* p = aTrack->GetUserInformation() )
	      {
		if ( PHG4TrackUserInfoV1* pp = dynamic_cast<PHG4TrackUserInfoV1*>(p) )
		  {
		    trkoffset = pp->GetTrackIdOffset();
		  }
	      }
	    hit->set_trkid(aTrack->GetTrackID() + trkoffset);
	  }

	  //set the initial energy deposit
	  hit->set_edep(0);
	  hit->set_eion(0); // only implemented for v5 otherwise empty

	  if (whichactive > 0) // return of isinCEmcTestDetector, > 0 hit in scintillator, < 0 hit in absorber
	    {
	      // Now add the hit
	      hits_->AddHit(tower_id, hit);
	    }
	  else
	    {
	      absorberhits_->AddHit(tower_id, hit);
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
void PHG4CEmcTestBeamSteppingAction::SetInterfacePointers( PHCompositeNode* topNode )
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
      std::cout << "PHG4CEmcTestBeamSteppingAction::SetTopNode - unable to find " << hitnodename << std::endl;
    }
  if ( ! absorberhits_)
    {
      if (verbosity > 0)
	{
	  cout << "PHG4HcalSteppingAction::SetTopNode - unable to find " << absorbernodename << endl;
	}
    }
}
