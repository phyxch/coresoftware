// $$Id: PHG4FullProjSpacalDetector.cc,v 1.3 2015/02/10 15:39:26 pinkenbu Exp $$

/*!
 * \file ${file_name}
 * \brief
 * \author Jin Huang <jhuang@bnl.gov>
 * \version $$Revision: 1.3 $$
 * \date $$Date: 2015/02/10 15:39:26 $$
 */
#include "PHG4FullProjSpacalDetector.h"
#include "PHG4CylinderGeomContainer.h"
#include "PHG4CylinderGeom_Spacalv1.h"

#include <g4main/PHG4PhenixDetector.h>
#include <g4main/PHG4Utils.h>

#include <phool/PHCompositeNode.h>
#include <phool/PHIODataNode.h>
#include <phool/getClass.h>

#include <Geant4/G4Trd.hh>
#include <Geant4/G4Trap.hh>
#include <Geant4/G4Box.hh>
#include <Geant4/G4Colour.hh>
#include <Geant4/G4Cons.hh>
#include <Geant4/G4ExtrudedSolid.hh>
#include <Geant4/G4LogicalVolume.hh>
#include <Geant4/G4UserLimits.hh>
#include <Geant4/G4Material.hh>
#include <Geant4/G4PhysicalConstants.hh>
#include <Geant4/G4PVPlacement.hh>
#include <Geant4/G4SubtractionSolid.hh>
#include <Geant4/G4SystemOfUnits.hh>
#include <Geant4/G4Tubs.hh>
#include <Geant4/G4TwoVector.hh>
#include <Geant4/G4UnionSolid.hh>
#include <Geant4/G4VisAttributes.hh>
#include <Geant4/G4Vector3D.hh>

#include <numeric>      // std::accumulate
#include <cassert>
#include <cmath>
#include <functional>
#include <algorithm>
#include <string>     // std::string, std::to_string
#include <sstream>
#include <boost/math/special_functions/sign.hpp>
#include <boost/foreach.hpp>

using namespace std;

//_______________________________________________________________
//note this inactive thickness is ~1.5% of a radiation length
PHG4FullProjSpacalDetector::PHG4FullProjSpacalDetector(PHCompositeNode *Node,
    const std::string &dnam, SpacalGeom_t * geom, const int lyr) :
    PHG4SpacalDetector(Node, dnam,
        dynamic_cast<PHG4SpacalDetector::SpacalGeom_t *>(geom), lyr), //
    _geom(geom) //
{

  if (_geom == NULL)
    {
      cout
          << "PHG4SpacalDetector::Constructor - Fatal Error - invalid geometry object!"
          << endl;
      exit(1);
    }

  step_limits = new G4UserLimits(_geom->get_calo_step_size() * cm);

  clading_step_limits = new G4UserLimits(
      _geom->get_fiber_clading_step_size() * cm);

  fiber_core_step_limits = new G4UserLimits(
      _geom->get_fiber_core_step_size() * cm);
}

//_______________________________________________________________
void
PHG4FullProjSpacalDetector::Construct(G4LogicalVolume* logicWorld)
{
  assert(_geom);

  if (_geom->get_construction_verbose() >= 1)
    {
      cout << "PHG4FullProjSpacalDetector::Construct::" << GetName()
          << " - start with PHG4SpacalDetector::Construct()." << endl;
    }

  PHG4SpacalDetector::Construct(logicWorld);

  if (_geom->get_construction_verbose() >= 1)
    {
      cout << "PHG4FullProjSpacalDetector::Construct::" << GetName()
          << " - Completed." << endl;
    }

}

