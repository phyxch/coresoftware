#include "PHG4ForwardEcalSteppingAction.h"
#include "PHG4ForwardEcalDetector.h"

#include <g4main/PHG4HitContainer.h>
#include <g4main/PHG4Hit.h>
#include <g4main/PHG4Hitv1.h>

#include <g4main/PHG4TrackUserInfoV1.h>

#include <phool/getClass.h>

#include <Geant4/G4Step.hh>

#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>
// this is an ugly hack, the gcc optimizer has a bug which
// triggers the uninitialized variable warning which
// stops compilation because of our -Werror
#include <boost/version.hpp> // to get BOOST_VERSION
#if (__GNUC__ == 4 && __GNUC_MINOR__ == 4 && BOOST_VERSION == 105700 )
#pragma GCC diagnostic ignored "-Wuninitialized"
#pragma message "ignoring bogus gcc warning in boost header lexical_cast.hpp"
#include <boost/lexical_cast.hpp>
#pragma GCC diagnostic warning "-Wuninitialized"
#else
#include <boost/lexical_cast.hpp>
#endif

#include <iostream>

using namespace std;


//____________________________________________________________________________..
PHG4ForwardEcalSteppingAction::PHG4ForwardEcalSteppingAction( PHG4ForwardEcalDetector* detector ):
  PHG4SteppingAction(NULL),
  detector_( detector ),
  hits_(NULL),
  absorberhits_(NULL),
  hit(NULL)
{

}


//____________________________________________________________________________..
bool PHG4ForwardEcalSteppingAction::UserSteppingAction( const G4Step* aStep, bool )
{
  G4TouchableHandle touch = aStep->GetPreStepPoint()->GetTouchableHandle();
  G4VPhysicalVolume* volume = touch->GetVolume();

  // detector_->IsInForwardEcal(volume)
  // returns
  //  0 is outside of Forward ECAL
  //  1 is inside scintillator
  // -1 is inside absorber (dead material)

  int whichactive = detector_->IsInForwardEcal(volume);

  if ( !whichactive  )
    {
      return false;
    }

  int layer_id = detector_->get_Layer();
  int tower_id = -1;
  int idx_j = -1;
  int idx_k = -1;
  int idx_l = -1;

  if (whichactive > 0) // in sctintillator
    {
      /* Find indizes of sctintillator / tower containing this step */
      FindTowerIndex(touch, idx_j, idx_k);
      tower_id = touch->GetCopyNumber();
    }
  else
    {
      tower_id = touch->GetCopyNumber();
    }

  /* Get energy deposited by this step */
  G4double edep = aStep->GetTotalEnergyDeposit() / GeV;
  G4double eion = (aStep->GetTotalEnergyDeposit() - aStep->GetNonIonizingEnergyDeposit()) / GeV;

  /* Get pointer to associated Geant4 track */
  const G4Track* aTrack = aStep->GetTrack();

  // if this block stops everything, just put all kinetic energy into edep
  if (detector_->IsBlackHole())
    {
      edep = aTrack->GetKineticEnergy() / GeV;
      G4Track* killtrack = const_cast<G4Track *> (aTrack);
      killtrack->SetTrackStatus(fStopAndKill);
    }

  /* Make sure we are in a volume */
  if ( detector_->IsActive() )
    {
      /* Check if particle is 'geantino' */
      bool geantino = false;
      if (aTrack->GetParticleDefinition()->GetPDGEncoding() == 0 &&
	  aTrack->GetParticleDefinition()->GetParticleName().find("geantino") != string::npos)
	{
	  geantino = true;
	}

      /* Get Geant4 pre- and post-step points */
      G4StepPoint * prePoint = aStep->GetPreStepPoint();
      G4StepPoint * postPoint = aStep->GetPostStepPoint();

      switch (prePoint->GetStepStatus())
	{
	case fGeomBoundary:
	case fUndefined:
	  hit = new PHG4Hitv1();
	  hit->set_scint_id(tower_id);

	  /* Set hit location (tower index) */
	  hit->set_index_j(idx_j);
	  hit->set_index_k(idx_k);
	  hit->set_index_l(idx_l);

	  /* Set hit location (space point) */
	  hit->set_x( 0, prePoint->GetPosition().x() / cm);
	  hit->set_y( 0, prePoint->GetPosition().y() / cm );
	  hit->set_z( 0, prePoint->GetPosition().z() / cm );

	  /* Set hit time */
	  hit->set_t( 0, prePoint->GetGlobalTime() / nanosecond );

	  /* set the track ID */
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

	  /* set intial energy deposit */
	  hit->set_edep( 0 );
	  hit->set_eion( 0 );

	  /* Now add the hit to the hit collection */
	  if (whichactive > 0)
	    {
	      // Now add the hit
	      hits_->AddHit(layer_id, hit);
	    }
	  else
	    {
	      absorberhits_->AddHit(layer_id, hit);
	    }
	  break;
	default:
	  break;
	}

      /* Update exit values- will be overwritten with every step until
       * we leave the volume or the particle ceases to exist */
      hit->set_x( 1, postPoint->GetPosition().x() / cm );
      hit->set_y( 1, postPoint->GetPosition().y() / cm );
      hit->set_z( 1, postPoint->GetPosition().z() / cm );

      hit->set_t( 1, postPoint->GetGlobalTime() / nanosecond );

      /* sum up the energy to get total deposited */
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

      return true;

    }
  else
    {
      return false;
    }
}


//____________________________________________________________________________..
void PHG4ForwardEcalSteppingAction::SetInterfacePointers( PHCompositeNode* topNode )
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
      std::cout << "PHG4ForwardEcalSteppingAction::SetTopNode - unable to find " << hitnodename << std::endl;
    }
  if ( ! absorberhits_)
    {
      if (verbosity > 0)
	{
	  cout << "PHG4ForwardEcalSteppingAction::SetTopNode - unable to find " << absorbernodename << endl;
	}
    }
}

int
PHG4ForwardEcalSteppingAction::FindTowerIndex(G4TouchableHandle touch, int& j, int& k)
{
        int j_0, k_0;           //The k and k indices for the scintillator / tower

        G4VPhysicalVolume* tower = touch->GetVolume(1);		//Get the tower solid
	ParseG4VolumeName(tower, j_0, k_0);

        j = (j_0*1);
        k = (k_0*1);

        return 0;
}

int
PHG4ForwardEcalSteppingAction::ParseG4VolumeName( G4VPhysicalVolume* volume, int& j, int& k ) 
{
	boost::char_separator<char> sep("_");
	boost::tokenizer<boost::char_separator<char> > tok(volume->GetName(), sep);
	boost::tokenizer<boost::char_separator<char> >::const_iterator tokeniter;
	for (tokeniter = tok.begin(); tokeniter != tok.end(); ++tokeniter)
	{
		if (*tokeniter == "j")
		{
			++tokeniter;
			j = boost::lexical_cast<int>(*tokeniter);
		}
		else if (*tokeniter == "k")
		{
			++tokeniter;
			k = boost::lexical_cast<int>(*tokeniter);
		}
	}

	return 0;
}
