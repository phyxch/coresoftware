#include "PHG4ConeRegionSteppingAction.h"
#include "PHG4ConeDetector.h"

#include <g4main/PHG4HitContainer.h>
#include <g4main/PHG4Hit.h>
#include <g4main/PHG4Hitv1.h>

#include <phool/getClass.h>

#include <Geant4/G4Step.hh>

#include <iostream>

using namespace std;
//____________________________________________________________________________..
PHG4ConeRegionSteppingAction::PHG4ConeRegionSteppingAction( PHG4ConeDetector* detector ):
  detector_( detector )
{}

//____________________________________________________________________________..
void PHG4ConeRegionSteppingAction::UserSteppingAction( const G4Step* aStep)
{

  // get volume of the current step
  G4VPhysicalVolume* volume = aStep->GetPreStepPoint()->GetTouchableHandle()->GetVolume();

  // collect energy and track length step by step
  G4double edep = aStep->GetTotalEnergyDeposit() / GeV;

  const G4Track* aTrack = aStep->GetTrack();

  int layer_id = 0;
  // make sure we are in a volume
  if ( detector_->IsInConeActive(volume) )
    {
      G4StepPoint * prePoint = aStep->GetPreStepPoint();
      G4StepPoint * postPoint = aStep->GetPostStepPoint();
       cout << "track id " << aTrack->GetTrackID() << endl;
       cout << "time prepoint: " << prePoint->GetGlobalTime() << endl;
       cout << "time postpoint: " << postPoint->GetGlobalTime() << endl;
      switch (prePoint->GetStepStatus())
        {
        case fGeomBoundary:
        case fUndefined:
          hit = new PHG4Hitv1();
          //here we set the entrance values in cm
          hit->set_x( 0, prePoint->GetPosition().x() / cm);
          hit->set_y( 0, prePoint->GetPosition().y() / cm );
          hit->set_z( 0, prePoint->GetPosition().z() / cm );
	  // time in ns
          hit->set_t( 0, prePoint->GetGlobalTime() / nanosecond );
          //set the track ID
          hit->set_trkid(aTrack->GetTrackID() );

          //set the initial energy deposit
          hit->set_edep(0);

          // Now add the hit
          hits_->AddHit(layer_id, hit);

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

      //    hit->identify();
      // return true to indicate the hit was used
      return;

    }
  else
    {
      return;
    }
}

//____________________________________________________________________________..
void PHG4ConeRegionSteppingAction::SetInterfacePointers( PHCompositeNode* topNode )
{

  string hitnodename = "G4HIT_" + detector_->GetName();
  //now look for the map and grab a pointer to it.
  hits_ =  findNode::getClass<PHG4HitContainer>( topNode , hitnodename.c_str() );

  // if we do not find the node we need to make it.
  if ( ! hits_ )
    { std::cout << "PHG4ConeRegionSteppingAction::SetTopNode - unable to find " << hitnodename << std::endl;
 }

}