std::pair<G4LogicalVolume *, G4Transform3D>
PHG4FullProjSpacalDetector::Construct_AzimuthalSeg()
{
  assert(_geom);
  assert(_geom->get_azimuthal_n_sec()>4);

  G4Tubs* sec_solid = new G4Tubs(G4String(GetName() + string("_sec")),
      _geom->get_radius() * cm, _geom->get_max_radius() * cm,
      _geom->get_length() * cm / 2.0,
      halfpi - pi / _geom->get_azimuthal_n_sec(),
      twopi / _geom->get_azimuthal_n_sec());

  G4Material * cylinder_mat = G4Material::GetMaterial("G4_AIR");
  assert(cylinder_mat);

  G4LogicalVolume * sec_logic = new G4LogicalVolume(sec_solid, cylinder_mat,
      G4String(G4String(GetName() + string("_sec"))), 0, 0, step_limits);

  G4VisAttributes* VisAtt = new G4VisAttributes();
  VisAtt->SetColor(.5, .9, .5, .5);
  VisAtt->SetVisibility(
      _geom->is_azimuthal_seg_visible() or _geom->is_virualize_fiber());
  VisAtt->SetForceSolid(false);
  VisAtt->SetForceWireframe(true);
  sec_logic->SetVisAttributes(VisAtt);

  // construct walls

  G4Material * wall_mat = G4Material::GetMaterial(_geom->get_sidewall_mat());
  assert(wall_mat);

  G4VisAttributes* wall_VisAtt = new G4VisAttributes();
  wall_VisAtt->SetColor(.5, .9, .5, .2);
  wall_VisAtt->SetVisibility(
      _geom->is_azimuthal_seg_visible() and (not _geom->is_virualize_fiber()));
  wall_VisAtt->SetForceSolid(true);

  if (_geom->get_sidewall_thickness()>0)
    {
      // end walls
      if (_geom->get_construction_verbose() >= 1)
        {
          cout << "PHG4FullProjSpacalDetector::Construct_AzimuthalSeg::" << GetName()
              << " - construct end walls." << endl;
        }
      G4Tubs* wall_solid = new G4Tubs(G4String(GetName() + string("_EndWall")),
          _geom->get_radius() * cm + _geom->get_sidewall_outer_torr() * cm,
          _geom->get_max_radius() * cm - _geom->get_sidewall_outer_torr() * cm,
          _geom->get_sidewall_thickness() * cm / 2.0,
          halfpi - pi / _geom->get_azimuthal_n_sec(),
          twopi / _geom->get_azimuthal_n_sec());

      G4LogicalVolume * wall_logic = new G4LogicalVolume(wall_solid, wall_mat,
          G4String(G4String(GetName() + string("_EndWall"))), 0, 0,
          step_limits);
      wall_logic->SetVisAttributes(wall_VisAtt);

      typedef map<int, double> z_locations_t;
      z_locations_t z_locations;
      z_locations[1000] = _geom->get_sidewall_thickness() * cm / 2.0
          + _geom->get_assembly_spacing() * cm;
      z_locations[1001] = _geom->get_length() * cm / 2.0
          - (_geom->get_sidewall_thickness() * cm / 2.0
              + _geom->get_assembly_spacing() * cm);
      z_locations[1100] = -(_geom->get_sidewall_thickness() * cm / 2.0
          + _geom->get_assembly_spacing() * cm);
      z_locations[1101] = -(_geom->get_length() * cm / 2.0
          - (_geom->get_sidewall_thickness() * cm / 2.0
              + _geom->get_assembly_spacing() * cm));

      BOOST_FOREACH(z_locations_t::value_type& val, z_locations)
        {
          if (_geom->get_construction_verbose() >= 2)
            cout << "PHG4FullProjSpacalDetector::Construct_AzimuthalSeg::"
                << GetName() << " - constructed End Wall ID " << val.first
                << " @ Z = " << val.second << endl;

          G4Transform3D wall_trans = G4TranslateZ3D(val.second);

          G4PVPlacement * wall_phys = new G4PVPlacement(wall_trans, wall_logic,
              G4String(GetName().c_str()) + G4String("_EndWall"), sec_logic,
              false, val.first, overlapcheck);

          calo_vol[wall_phys] = val.first;
        }
    }

  if (_geom->get_sidewall_thickness()>0)
    {
      // side walls
      if (_geom->get_construction_verbose() >= 1)
        {
          cout << "PHG4FullProjSpacalDetector::Construct_AzimuthalSeg::" << GetName()
              << " - construct side walls." << endl;
        }
      G4Box* wall_solid = new G4Box(G4String(GetName() + string("_SideWall")),
          _geom->get_sidewall_thickness() * cm / 2.0,
          _geom->get_thickness() * cm / 2.
              - 2 * _geom->get_sidewall_outer_torr() * cm,
          (_geom->get_length() / 2.
              - 2
                  * (_geom->get_sidewall_thickness()
                      + 2. * _geom->get_assembly_spacing())) * cm * .5);

      G4LogicalVolume * wall_logic = new G4LogicalVolume(wall_solid, wall_mat,
          G4String(G4String(GetName() + string("_SideWall"))), 0, 0,
          step_limits);
      wall_logic->SetVisAttributes(wall_VisAtt);

      typedef map<int, pair<int, int> > sign_t;
      sign_t signs;
      signs[2000] = make_pair<int, int>(+1, +1);
      signs[2001] = make_pair<int, int>(+1, -1);
      signs[2100] = make_pair<int, int>(-1, +1);
      signs[2101] = make_pair<int, int>(-1, -1);

      BOOST_FOREACH(sign_t::value_type& val, signs)
        {
          const int sign_z = val.second.first;
          const int sign_azimuth = val.second.second;

          if (_geom->get_construction_verbose() >= 2)
            cout << "PHG4FullProjSpacalDetector::Construct_AzimuthalSeg::"
                << GetName() << " - constructed Side Wall ID " << val.first
                << " with" << " Shift X = "
                << sign_azimuth
                    * (_geom->get_sidewall_thickness() * cm / 2.0
                        + _geom->get_sidewall_outer_torr() * cm)
                << " Rotation Z = "
                << sign_azimuth * pi / _geom->get_azimuthal_n_sec()
                << " Shift Z = " << sign_z * (_geom->get_length() * cm / 4)
                << endl;

          G4Transform3D wall_trans = G4RotateZ3D(
              sign_azimuth * pi / _geom->get_azimuthal_n_sec())
              * G4TranslateZ3D(sign_z * (_geom->get_length() * cm / 4))
              * G4TranslateY3D(
                  _geom->get_radius() * cm + _geom->get_thickness() * cm / 2.)
              * G4TranslateX3D(
                  sign_azimuth
                      * (_geom->get_sidewall_thickness() * cm / 2.0
                          + _geom->get_sidewall_outer_torr() * cm));

          G4PVPlacement * wall_phys = new G4PVPlacement(wall_trans, wall_logic,
              G4String(GetName().c_str()) + G4String("_EndWall"), sec_logic,
              false, val.first, overlapcheck);

          calo_vol[wall_phys] = val.first;
        }
    }

  // construct towers

  BOOST_FOREACH(const SpacalGeom_t::tower_map_t::value_type& val, _geom->get_sector_tower_map())
    {
      const SpacalGeom_t::geom_tower & g_tower = val.second;
      G4LogicalVolume* LV_tower = Construct_Tower(g_tower);

      G4Transform3D block_trans = G4TranslateX3D(g_tower.centralX * cm)
          * G4TranslateY3D(g_tower.centralY * cm)
          * G4TranslateZ3D(g_tower.centralZ * cm)
          * G4RotateX3D(g_tower.pRotationAngleX * rad);

      const bool overlapcheck_block = overlapcheck
          and (_geom->get_construction_verbose() >= 2);

      G4PVPlacement * block_phys = new G4PVPlacement(block_trans, LV_tower,
          G4String(GetName().c_str()) + G4String("_Tower"), sec_logic, false,
          g_tower.id, overlapcheck_block);
      block_vol[block_phys] = g_tower.id;

    }

  cout << "PHG4FullProjSpacalDetector::Construct_AzimuthalSeg::" << GetName()
      << " - constructed " << _geom->get_sector_tower_map().size()
      << " unique towers" << endl;

  return make_pair(sec_logic, G4Transform3D::Identity);
}

