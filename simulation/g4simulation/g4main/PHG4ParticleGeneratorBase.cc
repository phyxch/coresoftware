#include "PHG4ParticleGeneratorBase.h"
#include "PHG4Particlev1.h"

#include "PHG4InEvent.h"

#include <fun4all/getClass.h>

#include <phool/PHCompositeNode.h>
#include <phool/PHIODataNode.h>

#include <Geant4/G4ParticleTable.hh>
#include <Geant4/G4ParticleDefinition.hh>

using namespace std;

PHG4ParticleGeneratorBase::PHG4ParticleGeneratorBase(const string &name):
  SubsysReco(name),
  embedflag(0),
  vtx_x(0),
  vtx_y(0),
  vtx_z(0),
  t0(0)
{
  return;
}

PHG4ParticleGeneratorBase::~PHG4ParticleGeneratorBase()
{
  while (particlelist.begin() != particlelist.end())
    {
      delete particlelist.back();
      particlelist.pop_back();
    }
  return;
}

int
PHG4ParticleGeneratorBase::get_pdgcode(const std::string &name)
{
  G4ParticleTable* particleTable = G4ParticleTable::GetParticleTable();
  G4String particleName = name;
  G4ParticleDefinition* particledef
    = particleTable->FindParticle(particleName);
  if (particledef)
    {
      return particledef->GetPDGEncoding();
    }
  return 0;
}

string
PHG4ParticleGeneratorBase::get_pdgname(const int pdgcode)
{
  G4ParticleTable* particleTable = G4ParticleTable::GetParticleTable();
  G4ParticleDefinition* particledef
    = particleTable->FindParticle(pdgcode);
  if (particledef)
    {
      return particledef->GetParticleName();
    }
  return 0;
}

void
PHG4ParticleGeneratorBase::set_name(const std::string &particle)
{
  CheckAndCreateParticleVector();
  particlelist[0]->set_name(particle);
  particlelist[0]->set_pid(get_pdgcode(particle));
  return;
}

void
PHG4ParticleGeneratorBase::set_pid(const int pid)
{
  CheckAndCreateParticleVector();
  particlelist[0]->set_pid(pid);
}

void
PHG4ParticleGeneratorBase::set_mom(const double x, const double y, const double z)
{
  CheckAndCreateParticleVector();
  particlelist[0]->set_px(x);
  particlelist[0]->set_py(y);
  particlelist[0]->set_pz(z);
  return;
}

void
PHG4ParticleGeneratorBase::set_vtx(const double x, const double y, const double z)
{
  vtx_x = x;
  vtx_y = y;
  vtx_z = z;
  return;
}

int
PHG4ParticleGeneratorBase::InitRun(PHCompositeNode *topNode)
{
  PHG4InEvent *ineve = findNode::getClass<PHG4InEvent>(topNode, "PHG4INEVENT");
  if (!ineve)
    {
      PHNodeIterator iter( topNode );
      PHCompositeNode *dstNode;
      dstNode = dynamic_cast<PHCompositeNode*>(iter.findFirst("PHCompositeNode", "DST"));

      ineve = new PHG4InEvent();
      PHDataNode<PHObject> *newNode = new PHDataNode<PHObject>(ineve, "PHG4INEVENT", "PHObject");
      dstNode->addNode(newNode);
    }
  return 0;
}

int
PHG4ParticleGeneratorBase::process_event(PHCompositeNode *topNode)
{
  cout << PHWHERE << " " << Name() << " using empty process_event" << endl;
  return 0;
}

void
PHG4ParticleGeneratorBase::Print(const std::string &what) const
{
  vector<PHG4Particle *>::const_iterator iter;
  int i = 0;
  for (iter = particlelist.begin(); iter != particlelist.end(); iter++)
    {
      cout << "particle " << i << endl;
      (*iter)->identify();
      i++;
    }
}

void
PHG4ParticleGeneratorBase::AddParticle(const std::string &particle, const double x, const double y, const double z)
{
  PHG4Particle *part = new PHG4Particlev1(particle, get_pdgcode(particle), x, y, z);
  particlelist.push_back(part);
}

void
PHG4ParticleGeneratorBase::AddParticle(const int pid, const double x, const double y, const double z)
{
  PHG4Particle *particle = new PHG4Particlev1();
  particle->set_pid(pid);
  particle->set_px(x);
  particle->set_py(y);
  particle->set_pz(z);
  particlelist.push_back(particle);
}

void
PHG4ParticleGeneratorBase::CheckAndCreateParticleVector()
{
  if (!particlelist.size())
    {
      PHG4Particle *part = new PHG4Particlev1();
      particlelist.push_back(part);
    }
  return;
}

void
PHG4ParticleGeneratorBase::SetParticleId(PHG4Particle * particle, PHG4InEvent *ineve)
{
  if ((particle->get_name()).size() == 0) // no size -> empty name string
    {
      particle->set_name(get_pdgname(particle->get_pid()));
    }
  if (particle->get_pid() == 0)
    {
      particle->set_pid(get_pdgcode(particle->get_name()));
    }
  if (embedflag)
    {
      ineve->AddEmbeddedParticle(particle);
    }
  return;
}
