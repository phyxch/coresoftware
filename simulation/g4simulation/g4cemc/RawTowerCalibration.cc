#include "RawTowerCalibration.h"
#include "RawTowerContainer.h"
#include "RawTowerGeomv2.h"
#include "RawTowerv1.h"
#include <g4detectors/PHG4CylinderCellGeomContainer.h>
#include <g4detectors/PHG4CylinderCellGeom.h>
#include <g4detectors/PHG4CylinderCellContainer.h>
#include <g4detectors/PHG4CylinderCell.h>
#include <g4detectors/PHG4CylinderCellDefs.h>
#include <phool/PHCompositeNode.h>
#include <phool/PHNodeIterator.h>
#include <phool/PHIODataNode.h>
#include <fun4all/Fun4AllReturnCodes.h>
#include <phool/getClass.h>

#include <iostream>
#include <stdexcept>
#include <map>
#include <cassert>

using namespace std;

RawTowerCalibration::RawTowerCalibration(const std::string& name) :
    SubsysReco(name), _calib_algorithm(kNo_calibration), //
    _calib_towers(NULL), _raw_towers(NULL), //
    rawtowergeom(NULL), //
    detector("NONE"), //
    _calib_tower_node_prefix("CALIB"), _raw_tower_node_prefix("RAW"), //
    //! pedstal in unit of ADC
    _pedstal_ADC(NAN),
    //! calibration constant in unit of GeV per ADC
    _calib_const_GeV_ADC(NAN), //
    _zero_suppression_GeV(0), //
    _timer(PHTimeServer::get()->insert_new(name))
{
}

int
RawTowerCalibration::InitRun(PHCompositeNode *topNode)
{
  PHNodeIterator iter(topNode);

  // Looking for the DST node
  PHCompositeNode *dstNode;
  dstNode = dynamic_cast<PHCompositeNode*>(iter.findFirst("PHCompositeNode",
      "DST"));
  if (!dstNode)
    {
      std::cout << Name() << "::" << detector << "::" << __PRETTY_FUNCTION__
          << "DST Node missing, doing nothing." << std::endl;
      exit(1);
    }

  try
    {
      CreateNodes(topNode);
    }
  catch (std::exception& e)
    {
      std::cout << e.what() << std::endl;
      return Fun4AllReturnCodes::ABORTRUN;
    }
  return Fun4AllReturnCodes::EVENT_OK;
}

int
RawTowerCalibration::process_event(PHCompositeNode *topNode)
{
  if (verbosity)
    {
      std::cout << Name() << "::" << detector << "::" << __PRETTY_FUNCTION__<< "Process event entered" << std::endl;
    }

  RawTowerContainer::ConstRange begin_end = _raw_towers->getTowers();
  RawTowerContainer::ConstIterator rtiter;
  for (rtiter = begin_end.first; rtiter != begin_end.second; ++rtiter)
    {
      const RawTowerDefs::keytype key = rtiter->first;
      const RawTower *raw_tower = rtiter->second;
      assert(raw_tower);

      if (_calib_algorithm == kNo_calibration)
        {
          if (raw_tower->get_energy() > _zero_suppression_GeV)
            {
              _calib_towers->AddTower(key, new RawTowerv1(*raw_tower));
            }
        }
      else if (_calib_algorithm == kSimple_linear_calibration)
        {
          const double raw_energy = raw_tower->get_energy();
          const double calib_energy = (raw_energy - _pedstal_ADC)
              * _calib_const_GeV_ADC;

          if (calib_energy > _zero_suppression_GeV)
            {
              RawTower *calib_tower = new RawTowerv1(*raw_tower);
              calib_tower->set_energy(calib_energy);
              _calib_towers->AddTower(key, calib_tower);
            }
        }
      else
        {
          std::cout << Name() << "::" << detector << "::" << __PRETTY_FUNCTION__
              << " invalid calibration algorithm #" << _calib_algorithm
              << std::endl;

          return Fun4AllReturnCodes::ABORTRUN;
        }
    } //  for (rtiter = begin_end.first; rtiter != begin_end.second; ++rtiter)

  if (verbosity)
    {
      std::cout << Name() << "::" << detector << "::" << __PRETTY_FUNCTION__
          << "input sum energy = " << _raw_towers->getTotalEdep()
          << ", output sum digitalized value = " << _calib_towers->getTotalEdep()
          << std::endl;
    }
  return Fun4AllReturnCodes::EVENT_OK;
}

int
RawTowerCalibration::End(PHCompositeNode *topNode)
{
  return Fun4AllReturnCodes::EVENT_OK;
}

void
RawTowerCalibration::CreateNodes(PHCompositeNode *topNode)
{

  PHNodeIterator iter(topNode);
  PHCompositeNode *runNode = static_cast<PHCompositeNode*>(iter.findFirst(
      "PHCompositeNode", "RUN"));
  if (!runNode)
    {
      std::cerr << Name() << "::" << detector << "::" << __PRETTY_FUNCTION__
          << "Run Node missing, doing nothing." << std::endl;
      throw std::runtime_error(
          "Failed to find Run node in RawTowerCalibration::CreateNodes");
    }

  TowerGeomNodeName = "TOWERGEOM_" + detector;
  rawtowergeom = findNode::getClass<RawTowerGeom>(topNode,
      TowerGeomNodeName.c_str());
  if (!rawtowergeom)
    {
      std::cerr << Name() << "::" << detector << "::" << __PRETTY_FUNCTION__
          << " " << TowerGeomNodeName << " Node missing, doing bail out!"
          << std::endl;
      throw std::runtime_error(
          "Failed to find " + TowerGeomNodeName
              + " node in RawTowerCalibration::CreateNodes");
    }

  if (verbosity >= 1)
    {
      rawtowergeom->identify();
    }

  PHCompositeNode *dstNode = dynamic_cast<PHCompositeNode*>(iter.findFirst(
      "PHCompositeNode", "DST"));
  if (!dstNode)
    {
      std::cerr << Name() << "::" << detector << "::" << __PRETTY_FUNCTION__
          << "DST Node missing, doing nothing." << std::endl;
      throw std::runtime_error(
          "Failed to find DST node in RawTowerCalibration::CreateNodes");
    }

  RawTowerNodeName = "TOWER_" + _raw_tower_node_prefix + "_" + detector;
  _raw_towers = findNode::getClass<RawTowerContainer>(dstNode,
      RawTowerNodeName.c_str());
  if (!_raw_towers)
    {
      std::cerr << Name() << "::" << detector << "::" << __PRETTY_FUNCTION__
          << " " << RawTowerNodeName << " Node missing, doing bail out!"
          << std::endl;
      throw std::runtime_error(
          "Failed to find " + RawTowerNodeName
              + " node in RawTowerCalibration::CreateNodes");
    }

  // Create the tower nodes on the tree
  PHNodeIterator dstiter(dstNode);
  PHCompositeNode *DetNode = dynamic_cast<PHCompositeNode*>(dstiter.findFirst(
      "PHCompositeNode", detector));
  if (!DetNode)
    {
      DetNode = new PHCompositeNode(detector);
      dstNode->addNode(DetNode);
    }

  _calib_towers = new RawTowerContainer();
  CaliTowerNodeName = "TOWER_" + _calib_tower_node_prefix + "_" + detector;
  PHIODataNode<PHObject> *towerNode = new PHIODataNode<PHObject>(_calib_towers,
      CaliTowerNodeName.c_str(), "PHObject");
  DetNode->addNode(towerNode);

  return;
}