//! Fully projective spacal with 2D tapered modules. To speed up construction, same-length fiber is used cross one tower
int
PHG4FullProjSpacalDetector::Construct_Fibers_SameLengthFiberPerTower(
    const PHG4FullProjSpacalDetector::SpacalGeom_t::geom_tower & g_tower,
    G4LogicalVolume* LV_tower)
{
  assert(_geom);

  // construct fibers

  // first check out the fibers geometry

  typedef map<int, pair<G4Vector3D, G4Vector3D> > fiber_par_map;
  fiber_par_map fiber_par;
  G4double min_fiber_length = g_tower.pDz * cm * 4;

  G4Vector3D v_zshift = G4Vector3D(tan(g_tower.pTheta) * cos(g_tower.pPhi),
      tan(g_tower.pTheta) * sin(g_tower.pPhi), 1) * g_tower.pDz;
  int fiber_ID = 0;
  for (int ix = 0; ix < g_tower.NFiberX; ix++)
//  int ix = 0;
    {
      const double weighted_ix = static_cast<double>(ix)
          / (g_tower.NFiberX - 1.);

      const double weighted_pDx1 = (g_tower.pDx1 - g_tower.ModuleSkinThickness
          - _geom->get_fiber_outer_r()) * (weighted_ix * 2 - 1);
      const double weighted_pDx2 = (g_tower.pDx2 - g_tower.ModuleSkinThickness
          - _geom->get_fiber_outer_r()) * (weighted_ix * 2 - 1);

      const double weighted_pDx3 = (g_tower.pDx3 - g_tower.ModuleSkinThickness
          - _geom->get_fiber_outer_r()) * (weighted_ix * 2 - 1);
      const double weighted_pDx4 = (g_tower.pDx4 - g_tower.ModuleSkinThickness
          - _geom->get_fiber_outer_r()) * (weighted_ix * 2 - 1);

      for (int iy = 0; iy < g_tower.NFiberY; iy++)
//        int iy = 0;
        {
          if ((ix + iy) % 2 == 1)
            continue; // make a triangle pattern


          const double weighted_iy = static_cast<double>(iy)
              / (g_tower.NFiberY - 1.);

          const double weighted_pDy1 = (g_tower.pDy1
              - g_tower.ModuleSkinThickness - _geom->get_fiber_outer_r())
              * (weighted_iy * 2 - 1);
          const double weighted_pDy2 = (g_tower.pDy2
              - g_tower.ModuleSkinThickness - _geom->get_fiber_outer_r())
              * (weighted_iy * 2 - 1);

          const double weighted_pDx12 = weighted_pDx1 * (1-weighted_iy)
              + weighted_pDx2 * ( weighted_iy)
              + weighted_pDy1 * tan(g_tower.pAlp1);
          const double weighted_pDx34 = weighted_pDx3 * (1-weighted_iy)
              + weighted_pDx4 * (weighted_iy)
              + weighted_pDy1 * tan(g_tower.pAlp2);

          G4Vector3D v1 = G4Vector3D(weighted_pDx12, weighted_pDy1, 0)
              - v_zshift;
          G4Vector3D v2 = G4Vector3D(weighted_pDx34, weighted_pDy2, 0)
              + v_zshift;

          G4Vector3D vector_fiber = (v2 - v1);
          vector_fiber *= (vector_fiber.mag() - _geom->get_fiber_outer_r())
              / vector_fiber.mag(); // shrink by fiber boundary protection
          G4Vector3D center_fiber = (v2 + v1) / 2;

          // convert to Geant4 units
          vector_fiber *= cm;
          center_fiber *= cm;

          fiber_par[fiber_ID] = make_pair<G4Vector3D, G4Vector3D>(vector_fiber,
              center_fiber);

          const G4double fiber_length = vector_fiber.mag();

          min_fiber_length = min(fiber_length, min_fiber_length);

          ++fiber_ID;
        }
    }

  int fiber_count = 0;

  const G4double fiber_length = min_fiber_length;
  vector<G4double> fiber_cut;

  stringstream ss;
  ss << string("_Tower") << g_tower.id;
  G4LogicalVolume *fiber_logic = Construct_Fiber(fiber_length, ss.str());

  BOOST_FOREACH(const fiber_par_map::value_type& val, fiber_par)
    {
      const int fiber_ID = val.first;
      G4Vector3D vector_fiber = val.second.first;
      G4Vector3D center_fiber = val.second.second;
      const G4double optimal_fiber_length = vector_fiber.mag();

      const G4Vector3D v1 = center_fiber - 0.5 *vector_fiber;

      // keep a statistics
      assert(optimal_fiber_length-fiber_length>=0);
      fiber_cut.push_back(optimal_fiber_length - fiber_length);

      center_fiber += (fiber_length / optimal_fiber_length - 1) * 0.5
          * vector_fiber;
      vector_fiber *= fiber_length / optimal_fiber_length;

//      const G4Vector3D v1_new = center_fiber - 0.5 *vector_fiber;

      if (_geom->get_construction_verbose() >= 3)
        cout << "PHG4FullProjSpacalDetector::Construct_Fibers_SameLengthFiberPerTower::" << GetName()
            << " - constructed fiber " << fiber_ID << ss.str()            //
            << ", Length = " << optimal_fiber_length << "-"
            << (optimal_fiber_length - fiber_length) << "mm, " //
            << "x = " << center_fiber.x() << "mm, " //
            << "y = " << center_fiber.y() << "mm, " //
            << "z = " << center_fiber.z() << "mm, " //
            << "vx = " << vector_fiber.x() << "mm, " //
            << "vy = " << vector_fiber.y() << "mm, " //
            << "vz = " << vector_fiber.z() << "mm, " //
            << endl;

      const G4double rotation_angle = G4Vector3D(0, 0, 1).angle(vector_fiber);
      const G4Vector3D rotation_axis =
          rotation_angle == 0 ?
              G4Vector3D(1, 0, 0) : G4Vector3D(0, 0, 1).cross(vector_fiber);

      G4Transform3D fiber_place(
          G4Translate3D(center_fiber.x(), center_fiber.y(), center_fiber.z())
              * G4Rotate3D(rotation_angle, rotation_axis));

      stringstream name;
      name << GetName() + string("_Tower") << g_tower.id << "_fiber"
          << ss.str();

      const bool overlapcheck_fiber = overlapcheck
          and (_geom->get_construction_verbose() >= 3);
      G4PVPlacement * fiber_physi = new G4PVPlacement(fiber_place, fiber_logic,
          G4String(name.str().c_str()), LV_tower, false, fiber_ID,
          overlapcheck_fiber);
      fiber_vol[fiber_physi] = fiber_ID;

      fiber_count++;

    }

  if (_geom->get_construction_verbose() >= 2)
    cout
        << "PHG4FullProjSpacalDetector::Construct_Fibers_SameLengthFiberPerTower::"
        << GetName() << " - constructed tower ID " << g_tower.id << " with "
        << fiber_count << " fibers. Average fiber length cut = "
        << accumulate(fiber_cut.begin(), fiber_cut.end(), 0.0)
            / fiber_cut.size() << " mm" << endl;

  return fiber_count;
}

