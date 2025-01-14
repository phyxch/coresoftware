
#ifndef __CALOTRUTHEVAL_H__
#define __CALOTRUTHEVAL_H__

#include "BaseTruthEval.h"

#include <phool/PHCompositeNode.h>
#include <g4main/PHG4TruthInfoContainer.h>

#include <g4main/PHG4Particle.h>
#include <g4main/PHG4VtxPoint.h>
#include <g4main/PHG4HitContainer.h>
#include <g4main/PHG4Hit.h>

#include <string>
#include <set>
#include <map>

class CaloTruthEval {

public:

  CaloTruthEval(PHCompositeNode *topNode, std::string caloname); // CEMC, HCALIN, HCALOUT
  virtual ~CaloTruthEval();

  void next_event(PHCompositeNode *topNode);
  void do_caching(bool do_cache) {_do_cache = do_cache;}
  void set_strict(bool strict) {
    _strict = strict;
    _basetrutheval.set_strict(strict);
  }
  void set_verbosity(int verbosity) {
    _verbosity = verbosity;
    _basetrutheval.set_verbosity(verbosity);
  }
  
  std::set<PHG4Hit*> all_truth_hits(PHG4Particle* particle);  
  PHG4Particle*      get_parent_particle(PHG4Hit* g4hit);
  PHG4Particle*      get_primary_particle(PHG4Particle* particle);
  PHG4Particle*      get_primary_particle(PHG4Hit* g4hit);  
  int                get_embed(PHG4Particle* particle);
  PHG4VtxPoint*      get_vertex(PHG4Particle* particle);

  bool               is_g4hit_from_particle(PHG4Hit* g4hit, PHG4Particle* particle);
  bool               are_same_particle(PHG4Particle* p1, PHG4Particle* p2);
  bool               are_same_vertex(PHG4VtxPoint* vtx1, PHG4VtxPoint* vtx2);
  
  bool               is_primary(PHG4Particle* particle);
  std::set<PHG4Hit*> get_shower_from_primary(PHG4Particle* primary);  
  float              get_shower_moliere_radius(PHG4Particle* primary);
  float              get_shower_energy_deposit(PHG4Particle* primary);

  unsigned int       get_errors() {return _errors;}
  
private:

  void get_node_pointers(PHCompositeNode *topNode);

  BaseTruthEval _basetrutheval;
  
  std::string _caloname;
  PHG4TruthInfoContainer* _truthinfo;
  PHG4HitContainer* _g4hits;

  bool _strict;
  int _verbosity;
  unsigned int _errors;
  
  bool                                        _do_cache;
  std::map<PHG4Particle*,std::set<PHG4Hit*> > _cache_all_truth_hits_g4particle;
  std::map<PHG4Hit*,PHG4Particle*>            _cache_get_primary_particle_g4hit;
  std::map<PHG4Particle*,std::set<PHG4Hit*> > _cache_get_shower_from_primary;
  std::map<PHG4Particle*,float>               _cache_get_shower_moliere_radius;
  std::map<PHG4Particle*,float>               _cache_get_shower_energy_deposit;
};

#endif // __CALOTRUTHEVAL_H__
