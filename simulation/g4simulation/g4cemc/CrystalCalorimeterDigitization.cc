#include "CrystalCalorimeterDigitization.h"

#include <phool/PHCompositeNode.h>
#include <phool/PHNodeIterator.h>
#include <phool/PHIODataNode.h>

#include <stdio.h>

#include <fun4all/Fun4AllReturnCodes.h>
#include <phool/getClass.h>

#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>

#include <stdexcept>

using namespace std;

/*
 * Utility class for random number genreation using GSL
 */
class CrystalCalorimeterDigitizationRNG {

public:
  CrystalCalorimeterDigitizationRNG( int randomSeed )
  {
    rng_randSeed = randomSeed;

    /* create a generator */
    rng_generatorType = gsl_rng_mt19937;
    rng_dice = gsl_rng_alloc ( rng_generatorType );

    /* Set seed for random number generator */
    gsl_rng_set( rng_dice, rng_randSeed );
  }

  ~CrystalCalorimeterDigitizationRNG(){}

  float RollPoisson( float mean )
  {
    return gsl_ran_poisson( rng_dice, mean );
  }

  int rng_randSeed;
  const gsl_rng_type * rng_generatorType;
  gsl_rng * rng_dice;

};


CrystalCalorimeterDigitization::CrystalCalorimeterDigitization( const std::string& name , const std::string& nameRaw , const std::string& nameDigi ,  int randSeed):
  SubsysReco(name),
  _towersDigi(NULL),
  _nodeNameTowerRaw(nameRaw),
  _nodeNameTowerDigi(nameDigi),
  _meanLY(200),
  _applyPhotonStatistic(false),
  _randSeed(randSeed),
  _timer( PHTimeServer::get()->insert_new(name) )
{
  _dice = new CrystalCalorimeterDigitizationRNG( _randSeed );
}

int
CrystalCalorimeterDigitization::InitRun(PHCompositeNode *topNode)
{
  /* Access DST node */
  PHNodeIterator iter(topNode);

  /* Looking for the DST node */
  PHCompositeNode *dstNode;
  dstNode = dynamic_cast<PHCompositeNode*>(iter.findFirst("PHCompositeNode", "DST"));
  if (!dstNode)
    {
      std::cerr << PHWHERE << "DST Node missing, doing nothing." << std::endl;
      exit(1);
    }

  try
    {
      CreateNodes(topNode);
    }
  catch (std::exception& e)
    {
      std::cerr << e.what() << std::endl;
      exit(1);
    }

  return Fun4AllReturnCodes::EVENT_OK;

}

int
CrystalCalorimeterDigitization::process_event(PHCompositeNode *topNode)
{
  if (verbosity)
    {
      std::cout << PHWHERE << "CrystalCalorimeterDigitization : Process event entered" << std::endl;
    }

  RawTowerContainer* towersRaw = findNode::getClass<RawTowerContainer>(topNode, _nodeNameTowerRaw.c_str());
  if (!towersRaw)
    {
      std::cerr << PHWHERE << "CrystalCalorimeterDigitization : Could not locate input tower node " << _nodeNameTowerRaw << endl;
      exit(1);
    }

  // loop over all towers in the event
  RawTowerContainer::ConstIterator towerit;
  RawTowerContainer::ConstRange towers_begin_end = towersRaw->getTowers();

  RawTowerv1* tower_raw_i = NULL;
  for (towerit = towers_begin_end.first; towerit != towers_begin_end.second; towerit++)
    {
      /* Get raw tower and energy */
      tower_raw_i= dynamic_cast<RawTowerv1*>( (*towerit).second );
      double energy_raw = tower_raw_i->get_energy();

      int etabin = tower_raw_i->get_bineta();
      int phibin = tower_raw_i->get_binphi();

      /* If this digi tower exists already in output collection- throw error */
      if( _towersDigi->getTower( etabin, phibin ) )
	{
	  std::cerr << PHWHERE << "CrystalCalorimeterDigitization : Cannot create two towers with same ID j = " << etabin << " , k = " << phibin  << endl;
	  exit(1);
	}

      /* Create Digi tower for this Raw tower */
      RawTowerv1* tower_digi_i = new RawTowerv1( etabin , phibin );
      _towersDigi->AddTower(  etabin, phibin, tower_digi_i );

      /* Convert energy to number of photons via mean light yield*/
      int nPhotons = static_cast<int>( energy_raw * _meanLY * 1000.0 ); // [edep] = GeV, [_meanLY] = 200 / MeV
      tower_digi_i->add_ecell( 1, nPhotons );

      /* Apply photon statistic? */
      if ( _applyPhotonStatistic )
      	ApplyPhotonStatistic( *tower_digi_i );
    }

  return Fun4AllReturnCodes::EVENT_OK;
}

int
CrystalCalorimeterDigitization::End(PHCompositeNode *topNode)
{
  return Fun4AllReturnCodes::EVENT_OK;
}

void
CrystalCalorimeterDigitization::CreateNodes(PHCompositeNode *topNode)
{
  PHNodeIterator iter(topNode);
  PHCompositeNode *runNode = static_cast<PHCompositeNode*>(iter.findFirst("PHCompositeNode", "RUN"));
  if (!runNode)
    {
      std::cerr << PHWHERE << "Run Node missing, doing nothing." << std::endl;
      throw std::runtime_error("Failed to find Run node in CrystalCalorimeterDigitization::CreateNodes");
    }

  PHCompositeNode *dstNode = static_cast<PHCompositeNode*>(iter.findFirst("PHCompositeNode", "DST"));
  if (!dstNode)
    {
      std::cerr << PHWHERE << "DST Node missing, doing nothing." << std::endl;
      throw std::runtime_error("Failed to find DST node in CrystalCalorimeterDigitization::CreateNodes");
    }

  // Create the output tower node on the tree
  _towersDigi = new RawTowerContainer();

  PHIODataNode<PHObject> *towerNode = new PHIODataNode<PHObject>(_towersDigi, _nodeNameTowerDigi.c_str(), "PHObject");
  dstNode->addNode(towerNode);

  return;
}


void
CrystalCalorimeterDigitization::ApplyPhotonStatistic( RawTowerv1& tower )
{
  /* Use Poisson statistics for photon statistic smearing */
  float nPhotonsMean = tower.get_energy();
  float nPhotonsRand = _dice->RollPoisson( nPhotonsMean );

  /* set tower energy to number of photons */
  tower.Reset();
  tower.add_ecell( 1 , nPhotonsRand );

  return;
}