//! a block along z axis built with G4Trd that is slightly tapered in x dimension
int
PHG4FullProjSpacalDetector::Construct_Fibers(
    const PHG4FullProjSpacalDetector::SpacalGeom_t::geom_tower & g_tower,
    G4LogicalVolume* LV_tower)
{
  assert(_geom);

  G4Vector3D v_zshift = G4Vector3D(tan(g_tower.pTheta) * cos(g_tower.pPhi),
      tan(g_tower.pTheta) * sin(g_tower.pPhi), 1) * g_tower.pDz;
  int fiber_ID = 0;
  for (int ix = 0; ix < g_tower.NFiberX; ix++)
    {
      const double weighted_ix = static_cast<double>(ix)
          / (g_tower.NFiberX - 1.);

      const double weighted_pDx1 = (g_tower.pDx1 - g_tower.ModuleSkinThickness
          - _geom->get_fiber_outer_r()) * (weighted_ix * 2 - 1);
      const double weighted_pDx2 = (g_tower.pDx2 - g_tower.ModuleSkinThickness
          - _geom->get_fiber_outer_r()) * (weighted_ix * 2 - 1);

      const double weighted_pDx3 = (g_tower.pDx3 - g_tower.ModuleSkinThickness
          - _geom->get_fiber_outer_r()) * (weighted_ix * 2 - 1);
      const double weighted_pDx4 = (g_tower.pDx4 - g_tower.ModuleSkinThickness
          - _geom->get_fiber_outer_r()) * (weighted_ix * 2 - 1);

      for (int iy = 0; iy < g_tower.NFiberY; iy++)
        {
          if ((ix + iy) % 2 == 1)
            continue; // make a triangle pattern


          const double weighted_iy = static_cast<double>(iy)
              / (g_tower.NFiberY - 1.);

          const double weighted_pDy1 = (g_tower.pDy1
              - g_tower.ModuleSkinThickness - _geom->get_fiber_outer_r())
              * (weighted_iy * 2 - 1);
          const double weighted_pDy2 = (g_tower.pDy2
              - g_tower.ModuleSkinThickness - _geom->get_fiber_outer_r())
              * (weighted_iy * 2 - 1);

          const double weighted_pDx12 = weighted_pDx1 * (1-weighted_iy)
              + weighted_pDx2 * ( weighted_iy)
              + weighted_pDy1 * tan(g_tower.pAlp1);
          const double weighted_pDx34 = weighted_pDx3 * (1-weighted_iy)
              + weighted_pDx4 * (weighted_iy)
              + weighted_pDy1 * tan(g_tower.pAlp2);

          G4Vector3D v1 = G4Vector3D(weighted_pDx12, weighted_pDy1, 0)
              - v_zshift;
          G4Vector3D v2 = G4Vector3D(weighted_pDx34, weighted_pDy2, 0)
              + v_zshift;

          G4Vector3D vector_fiber = (v2 - v1);
          vector_fiber *= (vector_fiber.mag() - _geom->get_fiber_outer_r())
              / vector_fiber.mag(); // shrink by fiber boundary protection
          G4Vector3D center_fiber = (v2 + v1) / 2;

          // convert to Geant4 units
          vector_fiber *= cm;
          center_fiber *= cm;

          const G4double fiber_length = vector_fiber.mag();

          stringstream ss;
          ss << string("_Tower") << g_tower.id;
          ss << "_x" << ix;
          ss << "_y" << iy;
          G4LogicalVolume *fiber_logic = Construct_Fiber(fiber_length,
              ss.str());

          if (_geom->get_construction_verbose() >= 3)
            cout << "PHG4FullProjSpacalDetector::Construct_Fibers::" << GetName()
            << " - constructed fiber " << fiber_ID << ss.str()            //
                << ", Length = " << fiber_length << "mm, " //
                << "x = " << center_fiber.x() << "mm, " //
                << "y = " << center_fiber.y() << "mm, " //
                << "z = " << center_fiber.z() << "mm, " //
                << "vx = " << vector_fiber.x() << "mm, " //
                << "vy = " << vector_fiber.y() << "mm, " //
                << "vz = " << vector_fiber.z() << "mm, " //
                << endl;

          const G4double rotation_angle = G4Vector3D(0, 0, 1).angle(
              vector_fiber);
          const G4Vector3D rotation_axis =
              rotation_angle == 0 ?
                  G4Vector3D(1, 0, 0) : G4Vector3D(0, 0, 1).cross(vector_fiber);

          G4Transform3D fiber_place(
              G4Translate3D(center_fiber.x(), center_fiber.y(),
                  center_fiber.z())
                  * G4Rotate3D(rotation_angle, rotation_axis));

          stringstream name;
          name << GetName() + string("_Tower") << g_tower.id << "_fiber"
              << ss.str();

          const bool overlapcheck_fiber = overlapcheck
              and (_geom->get_construction_verbose() >= 3);
          G4PVPlacement * fiber_physi = new G4PVPlacement(fiber_place,
              fiber_logic, G4String(name.str().c_str()), LV_tower, false,
              fiber_ID, overlapcheck_fiber);
          fiber_vol[fiber_physi] = fiber_ID;

          ++fiber_ID;
        }
    }

  if (_geom->get_construction_verbose() >= 3)
    cout << "PHG4FullProjSpacalDetector::Construct_Fibers::" << GetName()
        << " - constructed tower ID " << g_tower.id << " with " << fiber_ID
        << " fibers" << endl;

  return fiber_ID;
}

