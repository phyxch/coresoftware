#include "PHG4PrimaryGeneratorAction.h"
#include "PHG4InEvent.h"
#include "PHG4VtxPoint.h"
#include "PHG4Particle.h"
#include "PHG4UserPrimaryParticleInformation.h"

#include <phool/phool.h>

#include <Geant4/G4Event.hh>
#include <Geant4/G4ParticleTable.hh>
#include <Geant4/G4PrimaryParticle.hh>
#include <Geant4/G4PrimaryVertex.hh>
#include <Geant4/G4SystemOfUnits.hh>
#include <Geant4/G4ThreeVector.hh>

#include <cstdlib>
#include <iostream>
#include <map>

using namespace std;

void
PHG4PrimaryGeneratorAction::GeneratePrimaries(G4Event* anEvent)
{
  if (!inEvent)
    {
      return;
    }
  map<int, PHG4VtxPoint *>::const_iterator vtxiter;
  multimap<int, PHG4Particle *>::const_iterator particle_iter;
  std::pair< std::map<int, PHG4VtxPoint *>::const_iterator, std::map<int, PHG4VtxPoint *>::const_iterator > vtxbegin_end = inEvent->GetVertices();

  for (vtxiter = vtxbegin_end.first; vtxiter != vtxbegin_end.second; ++vtxiter)
    {
      //       cout << "vtx number: " << vtxiter->first << endl;
      //       (*vtxiter->second).identify();
      // expected units are cm !
      G4ThreeVector position((*vtxiter->second).get_x()*cm, (*vtxiter->second).get_y()*cm, (*vtxiter->second).get_z()*cm );
      G4PrimaryVertex* vertex = new G4PrimaryVertex(position, (*vtxiter->second).get_t()*s);
      pair<multimap<int, PHG4Particle *>::const_iterator, multimap<int, PHG4Particle *>::const_iterator > particlebegin_end = inEvent->GetParticles(vtxiter->first);
      for (particle_iter = particlebegin_end.first; particle_iter != particlebegin_end.second; ++particle_iter)
        {
	  //          cout << "PHG4PrimaryGeneratorAction: dealing with" << endl;
	  //           (particle_iter->second)->identify();

	  // this is really ugly, and maybe it can be streamlined. Initially it was clear cut, if we only give a particle by its name,
	  // we find it here in the G4 particle table, find the
	  // PDG id and then hand it off with the momentum to G4PrimaryParticle
	  // We also have the capability to give a particle a PDG id and then we don't need this translation (the pdg/particle name lookup is
	  // done somewhere else, maybe this should be rethought)
	  // The problem is that geantinos have the pdg pid = 0 but handing this off to the G4PrimaryParticle ctor will just drop it. So
	  // after going through this pdg id lookup once, we have to go through it again in case it is still zero and treat the
	  // geantinos specially. Probably this can be combined with some thought, but rigth now I don't have time for this
          if (!(*particle_iter->second).get_pid())
            {
              G4String particleName = (*particle_iter->second).get_name();
              G4ParticleTable* particleTable = G4ParticleTable::GetParticleTable();
              G4ParticleDefinition* particledef
		= particleTable->FindParticle(particleName);
              if (particledef)
                {
                  (*particle_iter->second).set_pid(particledef->GetPDGEncoding());
                }
              else
                {
                  cout << PHWHERE << "Cannot get PDG value for particle " << particleName
                       << ", dropping it" << endl;
                  continue;
                }
            }
          G4PrimaryParticle* g4part = NULL;
          if (!(*particle_iter->second).get_pid()) // deal with geantinos which have pid=0
            {
              G4ParticleTable* particleTable = G4ParticleTable::GetParticleTable();
              G4ParticleDefinition* particle_definition
		= particleTable->FindParticle((*particle_iter->second).get_name());
              if (particle_definition)
                {
                  G4double mass =  particle_definition->GetPDGMass();
                  g4part = new G4PrimaryParticle(particle_definition);
                  double ekin = sqrt((*particle_iter->second).get_px() * (*particle_iter->second).get_px() +
                                     (*particle_iter->second).get_py() * (*particle_iter->second).get_py() +
                                     (*particle_iter->second).get_pz() * (*particle_iter->second).get_pz());

                  // expected momentum unit is GeV
                  g4part->SetKineticEnergy( ekin*GeV );
                  g4part->SetMass( mass );
                  G4ThreeVector v((*particle_iter->second).get_px(), (*particle_iter->second).get_py(), (*particle_iter->second).get_pz());
                  G4ThreeVector vunit = v.unit();
                  g4part->SetMomentumDirection( vunit );
                  g4part->SetCharge(  particle_definition->GetPDGCharge() );
                  G4ThreeVector         particle_polarization;
                  g4part->SetPolarization(particle_polarization.x(),
                                          particle_polarization.y(),
                                          particle_polarization.z());
                }
	      else
		{
		  cout << PHWHERE << " cannot get G4 particle definition" << endl;
		  cout << "you should have never gotten here, please check this in detail" << endl;
		  cout << "exiting now" << endl;
		  exit(1);
		}
            }
          else
            {
              // expected momentum unit is GeV
              g4part = new G4PrimaryParticle((*particle_iter->second).get_pid(),
                                             (*particle_iter->second).get_px()*GeV,
                                             (*particle_iter->second).get_py()*GeV,
                                             (*particle_iter->second).get_pz()*GeV);
            }
          if (inEvent->isEmbeded(particle_iter->second))
            {
              PHG4UserPrimaryParticleInformation *userdata = new PHG4UserPrimaryParticleInformation(1);
              g4part->SetUserInformation(userdata);
            }
          vertex->SetPrimary(g4part);
        }
      //      vertex->Print();
      anEvent->AddPrimaryVertex(vertex);
    }
  return;
}
