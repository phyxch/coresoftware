// This is G4 detector implementation for the 2nd hcal prototype.
// The structure is very similar to the 1st prototype
// Created on 11/20/2015, HeXC
// Updated on 12/7/2015, Hexc
// 

#include "PHG4HcalPrototype2Detector.h"

#include <g4main/PHG4Utils.h>

#include <phool/PHCompositeNode.h>
#include <phool/PHIODataNode.h>
#include <phool/getClass.h>

#include <Geant4/G4Material.hh>
#include <Geant4/G4Box.hh>
#include <Geant4/G4ExtrudedSolid.hh>
#include <Geant4/G4LogicalVolume.hh>
#include <Geant4/G4PVPlacement.hh>
#include <Geant4/G4Tubs.hh>
#include <Geant4/G4Box.hh>
#include <Geant4/G4UnionSolid.hh>
#include <Geant4/G4TwoVector.hh>
#include <Geant4/G4Trap.hh>

#include <Geant4/G4VisAttributes.hh>
#include <Geant4/G4Colour.hh>

#include <Geant4/G4NistManager.hh>
#include <Geant4/G4GeometryManager.hh>
#include <Geant4/G4PhysicalVolumeStore.hh>
#include <Geant4/G4LogicalVolumeStore.hh>
#include <Geant4/G4SolidStore.hh>

#include <Geant4/G4RunManager.hh>

#include <sstream>

using namespace std;

// added safety margin against overlaps by using same boundary between volumes
// static double no_overlap = 0.00015 * cm; 

static const double inch = 2.54 * cm;

PHG4HcalPrototype2Detector::PHG4HcalPrototype2Detector( PHCompositeNode *Node, 
							const std::string &dnam, 
							const int lyr  ):
  PHG4Detector(Node, dnam),
  active(0),
  absorberactive(0),
  layer(lyr),
  blackhole(0)
{

  // Outer Hcal scintillattor and absorber box dimensions
  // from Richie's drawing
  hcal2ScintSizeX = (2.54*32.68 + 0.15)*cm;    // add 0.15cm extra space; 1st prototype: 2.54*26.1*cm;   
  hcal2ScintSizeZ = (2.54*63.0 + 0.15)*cm;     // add 0.15cm extra space; 1st prototype: 2.54*29.53*cm;
  hcal2ScintSizeY = (7.0 + 26.2 + 1.5)*mm;     // sintilator thickness + thinner-end of the abs + 1.5 mm cushion

  // Inner Hcal scintillator box dimensions
  // from Richie's drawing
  hcal1ScintSizeX = (2.54*7.99 + 0.15)*cm;     // add 0.15cm extra space; 1st prototype: 2.54*12.43*cm;  
  hcal1ScintSizeZ = (2.54*35.50 + 0.15)*cm;    // add 0.15cm extra space; 1st prototype: 2.54*17.1*cm; 
  hcal1ScintSizeY = (7.0 + 10.28 + 1.5)*mm;    // sintilator thickness + thinner-end of the abs + 1.5 mm cushion 

  // Radius dimension
  hcal1RadiusIn = 1167.7*mm;          // 1st prototype: 1855*mm;
  hcal2RadiusIn = 1831*mm;            // 1st prototype: hcal1RadiusIn + hcal1ScintSizeX;

  nScint360 = 256;  // This is legacy variable in case one wants to build a cylindrical calorimeter 
  nHcal2Layers = nHcal1Layers = 20;   // 1st prototype: 16

  //hcal2DPhi = (2*12.5)*M_PI/180.;   //  = 0.436; 1st prototype: 0.288;   
  //hcal1DPhi = (2*12.5)*M_PI/180.;   //  = 0.436; 1st prototype: 0.27;  
  hcal2DPhi = (2*12.5)*M_PI/180.;   
  hcal1DPhi = (2*12.5)*M_PI/180.;    

  hcal2TiltAngle = 0.14;            // 1st prototype: 0.14;  // Tuned on 4/7/2014
  hcal1TiltAngle = 0.27;            // 1st prototype: 0.27;  // Tuned to this value on 3/21/2014

  // Outer HCal absorber dimenstion
  hcal2Abs_dxa = 2.54*32.68*cm;     // 1st prototye: 2.54*27.1*cm; 
  hcal2Abs_dxb = hcal2Abs_dxa;
  hcal2Abs_dya = 2.54*1.030*cm;     // 1st prototype: 2.54*1.099*cm;
  hcal2Abs_dyb = 2.54*1.672*cm;     // 1st prototype: 2.54*1.776*cm;  
  hcal2Abs_dz  = 2.54*63.0*cm;

  // Inner HCal absorber dimension
  hcal1Abs_dxa = 2.54*7.99*cm;   
  hcal1Abs_dxb = hcal1Abs_dxa;
  hcal1Abs_dya = 2.54*0.40*cm;      // 1st prototype: 2.54*0.787*cm;
  hcal1Abs_dyb = 2.54*0.56*cm;      // 1st prototype: 2.54*1.115*cm;    
  hcal1Abs_dz  = 2.54*35.50*cm;   

  cryostatSizeX = (1765.0 - 1420.1)*mm;  // from Richie's drawing
  cryostatSizeY = 30*hcal1ScintSizeY;    
  cryostatSizeZ = hcal1Abs_dz;

  // The frame box for the HCAL.
  // Make it 1% larger than the scintillator holder box for HCal2 (outer) in Y and Z directions
  // The length in X is determined from the depth of hcal1 (inner), hcal2 (outer), cryostat and the extra spaces in between
  hcalBoxSizeX = 1.1*(cryostatSizeX + hcal2ScintSizeX + hcal1ScintSizeX + 10*cm);
  hcalBoxSizeY = 1.01*42*hcal2ScintSizeY;      
  hcalBoxSizeZ = 1.01*hcal2ScintSizeZ; 

  hcalBoxRotationAngle_z = 0.0*rad;   // Rotation along z-axis
  hcalBoxRotationAngle_y = 0.0*rad;   // Rotation along y-axis

  // create commands for interactive definition of the detector  
  fDetectorMessenger = new PHG4HcalPrototype2DetectorMessenger(this);
}

