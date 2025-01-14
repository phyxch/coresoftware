#ifndef PHG4InnerHcalDetector_h
#define PHG4InnerHcalDetector_h

#include "PHG4InnerHcalParameters.h"
#include <g4main/PHG4Detector.h>

#include <Geant4/globals.hh>
#include <Geant4/G4Types.hh>
#include <Geant4/G4SystemOfUnits.hh>
#include <Geant4/G4RotationMatrix.hh>

#include <CGAL/Exact_circular_kernel_2.h>
#include <CGAL/point_generators_2.h>

#include <map>
#include <vector>
#include <set>

class G4AssemblyVolume;
class G4LogicalVolume;
class G4VPhysicalVolume;
class G4VSolid;
//class PHG4InnerHcalParameters;

class PHG4InnerHcalDetector: public PHG4Detector
{
typedef CGAL::Exact_circular_kernel_2             Circular_k;
typedef CGAL::Point_2<Circular_k>                 Point_2;

  public:

  //! constructor
 PHG4InnerHcalDetector( PHCompositeNode *Node,  PHG4InnerHcalParameters *parameters, const std::string &dnam);

  //! destructor
  virtual ~PHG4InnerHcalDetector(){}

  //! construct
  virtual void Construct( G4LogicalVolume* world );

  virtual void Print(const std::string &what = "ALL") const;

  //!@name volume accessors
  //@{
  int IsInInnerHcal(G4VPhysicalVolume*) const;
  //@}

  int IsActive() const {return params->IsActive();}
  void SuperDetector(const std::string &name) {superdetector = name;}
  const std::string SuperDetector() const {return superdetector;}
  int get_Layer() const {return layer;}

  int IsBlackHole() const {return params->IsBlackHole();}

  G4VSolid* ConstructSteelPlate(G4LogicalVolume* hcalenvelope);
  G4VSolid* ConstructScintillatorBox(G4LogicalVolume* hcalenvelope);
  void ShiftSecantToTangent(Point_2 &lowleft, Point_2 &upleft, Point_2 &upright, Point_2 &lowright);

  G4AssemblyVolume *ConstructHcalScintillatorAssembly(G4LogicalVolume* hcalenvelope);
  void ConstructHcalSingleScintillators(G4LogicalVolume* hcalenvelope);
  int CheckTiltAngle() const;
  int ConsistencyCheck() const;
  void SetTiltViaNcross();

  protected:
  void AddGeometryNode();
  int ConstructInnerHcal(G4LogicalVolume* sandwich);
  int DisplayVolume(G4VSolid *volume,  G4LogicalVolume* logvol, G4RotationMatrix* rotm=NULL);
  G4double x_at_y(Point_2 &p0, Point_2 &p1, G4double yin);
  PHG4InnerHcalParameters *params;
  G4double scinti_tile_x;
  G4double scinti_tile_x_lower;
  G4double scinti_tile_x_upper;
  G4double scinti_tile_z;
  G4double envelope_inner_radius;
  G4double envelope_outer_radius;
  G4double envelope_z;
  double volume_envelope;
  double volume_steel;
  double volume_scintillator;

  int layer;
  std::string detector_type;
  std::string superdetector;
  std::set<G4VPhysicalVolume *>steel_absorber_vec;
  std::vector<G4VSolid *> scinti_tiles_vec; 
  std::string scintilogicnameprefix;
};

#endif
