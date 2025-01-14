#include "G4SnglNtuple.h"

#include <g4main/PHG4HitContainer.h>
#include <g4main/PHG4Hit.h>
#include <g4main/PHG4Particle.h>
#include <g4main/PHG4TruthInfoContainer.h>

#include <fun4all/Fun4AllHistoManager.h>

#include <phool/getClass.h>

#include <TFile.h>
#include <TH1.h>
#include <TH2.h>
#include <TNtuple.h>

#include <boost/foreach.hpp>

#include<sstream>

using namespace std;

G4SnglNtuple::G4SnglNtuple(const std::string &name, const std::string &filename):
  SubsysReco( name ),
  nblocks(0),
  hm(NULL),
  _filename(filename),
  ntup(NULL),
  outfile(NULL)
{}

G4SnglNtuple::~G4SnglNtuple()
{
  //  delete ntup;
  delete hm;
}


int
G4SnglNtuple::Init( PHCompositeNode* )
{
  ostringstream hname, htit;
  hm = new  Fun4AllHistoManager(Name());
  outfile = new TFile(_filename.c_str(), "RECREATE");
  ntup = new TNtuple("snglntup", "G4Sngls", "phi:theta:e:detid:layer:x0:y0:z0:x1:y1:z1:edep");
  ntup_e = new TNtuple("sngl_e", "G4SnglEdeps", "phi:theta:e:detid:layer:edep");
  //  ntup->SetDirectory(0);
  TH1 *h1 = new TH1F("edep1GeV","edep 0-1GeV",1000,0,1);
  eloss.push_back(h1);
  h1 = new TH1F("edep100GeV","edep 0-100GeV",1000,0,100);
  eloss.push_back(h1);
  return 0;
}

int
G4SnglNtuple::process_event( PHCompositeNode* topNode )
{
  // get the primary particle which did this to us....
  PHG4TruthInfoContainer* truthInfoList 
    =  findNode::getClass<PHG4TruthInfoContainer>(topNode , "G4TruthInfo" );

  const PHG4TruthInfoContainer::Map primMap = truthInfoList->GetPrimaryMap();
  double px = primMap.begin()->second->get_px();
  double py = primMap.begin()->second->get_py();
  double pz = primMap.begin()->second->get_pz();
  double e = primMap.begin()->second->get_e();
  double pt = sqrt(px * px + py * py);
  double phi = atan2(py, px);
  double theta = atan2(pt, pz);

  ostringstream nodename;
  set<string>::const_iterator iter;
  vector<TH1 *>::const_iterator eiter;

  map<int,double> layer_edep_map;
  map<int,double>::const_iterator edepiter;

  for (iter = _node_postfix.begin(); iter != _node_postfix.end(); iter++)
    {
      layer_edep_map.clear();
      int detid = (_detid.find(*iter))->second;
      nodename.str("");
      nodename << "G4HIT_" << *iter;
      PHG4HitContainer *hits = findNode::getClass<PHG4HitContainer>(topNode, nodename.str().c_str());
      if (hits)
        {
          double esum = 0;
          PHG4HitContainer::ConstRange hit_range = hits->getHits();
          for ( PHG4HitContainer::ConstIterator hit_iter = hit_range.first ; hit_iter !=  hit_range.second; hit_iter++ )
            {
              layer_edep_map[hit_iter->second->get_layer()] += hit_iter->second->get_edep();
              esum += hit_iter->second->get_edep();
              ntup->Fill(phi,theta,e,detid,
                         hit_iter->second->get_layer(),
                         hit_iter->second->get_x(0),
                         hit_iter->second->get_y(0),
                         hit_iter->second->get_z(0),
                         hit_iter->second->get_x(1),
                         hit_iter->second->get_y(1),
                         hit_iter->second->get_z(1),
                         hit_iter->second->get_edep());
            }
	  for (edepiter= layer_edep_map.begin(); edepiter != layer_edep_map.end(); edepiter++)
	    {
              ntup_e->Fill(phi,theta,e,detid,edepiter->first,edepiter->second);
	    }
	  ntup_e->Fill(phi,theta,e,detid,-1,esum); // fill sum over all layers for each detector

	  for (eiter=eloss.begin(); eiter != eloss.end(); eiter++)
	    {
	      (*eiter)->Fill(esum);
	    }
        }
    }
  return 0;
}

int
G4SnglNtuple::End(PHCompositeNode * topNode)
{
  outfile->cd();
  ntup->Write();
  outfile->Write();
  outfile->Close();
  delete outfile;
  hm->dumpHistos(_filename, "UPDATE");
  return 0;
}

void
G4SnglNtuple::AddNode(const std::string &name, const int detid)
{
 _node_postfix.insert(name);
 _detid[name] = detid;
 return;
}