//_______________________________________________________________
//_______________________________________________________________
int
PHG4HcalPrototype2Detector::IsInHcalPrototype2(G4VPhysicalVolume * volume) const
{
  return 0;  // not sure what value to return for now.
}

void PHG4HcalPrototype2Detector::Construct( G4LogicalVolume* world )
{
  //  physiWorld = world->GetSolid();`
  logicWorld = world;

  DefineMaterials();
  
  ConstructDetector();
}


void PHG4HcalPrototype2Detector::DefineMaterials()
{ 

  // Water is defined from NIST material database
  G4NistManager * nist = G4NistManager::Instance();
  
  // G4Material* Vacuum =
  //  new G4Material("Galactic", 1., 1.01*g/mole, CLHEP::universe_mean_density,
  //		   kStateGas, 2.73*kelvin, 3.e-18*pascal);
  
  steel = nist->FindOrBuildMaterial("G4_Fe");
  scint_mat = nist->FindOrBuildMaterial("G4_POLYSTYRENE");
  alum = nist->FindOrBuildMaterial("G4_Al");
  copper =  nist->FindOrBuildMaterial("G4_Cu");

  world_mat = nist->FindOrBuildMaterial("G4_AIR");

  G4cout << *(G4Material::GetMaterialTable()) << G4endl;
}