//! a block along z axis built with G4Trd that is slightly tapered in x dimension
G4LogicalVolume*
PHG4FullProjSpacalDetector::Construct_Tower(
    const PHG4FullProjSpacalDetector::SpacalGeom_t::geom_tower & g_tower)
{
  assert(_geom);

  std::stringstream sout;
  sout << "_" << g_tower.id;
  const G4String sTowerID(sout.str());

  //Processed PostionSeeds 1 from 1 1

  G4Trap* block_solid = new G4Trap(
  /*const G4String& pName*/G4String(GetName().c_str()) + sTowerID,
      g_tower.pDz * cm, // G4double pDz,
      g_tower.pTheta * rad, g_tower.pPhi * rad, // G4double pTheta, G4double pPhi,
      g_tower.pDy1 * cm, g_tower.pDx1 * cm, g_tower.pDx2 * cm, // G4double pDy1, G4double pDx1, G4double pDx2,
      g_tower.pAlp1 * rad, // G4double pAlp1,
      g_tower.pDy2 * cm, g_tower.pDx3 * cm, g_tower.pDx4 * cm, // G4double pDy2, G4double pDx3, G4double pDx4,
      g_tower.pAlp2 * rad // G4double pAlp2 //
          );

  G4Material * cylinder_mat = G4Material::GetMaterial(
      _geom->get_absorber_mat());
  assert(cylinder_mat);

  G4LogicalVolume * block_logic = new G4LogicalVolume(block_solid, cylinder_mat,
      G4String(G4String(GetName()) + string("_Tower") + sTowerID), 0, 0,
      step_limits);

  G4VisAttributes* VisAtt = new G4VisAttributes();
//  PHG4Utils::SetColour(VisAtt, "W_Epoxy");
  VisAtt->SetColor(.3, .3, .3, .3);
  VisAtt->SetVisibility(
      _geom->is_azimuthal_seg_visible() or _geom->is_virualize_fiber());
  VisAtt->SetForceSolid(not _geom->is_virualize_fiber());
  block_logic->SetVisAttributes(VisAtt);

  // construct fibers

  int fiber_count = 0;

  if (_geom->get_config() == SpacalGeom_t::kFullProjective_2DTaper)
    {
      fiber_count = Construct_Fibers(g_tower, block_logic);

      if (_geom->get_construction_verbose() >= 2)
        cout << "PHG4FullProjSpacalDetector::Construct_Tower::" << GetName()
            << " - constructed tower ID " << g_tower.id << " with "
            << fiber_count << " fibers using Construct_Fibers" << endl;
    }
  else if (_geom->get_config()
      == SpacalGeom_t::kFullProjective_2DTaper_SameLengthFiberPerTower)
    {
      fiber_count = Construct_Fibers_SameLengthFiberPerTower(g_tower,
          block_logic);

      if (_geom->get_construction_verbose() >= 2)
        cout << "PHG4FullProjSpacalDetector::Construct_Tower::" << GetName()
            << " - constructed tower ID " << g_tower.id << " with "
            << fiber_count
            << " fibers using Construct_Fibers_SameLengthFiberPerTower" << endl;
    }
  else
    {

      G4ExceptionDescription message;
      message << "can not recognize configuration type " << _geom->get_config();

      G4Exception("PHG4FullProjSpacalDetector::Construct_Tower", "Wrong",
          FatalException, message, "");
    }

  return block_logic;
}

void
PHG4FullProjSpacalDetector::Print(const std::string &what) const
{
  cout << "PHG4FullProjSpacalDetector::Print::" << GetName()
      << " - Print Geometry:" << endl;
  _geom->Print();

  return;
}
