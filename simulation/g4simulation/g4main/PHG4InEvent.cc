#include "PHG4InEvent.h"
#include "PHG4Particle.h"
#include "PHG4VtxPointv1.h"

#include <phool/phool.h>

#include <cstdlib>

using namespace std;

ClassImp(PHG4InEvent)

PHG4InEvent::~PHG4InEvent()
{
  
  vtxlist.clear();
  particlelist.clear();
  return;
}

int
PHG4InEvent::AddVtx(const int id, const PHG4VtxPoint &vtx)
{
  PHG4VtxPoint *newvtx = new PHG4VtxPoint(vtx);
  int iret = AddVtxCommon(newvtx);
  return iret;
}

int
PHG4InEvent::AddVtxHepMC(const int id, const double x, const double y, const double z, const double t0 = NAN)
{
  PHG4VtxPoint *newvtx = new PHG4VtxPointv1(x,y,z,t0);
  vtxlist[id]=newvtx;
  return 0;
}

int
PHG4InEvent::AddVtx(const double x, const double y, const double z, const double t0 = NAN)
{
  PHG4VtxPoint *newvtx = new PHG4VtxPointv1(x,y,z,t0);
  int iret = AddVtxCommon(newvtx);
  return iret;
}

int
PHG4InEvent::AddVtxCommon(PHG4VtxPoint *newvtx)
{
  std::pair< std::map<int, PHG4VtxPoint *>::const_iterator, std::map<int, PHG4VtxPoint *>::const_iterator > vtxbegin_end =  GetVertices();
  for (map<int, PHG4VtxPoint *>::const_iterator viter = vtxbegin_end.first; viter != vtxbegin_end.second; ++viter)
    {
      if (*newvtx == *(viter->second))
	{
	  delete newvtx;
	  return viter->first;
	}
    }
  int id = GetNVtx()+1;
  vtxlist[id]=newvtx;
  return id;
}

int
PHG4InEvent::AddParticle(const int vtxid, PHG4Particle *particle)
{
  if (vtxlist.find(vtxid) == vtxlist.end())
    {
      cout << "cannot add particle to non existing vertex, id: " << vtxid << endl;
      exit(1);
    }
  std::pair< std::multimap<int,PHG4Particle *>::const_iterator, std::multimap<int,PHG4Particle *>::const_iterator > particles = GetParticles(vtxid);

  // checking for duplicate particles - sometimes interesting
//   for (multimap<int,PHG4Particle *>::const_iterator piter = particles.first; piter != particles.second; piter++)
//     {
//       if (*particle == *(piter->second))
// 	{
// 	  cout << PHWHERE << "Particle already added (same pid and momentum), dropping it since duplication will cause problems down the line particle:" << endl;
// 	  particle->identify();
// 	  delete particle;
// 	  return -1;
// 	}
//     }
  particlelist.insert(pair<int,PHG4Particle *>(vtxid, particle) );
  return 0;
}

void
PHG4InEvent::Reset()
{
  embedded_particlelist.clear(); // just pointers - we can clear it without deleting
  while(vtxlist.begin() != vtxlist.end())
    {
      delete vtxlist.begin()->second;
      vtxlist.erase(vtxlist.begin());
    }
  while(particlelist.begin() != particlelist.end())
    {
      delete particlelist.begin()->second;
      particlelist.erase(particlelist.begin());
    }
  return;
}

pair< map<int, PHG4VtxPoint *>::const_iterator, map<int, PHG4VtxPoint *>::const_iterator >
PHG4InEvent::GetVertices() const
{
  pair< map<int, PHG4VtxPoint *>::const_iterator, map<int, PHG4VtxPoint *>::const_iterator > retpair(vtxlist.begin(), vtxlist.end());
  return retpair;
}


pair< multimap<int,PHG4Particle *>::const_iterator, multimap<int,PHG4Particle *>::const_iterator >
PHG4InEvent::GetParticles(const int vtxid) const
{
  pair<multimap<int,PHG4Particle *>::const_iterator, multimap<int,PHG4Particle *>::const_iterator > retpair(particlelist.lower_bound(vtxid),particlelist.upper_bound(vtxid));
  return retpair;
}

pair< multimap<int,PHG4Particle *>::const_iterator, multimap<int,PHG4Particle *>::const_iterator >
PHG4InEvent::GetParticles() const
{
  pair<multimap<int,PHG4Particle *>::const_iterator, multimap<int,PHG4Particle *>::const_iterator > retpair(particlelist.begin(),particlelist.end());
  return retpair;
}

pair< multimap<int,PHG4Particle *>::iterator, multimap<int,PHG4Particle *>::iterator >
PHG4InEvent::GetParticles_Modify()
{
  pair<multimap<int,PHG4Particle *>::iterator, multimap<int,PHG4Particle *>::iterator > retpair(particlelist.begin(),particlelist.end());
  return retpair;
}

void
PHG4InEvent::identify(ostream& os) const
{
  os << "vtx: " << endl;
  multimap<int,PHG4Particle *>::const_iterator particle_iter;
  for(map<int,PHG4VtxPoint *>::const_iterator iter = vtxlist.begin(); iter != vtxlist.end(); ++iter)
    {
      os << "vtx " << iter->first << " , ";
      iter->second->identify(os);
      pair<multimap<int, PHG4Particle *>::const_iterator, multimap<int, PHG4Particle *>::const_iterator > particlebegin_end = GetParticles(iter->first);
      for(particle_iter = particlebegin_end.first; particle_iter != particlebegin_end.second; ++particle_iter)
	{
	  os << "vtx " << particle_iter->first << ", ";
	  particle_iter->second->identify(os);
	}
    }
  if (!embedded_particlelist.empty())
    {
      os << "embedded particles:" << endl;
      for (set<PHG4Particle *>::const_iterator iter = embedded_particlelist.begin(); iter != embedded_particlelist.end(); ++iter)
	{
	  (*iter)->identify(os);
	}
    }
  else
    {
      os << "no embedded particles" << endl;
    }
  return;
}

int
PHG4InEvent::isEmbeded(PHG4Particle *p) const
{
  if (embedded_particlelist.find(p) != embedded_particlelist.end())
    {
      return true;
    }
  return false;
}

void
PHG4InEvent::DeleteParticle(std::multimap<int, PHG4Particle *>::iterator &iter)
{
  delete iter->second;
  particlelist.erase(iter);
}