// We now build our detector solids, etc
// 
G4VPhysicalVolume*  PHG4HcalPrototype2Detector::ConstructDetector()
{

  // Option to switch on/off checking of volumes overlaps
  //
  G4bool checkOverlaps = true;

  // HCAL Frame Box for enclosing the inner and the outer hcal
  // which allows us to rotate the whole calorimeter easily
  solidHcalBox = new G4Box("HcalBox",				           //its name
			 hcalBoxSizeX/2,hcalBoxSizeY/2,hcalBoxSizeZ/2);    //its size

  logicHcalBox = new G4LogicalVolume(solidHcalBox,      //its solid
				     world_mat,	        //its material
				     "HcalBox");        //its name

  // Place the HcalBox inside the World
  // The idea is to fix the distance between the norminal center and the front face of the box.
  // This will help to properly position the absorbers and scintillators
  G4ThreeVector boxVector = G4ThreeVector(hcal1RadiusIn*cos(12.5*M_PI/180.) + hcalBoxSizeX/2.0,  0, 0); 
  G4RotationMatrix rotBox  = G4RotationMatrix();

  rotBox.rotateZ(hcalBoxRotationAngle_z);     // Rotate the whole calorimeter box along z-axis
  rotBox.rotateY(hcalBoxRotationAngle_y);     // Rotate the whole calorimeter box along z-axis

  G4Transform3D boxTransform = G4Transform3D(rotBox, boxVector);
  
  physiHcalBox = new G4PVPlacement(boxTransform,        // rotation + positioning
				   logicHcalBox,	//its logical volume
				   "HcalBox",		//its name
				   logicWorld,		//its mother  volume
				   false,		//no boolean operation
				   0,			//copy number
				   checkOverlaps);      //checking overlaps
  
  // HCal Box visualization attributes
  G4VisAttributes* hcalBoxVisAtt= new G4VisAttributes(G4Colour(0.0,0.0,1.0)); // blue
  hcalBoxVisAtt->SetVisibility(true);
  //hcalBoxVisAtt->SetForceSolid(true);
  logicHcalBox->SetVisAttributes(hcalBoxVisAtt);

  // Make outer hcal section
  // 
  // Build scintillator layer box as a place holder for the scintillator sheets
  // and the absorber plate
  // The commented lines below are from the first version.  
  /*
  G4Box* hcal2ScintLayer = new G4Box("hcal2ScintLayer",		     //its name
				     hcal2ScintSizeX/2,
				     (hcal2ScintSizeY-0.05*cm)/2,
				     hcal2ScintSizeZ/2);             //its size, make it 0.05cm thinner than the true gap width
  
  */

  // Use Trap volume for holding the scintilator tiles and the absorber
  G4Trap* hcal2ScintLayer = new G4Trap("hcal2ScintLayer",             //its name
				       hcal2Abs_dz, hcal2Abs_dxa, 
				       hcal2Abs_dyb + 2*0.35*cm + 0.1*mm,  // add 0.1*mm cushion
				       hcal2Abs_dya + 2*0.35*cm + 0.1*mm); // add 0,1*mm cushion

  logicHcal2ScintLayer = new G4LogicalVolume(hcal2ScintLayer,      // its solid name
					     world_mat,            // material, the same as the material of the world valume
					     "hcal2ScintLayer");   // its name
  
  // Scintillator visualization attributes
  G4VisAttributes* scintVisAtt= new G4VisAttributes(G4Colour(1.0,0.0,0.0));      //Red
  scintVisAtt->SetVisibility(true);
  //scintVisAtt->SetForceSolid(true);
  logicHcal2ScintLayer->SetVisAttributes(scintVisAtt);

  // Construct outer scintillator 1U
  G4double outer1UpDz = 828.9*mm;        // 1st prototype: 649.8*mm;   
  //  G4double outer1UpTheta = 0.0;
  //  G4double outer1UpPhi = 0.0;
  G4double outer1UpDy1 = 2*0.35*cm;
  //  G4double outer1UpDy2 = 0.35*cm;        // not used!
  //  G4double outer1UpDx1 = 0.25*179.3*mm;  // not used!
  G4double outer1UpDx2 = 240.54*mm;      // 1st prototype: 179.3*mm
  //  G4double outer1UpDx3 = 0.25*113.2*mm;  // not used!  
  G4double outer1UpDx4 = 166.25*mm;      // 1st prototype: 113.2*mm;
  //  G4double outer1UpAlp1 = 0., outer1UpAlp2 = 0.;

  G4Trap *outer1USheetSolid = new G4Trap("outer1USheet",                      //its name
					 outer1UpDy1, outer1UpDz, 
					 outer1UpDx2, outer1UpDx4);           //its size
  
  G4LogicalVolume *logicOuter1USheet = new G4LogicalVolume(outer1USheetSolid, 
							   scint_mat,
							   "outer1USheet");
  
  // Scintillator visualization attributes
  G4VisAttributes* scintSheetVisAtt= new G4VisAttributes(G4Colour(0.5,0.5,0.0)); //White
  scintSheetVisAtt->SetVisibility(true);
  scintSheetVisAtt->SetForceSolid(true);

  logicOuter1USheet->SetVisAttributes(scintSheetVisAtt);

  // Detector placement
  // Position the outer right 1U sheet
  //   Since we are putting the absorber (in trap shape) and the scintilator tiles inside a trap-shaped airbox,
  //   it is easier to put the scintilator tiles at the bottom of the airbox.
  G4ThreeVector threeVecOuter1U_1 = G4ThreeVector(-0.5*(hcal2Abs_dya + 2*0.35*cm + 0.1*mm), 
						  0.0*mm, 
						  -0.250*(outer1UpDx2+outer1UpDx4));
  G4RotationMatrix rotOuter1U_1  = G4RotationMatrix();
  rotOuter1U_1.rotateZ(90*deg);
  rotOuter1U_1.rotateX(-90*deg);
  rotOuter1U_1.rotateZ(-90*deg);

  G4Transform3D transformOuter1U_1 = G4Transform3D(rotOuter1U_1,threeVecOuter1U_1);

  new G4PVPlacement(transformOuter1U_1,
		    logicOuter1USheet,
		    "outer1USheet",
		    logicHcal2ScintLayer,
		    false,
		    0,                    // Copy one
		    checkOverlaps);
  
  // Detector placement
  // Position the outer left 1U sheet
  G4ThreeVector threeVecOuter1U_2 = G4ThreeVector(-0.5*(hcal2Abs_dya + 2*0.35*cm + 0.1*mm), 
						  0.0*mm, 
						  0.250*(outer1UpDx2+outer1UpDx4));
  G4RotationMatrix rotOuter1U_2  = G4RotationMatrix();
  rotOuter1U_2.rotateZ(90*deg);
  rotOuter1U_2.rotateX(90*deg);
  rotOuter1U_2.rotateZ(-90*deg);

  G4Transform3D transformOuter1U_2 = G4Transform3D(rotOuter1U_2,threeVecOuter1U_2);

  new G4PVPlacement(transformOuter1U_2,
		    logicOuter1USheet,
		    "outer1USheet",
		    logicHcal2ScintLayer,
		    false,
		    1,                      // Copy two
		    checkOverlaps);

  // Construct outer scintillator 2U 
  G4double outer2UpDz = 0.5*828.9*mm;        // 1st prototype: 0.5*649.8*mm;
  G4double outer2UpTheta = 7.69*M_PI/180.;    // forgot about how I got the number 8.8 
  G4double outer2UpPhi = 0.0*M_PI/180.;
  G4double outer2UpDy1 = 0.35*cm;
  G4double outer2UpDy2 = 0.35*cm;
  G4double outer2UpDx1 = 0.5*246.65;         // 1st prototype: 0.5*179.3*mm; 
  G4double outer2UpDx2 = 0.5*246.65;         // 1st prototype: 0.5*179.3*mm;
  G4double outer2UpDx3 = 0.5*171.5*mm;       // 1st prototype: 0.5*113.2*mm;
  G4double outer2UpDx4 = 0.5*171.5*mm;       // 1st prototype: 0.5*113.2*mm;
  G4double outer2UpAlp1 = 0.*M_PI/180.;
  G4double outer2UpAlp2 = 0.*M_PI/180.;

  G4Trap *outer2USheetSolid = new G4Trap("outer2USheet",
				  outer2UpDz,
				  outer2UpTheta,
				  outer2UpPhi, 
				  outer2UpDy1,
				  outer2UpDx1,
				  outer2UpDx2,
				  outer2UpAlp1,
				  outer2UpDy2,
				  outer2UpDx3,
				  outer2UpDx4,
				  outer2UpAlp2);

  G4LogicalVolume *logicOuter2USheet = new G4LogicalVolume(outer2USheetSolid, 
							 scint_mat,
							 "outer2USheet");

  logicOuter2USheet->SetVisAttributes(scintSheetVisAtt);
  
  // Detector placement
  // Position the left most sheet
  G4ThreeVector threeVecOuter2U_1 = G4ThreeVector(-0.5*(hcal2Abs_dya + 2*0.35*cm + 0.1*mm),
						  0.0*mm,
						  0.755*(outer1UpDx2+outer1UpDx4));
  G4RotationMatrix rotOuter2U_1  = G4RotationMatrix();
  rotOuter2U_1.rotateZ(90*deg);
  rotOuter2U_1.rotateX(-90*deg);

  G4Transform3D transformOuter2U_1 = G4Transform3D(rotOuter2U_1,threeVecOuter2U_1);

  new G4PVPlacement(transformOuter2U_1,
		    logicOuter2USheet,
		    "outer2USheet",
		    logicHcal2ScintLayer,
		    false,
		    0,                     // copy one
		    checkOverlaps);

  // Detector placement
  // Position the outer right most sheet
  G4ThreeVector threeVecOuter2U_2 = G4ThreeVector(-0.5*(hcal2Abs_dya + 2*0.35*cm + 0.1*mm), 
						  0.0*mm, 
						  -0.755*(outer1UpDx2+outer1UpDx4));
  G4RotationMatrix rotOuter2U_2  = G4RotationMatrix();
  rotOuter2U_2.rotateZ(-90*deg);
  rotOuter2U_2.rotateX(-90*deg);

  G4Transform3D transformOuter2U_2 = G4Transform3D(rotOuter2U_2,threeVecOuter2U_2);

  new G4PVPlacement(transformOuter2U_2,
		    logicOuter2USheet,
		    "outer2USheet",
		    logicHcal2ScintLayer,
		    false,
		    1,                     // copy two
		    checkOverlaps);  
  //
  // Build hcal2 absorber layer in trapezoid shape
  //         
  G4Trap* hcal2AbsLayer =    
    new G4Trap("hcal2AbsLayer",             //its name
	       hcal2Abs_dz, hcal2Abs_dxa, 
	       hcal2Abs_dyb, hcal2Abs_dya); //its size
   
  logicHcal2AbsLayer = new G4LogicalVolume(hcal2AbsLayer,    //  hcal2AbsLayer,
					   steel,
					   "hcal2AbsLayer");  
  // hcal2Aborber visualization attributes
  G4VisAttributes* hcal2AbsVisAtt= new G4VisAttributes(G4Colour(0.5,0.5,1.0)); 
  hcal2AbsVisAtt->SetVisibility(true);
  logicHcal2AbsLayer->SetVisAttributes(hcal2AbsVisAtt);
  
  // Detector placement
  // Position the hcal2Absorber in the holder box
  G4ThreeVector threeVecHcal2Abs = G4ThreeVector(2*0.35*cm, 0*cm, 0*cm);
  G4RotationMatrix rotHcal2Abs = G4RotationMatrix();
  //rotHcal2Abs.rotateZ(90*deg);
  //rotHcal2Abs.rotateX(180*deg);

  G4Transform3D transformHcal2Abs = G4Transform3D(rotHcal2Abs,threeVecHcal2Abs);

  new G4PVPlacement(transformHcal2Abs,
		    logicHcal2AbsLayer,
		    "hcal2AbsLayer",
		    logicHcal2ScintLayer,        // mother volume is the holder box
		    false,
		    0,        
		    checkOverlaps);  
  
  G4double theta = 0.;
  G4double theta2 = hcal2TiltAngle;   // I am not convinced that I got the tilt angle right.  So I simply forced it to 
  //G4double rScintLayerCenter = hcal2RadiusIn + hcal2ScintSizeX/2.0;  // move the scintillator toward the rear end of HCAL2
  //G4double rScintLayerCenter = hcal2RadiusIn;  // move the scintillator toward the rear end of HCAL2
  G4double rAbsLayerCenter = hcal2RadiusIn + hcal2Abs_dxa/2.0;
  //G4double RmidScintLayerX =  0.81*(hcal2ScintSizeX - hcal1ScintSizeX)/2.0;  // move the scintillator toward the rear end of HCAL2
  G4double RmidAbsLayerX =  hcal2Abs_dxa/2.0;
  G4int LayerNum = 1;
  G4double tiltPadding = 0.0;
  G4double xPadding = 0.0;
  G4double yPadding = 0.0;
  for (G4int iLayer = -nHcal2Layers/2; iLayer < nHcal2Layers/2; iLayer++) {
    //
    // Place outer hcal scintillator layer
    //
    theta = hcal2DPhi/nHcal2Layers * iLayer;
    G4cout << "M_PI: " << M_PI << "     theta: " << theta << "    TileAngle: " << theta2 << G4endl;

    G4double xposShift = rAbsLayerCenter * (cos(theta) - 1.0);
    G4double yposShift = rAbsLayerCenter * sin(theta);

    //G4ThreeVector absTrans = G4ThreeVector(RmidAbsLayerX + xposShift - xPadding, yposShift, 0); 

    xPadding = 0.013*RmidAbsLayerX*LayerNum;      // adding an extra padding for placing the absorber and scintillator 

    G4ThreeVector myTrans = G4ThreeVector(RmidAbsLayerX + xposShift - xPadding, yposShift, 0); 
    G4RotationMatrix rotm  = G4RotationMatrix();
    rotm.rotateY(90*deg);
    rotm.rotateX(90*deg);
    rotm.rotateY(-90*deg);

    tiltPadding = LayerNum*0.005*rad;     // This should be checked too 11/19/2015
    rotm.rotateZ((iLayer + nHcal2Layers/2 + 1)*0.020*rad + tiltPadding);    // Added a funge increment factor 0.02
 
    G4Transform3D transform = G4Transform3D(rotm,myTrans);
     
    G4cout << "  iLayer " << iLayer << G4endl;

    new G4PVPlacement(transform,             //rotation,position
                      logicHcal2ScintLayer,  //its logical volume
                      "hcal2ScintLayer",     //its name
                      logicHcalBox,          //its mother volume
                      false,                 //no boolean operation
                      iLayer,                //copy number
                      checkOverlaps);        //checking overlaps 
    LayerNum++;
  }

  // Make INNER hcal (hcal1) section
  // 
  // Build scintillator layer box as a place holder for the scintillator sheets
  // 
  /*
  G4Box* hcal1ScintLayer = new G4Box("hcal1ScintLayer",		                                        //its name
				     hcal1ScintSizeX/2,
				     (hcal1ScintSizeY-0.05*cm)/2,
				     hcal1ScintSizeZ/2);  //its size; make it 0.05*cm thinner than the true gap width
  */

  // Try out a Trap volume for holding the scintilator tiles and the absorber
  G4Trap* hcal1ScintLayer = new G4Trap("hcal1ScintLayer",             //its name
				       hcal1Abs_dz, hcal1Abs_dxa, 
				       hcal1Abs_dyb + 2*0.35*cm + 0.1*mm,  // add 0.1*mm cushion
				       hcal1Abs_dya + 2*0.35*cm + 0.1*mm); // add 0,1*mm cushion

  logicHcal1ScintLayer = new G4LogicalVolume(hcal1ScintLayer,     // its solid name
					     world_mat,           // simply use world material
					     "hcal1ScintLayer");  // its name
  
  // Scintillator visualization attributes
  //G4VisAttributes* scintVisAtt= new G4VisAttributes(G4Colour(1.0,0.0,0.0));  //White
  //scintVisAtt->SetVisibility(true);
  //scintVisAtt->SetForceSolid(true);
  logicHcal1ScintLayer->SetVisAttributes(scintVisAtt);

  // Constructing scintillator sheets
  // Construct inner scintillator 1U
  // The numbers are read off from Richie's drawings
  G4double inner1UpDz = 198.1*mm;          // 1st prototype: 316.8*mm;
  //  G4double inner1UpTheta = 0.0;
  //  G4double inner1UpPhi = 0.0;
  G4double inner1UpDy1 = 2*0.35*cm;
  //  G4double inner1UpDy2 = 0.35*cm;          
  //  G4double inner1UpDx1 = 0.25*121.3*mm;    // 1st prototype: 0.25*108.6*mm;   // not used
  G4double inner1UpDx2 = 121.3*mm;         // 1st prototype: 108.6*mm;
  // G4double inner1UpDx3 = 0.25*105.9*mm;    // 1st prototype: 0.25*77.4*mm;    // not used
  G4double inner1UpDx4 = 121.3*mm;         // 1st prototype: 77.4*mm;
  //  G4double inner1UpAlp1 = 0., inner1UpAlp2 = 0.;

  G4Trap *inner1USheetSolid = new G4Trap("inner1USheet",                      //its name
					 inner1UpDy1, inner1UpDz, 
					 inner1UpDx2, inner1UpDx4);           //its size
    
  G4LogicalVolume *logicInner1USheet = new G4LogicalVolume(inner1USheetSolid, 
							 scint_mat,
							 "inner1USheet");

  logicInner1USheet->SetVisAttributes(scintSheetVisAtt);

  // Detector placement
  // Position the inner right 1U sheet

  G4ThreeVector threeVecInner1U_1 = G4ThreeVector(-0.5*(hcal1Abs_dya + 2*0.35*cm + 0.1*mm), 
						  0.0*mm, 
						  -0.250*(inner1UpDx2+inner1UpDx4));

  G4RotationMatrix rotInner1U_1  = G4RotationMatrix();
  rotInner1U_1.rotateZ(90*deg);
  rotInner1U_1.rotateX(-90*deg);
  rotInner1U_1.rotateZ(-90*deg);

  G4Transform3D transformInner1U_1 = G4Transform3D(rotInner1U_1,threeVecInner1U_1);

  new G4PVPlacement(transformInner1U_1,
		    logicInner1USheet,
		    "inner1USheet",
		    logicHcal1ScintLayer,
		    false,
		    0,                    // Copy one
		    checkOverlaps);

  
  // Detector placement
  // Position the inner left 1U sheet
  G4ThreeVector threeVecInner1U_2 = G4ThreeVector(-0.5*(hcal1Abs_dya + 2*0.35*cm + 0.1*mm), 
						  0.0*mm, 
						  0.250*(inner1UpDx2+inner1UpDx4));
  G4RotationMatrix rotInner1U_2  = G4RotationMatrix();
  rotInner1U_2.rotateZ(90*deg);
  rotInner1U_2.rotateX(90*deg);
  rotInner1U_2.rotateZ(-90*deg);
  G4Transform3D transformInner1U_2 = G4Transform3D(rotInner1U_2,threeVecInner1U_2);

  new G4PVPlacement(transformInner1U_2,
		    logicInner1USheet,
		    "inner1USheet",
		    logicHcal1ScintLayer,
		    false,
		    1,                      // Copy two
		    checkOverlaps);

  // Construct inner scintillator 2U
  // updated on 11/12/2015, HE
  G4double inner2UpDz = 0.5*198.1*mm;         // 1st prototype: 0.5*316.8*mm; 
  G4double inner2UpTheta = 2.5*M_PI/180.;    // 1st protytype: 8.8*M_PI/180.;
  G4double inner2UpPhi = 0.0*M_PI/180.;
  G4double inner2UpDy1 = 0.35*cm;
  G4double inner2UpDy2 = 0.35*cm;
  G4double inner2UpDx1 = 0.5*126.4*mm;        // 1st prototype: 0.5*108.6*mm;
  G4double inner2UpDx2 = 0.5*126.4*mm;        // 1st prototype: 0.5*108.6*mm;
  G4double inner2UpDx3 = 0.5*110.59*mm;       // 1st prototype: 0.5*77.4*mm;
  G4double inner2UpDx4 = 0.5*110.59*mm;       // 1st prototype: 0.5*77.4*mm;
  G4double inner2UpAlp1 = 0.*M_PI/180., inner2UpAlp2 = 0.*M_PI/180;

  G4Trap *inner2USheetSolid = new G4Trap("inner2USheet",
				  inner2UpDz,
				  inner2UpTheta,
				  inner2UpPhi, 
				  inner2UpDy1,
				  inner2UpDx1,
				  inner2UpDx2,
				  inner2UpAlp1,
				  inner2UpDy2,
				  inner2UpDx3,
				  inner2UpDx4,
				  inner2UpAlp2);

  G4LogicalVolume *logicInner2USheet = new G4LogicalVolume(inner2USheetSolid, 
							 scint_mat,
							 "inner2USheet");

  logicInner2USheet->SetVisAttributes(scintSheetVisAtt);
  
  // Detector placement
  // Position the left most sheet

  G4ThreeVector threeVecInner2U_1 = G4ThreeVector(-0.5*(hcal1Abs_dya + 2*0.35*cm + 0.1*mm),
						  0.0*mm,
						  0.755*(inner1UpDx2+inner1UpDx4));

  G4RotationMatrix rotInner2U_1  = G4RotationMatrix();
  rotInner2U_1.rotateZ(90*deg);
  rotInner2U_1.rotateX(-90*deg);

  G4Transform3D transformInner2U_1 = G4Transform3D(rotInner2U_1,threeVecInner2U_1);

  new G4PVPlacement(transformInner2U_1,
		    logicInner2USheet,
		    "inner2USheet",
		    logicHcal1ScintLayer,
		    false,
		    0,                     // copy one
      		    checkOverlaps);
  
  // Detector placement
  // Position the inner right most sheet
  // Position the left most sheet
  G4ThreeVector threeVecInner2U_2 = G4ThreeVector(-0.5*(hcal1Abs_dya + 2*0.35*cm + 0.1*mm),
						  0.0*mm,
						  -0.755*(inner1UpDx2+inner1UpDx4));

  G4RotationMatrix rotInner2U_2  = G4RotationMatrix();
  rotInner2U_2.rotateZ(-90*deg);
  rotInner2U_2.rotateX(-90*deg);

  G4Transform3D transformInner2U_2 = G4Transform3D(rotInner2U_2,threeVecInner2U_2);

  new G4PVPlacement(transformInner2U_2,
		    logicInner2USheet,
		    "inner2USheet",
		    logicHcal1ScintLayer,
		    false,
		    1,                     // copy two
		    checkOverlaps);  
  
  //
  // Build hcal1 (inner HCAL) absorber layer in trapezoid shape 
  //    
  G4Trap* hcal1AbsLayer = new G4Trap("hcal1AbsLayer",                  //its name
				     hcal1Abs_dz, hcal1Abs_dxa, 
				     hcal1Abs_dyb, hcal1Abs_dya);      //its size

  logicHcal1AbsLayer = new G4LogicalVolume(hcal1AbsLayer,
					   steel,
					   "hcal1AbsLayer"); 
  
  logicHcal1AbsLayer->SetVisAttributes(hcal2AbsVisAtt);

  // Detector placement
  // Position the hcal1Absorber (inner) in the holder box
  G4ThreeVector threeVecHcal1Abs = G4ThreeVector(2*0.35*cm, 0*cm, 0*cm);
  G4RotationMatrix rotHcal1Abs = G4RotationMatrix();

  G4Transform3D transformHcal1Abs = G4Transform3D(rotHcal1Abs,threeVecHcal1Abs);

  new G4PVPlacement(transformHcal1Abs,
		    logicHcal1AbsLayer,
		    "hcal1AbsLayer",
		    logicHcal1ScintLayer,        // mother volume is the holder box
		    false,
		    0,        
		    checkOverlaps); 

  // Calculate the title angle
  theta = 0.;
  theta2 = hcal1TiltAngle;
  rAbsLayerCenter = hcal1RadiusIn + hcal1Abs_dxa/2.0;
  RmidAbsLayerX = -hcalBoxSizeX/2.0 + hcal1Abs_dxa/2.0 + 3*cm;  // added 3cm space in front of the inner hcal
  LayerNum = 1;          // These three parameters are for making fine adjustment of the placement of inner hcal
  xPadding = 0.0;
  yPadding = 0.0;
  tiltPadding = 0.0;
  for (G4int iLayer = -nHcal2Layers/2 -1; iLayer < nHcal2Layers/2 - 1; iLayer++) {  // shift one layer down
    //
    // Place inner hcal scintillator + absorber layer
    //
    theta = hcal1DPhi/nHcal1Layers * iLayer;
    //G4cout << "M_PI: " << M_PI << "     theta: " << theta << "    TileAngle: " << theta2 << G4endl;

    G4double xposShift = rAbsLayerCenter * (cos(theta) - 1.0);
    G4double yposShift = rAbsLayerCenter * sin(theta);

    G4ThreeVector myTrans = G4ThreeVector(RmidAbsLayerX + xposShift + xPadding, yposShift + yPadding, 0); 

    G4RotationMatrix rotm  = G4RotationMatrix();
    rotm.rotateY(90*deg);
    rotm.rotateX(90*deg);
    rotm.rotateY(-90*deg);

    rotm.rotateZ((theta-theta2)*rad + tiltPadding);   

    G4Transform3D transform = G4Transform3D(rotm,myTrans);
     
    G4cout << "  Inner HCAL iLayer " << iLayer << G4endl;

    new G4PVPlacement(transform,               //rotation,position
                      logicHcal1ScintLayer,    //its logical volume
                      "hcal1ScintLayer",       //its name
                      logicHcalBox,            //its mother volume: logicHcal1Ab
                      false,                   //no boolean operation
                      iLayer,                  //copy number
                      checkOverlaps);          //checking overlaps 
    LayerNum++;
  }

  //
  // Make a cryostat box 
  //
  G4Box* cryostatBox = new G4Box("cryoBox",		     //its name
				 cryostatSizeX/2, cryostatSizeY/2, cryostatSizeZ/2);  //its size  

  G4LogicalVolume* logicCryostatBox = new G4LogicalVolume(cryostatBox,      //its solid
							  alum,	   //its material
							  "cryoBox");       //its name
  
  G4ThreeVector threeVecCryostatBox = G4ThreeVector((hcal1ScintSizeX - 0.5*hcalBoxSizeX) + 10.0*cm + cryostatSizeX/2, -5*hcal1ScintSizeY, 0.0*cm);
  G4RotationMatrix rotCryostatBox  = G4RotationMatrix();
  //rotJunction.rotateZ(0*deg);
  //rotJunction.rotateX(0*deg);
  G4Transform3D transformCryostatBox = G4Transform3D(rotCryostatBox,threeVecCryostatBox);  

  // Place the cryostat box inside the HcalBox in between the inner and outer hcal section.

  new G4PVPlacement(transformCryostatBox,        // rotation + positioning
		    logicCryostatBox,	        //its logical volume
		    "cryoBox", 		        //its name
		    logicHcalBox,		//its mother  volume
		    false,		        //no boolean operation
		    0,			        //copy number
		    checkOverlaps);              //checking overlaps
  
  // HCal Box visualization attributes
  G4VisAttributes* cryoBoxVisAtt= new G4VisAttributes(G4Colour(0.0,0.0,1.0)); // blue
  cryoBoxVisAtt->SetVisibility(true);
  //hcalBoxVisAtt->SetForceSolid(true);
  logicCryostatBox->SetVisAttributes(cryoBoxVisAtt);
							     
  return physiWorld;
}


void PHG4HcalPrototype2Detector::CalculateGeometry()
{
  // tilt angle outer steel
  // outer_tilt_angle = atan((outer_steel_y_out-outer_steel_y_in)/outer_plate_x);
  // cout << "outer tilt angle: " << outer_tilt_angle*180./M_PI << endl;
    return;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

void PHG4HcalPrototype2Detector::SetMaterial(G4String materialChoice)
{
  /*
  // search the material by its name   
  G4Material* pttoMaterial =
    G4NistManager::Instance()->FindOrBuildMaterial(materialChoice);
  if (pttoMaterial) {
    world_mat = pttoMaterial;
    logicWorld->SetMaterial(world_mat);
    G4RunManager::GetRunManager()->PhysicsHasBeenModified();
  }
  */
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
 
void PHG4HcalPrototype2Detector::UpdateGeometry()
{
  G4RunManager::GetRunManager()->PhysicsHasBeenModified();
  G4RunManager::GetRunManager()->DefineWorldVolume(ConstructDetector());
}

// The following code is copied from the sPHENIX G4 simulation
void PHG4HcalPrototype2Detector::SetTiltViaNcross(const int ncross)
{
  G4double sign = 1;
  if (ncross < 0)
    {
      sign = -1;
    }
  G4int ncr = fabs(ncross);

  // Determine title angle for the outer hcal
  //
  G4double cSide = hcal2RadiusIn + hcal2ScintSizeX/2;
  G4double bSide = hcal2RadiusIn + hcal2ScintSizeX;

  G4double alpha = 0;
  if (ncr > 1)
    {
      alpha = (360. / nHcal2Layers * M_PI / 180.) * (ncr - 1) / 2.0;
    }
  else
    {
      alpha = (360. / nHcal2Layers * M_PI / 180.) / 2.;
    }

  G4double sinbSide = sin(alpha) * bSide / (sqrt(bSide * bSide + cSide * cSide - 2 * bSide * cSide * cos(alpha)));
  G4double beta = asin(sinbSide);  // This is the slat angle

  hcal2TiltAngle = beta * sign;

  // Determine title angle for the inner hcal
  //
  cSide = hcal1RadiusIn + hcal1ScintSizeX/2;
  bSide = hcal1RadiusIn + hcal1ScintSizeX;

  /* This part is the same both for the inner and the outer hcal section
  G4doube alpha = 0;
  if (ncr > 1)
    {
      alpha = (360. / nScint360 * M_PI / 180.) * (ncr - 1);
    }
  else
    {
      alpha = (360. / nScint360 * M_PI / 180.) / 2.;
    }
  */

  sinbSide = sin(alpha) * bSide / (sqrt(bSide * bSide + cSide * cSide - 2.0 * bSide * cSide * cos(alpha)));
  beta = asin(sinbSide);  // This is the slat angle

  hcal1TiltAngle = beta * sign;

  G4cout << " alpha : " << alpha << G4endl;
  G4cout <<  " SetTitlViaNCross(" << ncross << ") setting the outer hcal slat tilt angle to : " << hcal2TiltAngle << " radian" << G4endl;
  G4cout <<  " SetTitlViaNCross(" << ncross << ") setting the inner hcal slat tilt angle to : " << hcal1TiltAngle << " radian" << G4endl;
  return;

}

// Detector construction messengers for setting plate angles
// 
void PHG4HcalPrototype2Detector::SetOuterHcalDPhi(G4double dphi)
{
  hcal2DPhi = dphi;
  G4cout << "In SetOuterHcalDPhi: " << hcal2DPhi << " is set!!! " << G4endl;
}

void PHG4HcalPrototype2Detector::SetOuterPlateTiltAngle(G4double dtheta)
{
  hcal2TiltAngle = dtheta;
  G4cout << "In SetOuterPlateTiltAngle: " << hcal2TiltAngle << " is set!!! " << G4endl;
}

void PHG4HcalPrototype2Detector::SetInnerHcalDPhi(G4double dphi)
{
  hcal1DPhi = dphi;
  G4cout << "In SetInnerHcalDPhi: " << hcal1DPhi << " is set!!! " << G4endl;
}

void PHG4HcalPrototype2Detector::SetInnerPlateTiltAngle(G4double dtheta)
{
  hcal1TiltAngle = dtheta;
  G4cout << "In SetInnerPlateTiltAngle: " << hcal1TiltAngle << " is set!!! " << G4endl;
}


