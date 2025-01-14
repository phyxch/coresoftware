#include "PHG4HcalSubsystem.h"
#include "PHG4HcalDetector.h"
#include "PHG4HcalSteppingAction.h"
#include "PHG4EventActionClearZeroEdep.h"
#include <g4main/PHG4Utils.h>

#include <g4main/PHG4PhenixDetector.h>
#include <g4main/PHG4HitContainer.h>

#include <phool/getClass.h>

#include <Geant4/globals.hh>

#include <sstream>
#include "TMath.h"
#include "TVector2.h"

using namespace std;

//_______________________________________________________________________
PHG4HcalSubsystem::PHG4HcalSubsystem( const std::string &na, const int lyr):
  detector_( NULL ),
  steppingAction_( NULL ),
  eventAction_(NULL),
  radius(100),
  length(100),
  xpos(0),
  ypos(0),
  zpos(0),
  lengthViaRapidityCoverage(true),
  TrackerThickness(100),
  material("G4_GALACTIC"),
  _sciTilt(0),
  _sciWidth(0.7),
  _sciNum(256),
  _sciPhi0(0),
  active(0),
  absorberactive(0),
  layer(lyr),
  detector_type(na),
  superdetector("NONE"),
  light_scint_model_(true),
  light_balance_(false),
  light_balance_inner_radius_(0.0),
  light_balance_inner_corr_(1.0),
  light_balance_outer_radius_(10.0),
  light_balance_outer_corr_(1.0)  
{
  // put the layer into the name so we get unique names
  // for multiple SVX layers
  ostringstream nam;
  nam << na << "_" << lyr;
  Name(nam.str().c_str());
}

//_______________________________________________________________________
int PHG4HcalSubsystem::InitRun( PHCompositeNode* topNode )
{
  // create hit list only for active layers
  PHNodeIterator iter( topNode );
  PHCompositeNode *dstNode = dynamic_cast<PHCompositeNode*>(iter.findFirst("PHCompositeNode", "DST" ));
  // create detector
  detector_ = new PHG4HcalDetector(topNode, Name(), layer);
  detector_->SetRadius(radius);
  G4double detlength = length;
  if (lengthViaRapidityCoverage)
    {
      detlength =  PHG4Utils::GetLengthForRapidityCoverage(radius + TrackerThickness) * 2;
    }
  detector_->SetLength(detlength);
  detector_->SetPosition(xpos, ypos, zpos);
  detector_->SetThickness(TrackerThickness);
  detector_->SetTilt(_sciTilt);
  detector_->SetScintWidth(_sciWidth);
  detector_->SetNumScint(_sciNum);
  detector_->SetScintPhi0(_sciPhi0);
  detector_->SetMaterial(material);
  detector_->SetActive(active);
  detector_->SetAbsorberActive(absorberactive);
  detector_->SuperDetector(superdetector);
  detector_->OverlapCheck(overlapcheck);
  if (active)
    {
      ostringstream nodename;
      if (superdetector != "NONE")
        {
          nodename <<  "G4HIT_" << superdetector;
        }
      else
        {
          nodename <<  "G4HIT_" << detector_type << "_" << layer;
        }
      PHG4HitContainer* cylinder_hits =  findNode::getClass<PHG4HitContainer>( topNode , nodename.str().c_str() );
      if ( !cylinder_hits )
        {
          dstNode->addNode( new PHIODataNode<PHObject>( cylinder_hits = new PHG4HitContainer(), nodename.str().c_str(), "PHObject" ));
        }
      cylinder_hits->AddLayer(layer);
      PHG4EventActionClearZeroEdep *evtac = new PHG4EventActionClearZeroEdep(topNode, nodename.str());
      if (absorberactive)
        {
          nodename.str("");
          if (superdetector != "NONE")
            {
              nodename <<  "G4HIT_ABSORBER_" << superdetector;
            }
          else
            {
              nodename <<  "G4HIT_ABSORBER_" << detector_type << "_" << layer;
            }
          PHG4HitContainer* cylinder_hits =  findNode::getClass<PHG4HitContainer>( topNode , nodename.str().c_str() );
          if ( !cylinder_hits )
            {
              dstNode->addNode( new PHIODataNode<PHObject>( cylinder_hits = new PHG4HitContainer(), nodename.str().c_str(), "PHObject" ));
            }
          cylinder_hits->AddLayer(layer);
          evtac->AddNode(nodename.str());
        }
      eventAction_ = evtac;
      steppingAction_ = new PHG4HcalSteppingAction(detector_);
      steppingAction_->set_zmin(zpos - detlength / 2.);
      steppingAction_->set_zmax(zpos + detlength / 2.);
      if (light_balance_) {
	steppingAction_->SetLightCorrection(light_balance_inner_radius_,
					    light_balance_inner_corr_,
					    light_balance_outer_radius_,
					    light_balance_outer_corr_);
	steppingAction_->SetLightScintModel(light_scint_model_);
      }
    }


  return 0;

}

//_______________________________________________________________________
int PHG4HcalSubsystem::process_event( PHCompositeNode* topNode )
{
  // pass top node to stepping action so that it gets
  // relevant nodes needed internally
  if (steppingAction_)
    {
      steppingAction_->SetInterfacePointers( topNode );
    }
  return 0;

}

//_______________________________________________________________________
PHG4Detector* PHG4HcalSubsystem::GetDetector( void ) const
{
  return detector_;
}

//_______________________________________________________________________
PHG4SteppingAction* PHG4HcalSubsystem::GetSteppingAction( void ) const
{
  return steppingAction_;
}


void
PHG4HcalSubsystem::Print(const std::string &what) const
{
  detector_->Print(what);
  return;
}


void
PHG4HcalSubsystem::SetTiltViaNcross(const int ncross)
{
  if (ncross == 0)
    {
      cout << Name() << " invalid crossing number " << ncross 
	   << " how do you think we can construct a meaningful detector with this number????" << endl;
      exit(1);
    }
  // The delta phi angle between 2 adjacent slats is just 360/nslats. If
  // there is just one crossing the tips of each slat can extend from
  // phi-delta-phi/2 to phi+delta-phi/2. G4 rotates around the center of a 
  // slat so we are dealing in increments of just delta-phi/2. This 
  // has to be multiplied by the number of crossings we want.
  //
  // To find this tilt angle we have to calculate a triangle
  // the long sides are the outer radius and the
  // inner radius + 1/2 tracker thickness
  // the angle between these sides is just (360/nslat)/2*(cross)
  // the we use a/sin(alpha) = b/sin(beta)
  // and c^2 = a^2+b^2 -2ab*cos(gamma)
  // the solution is not unique, we have to pick 180-beta
  // but the slat angle is 180-beta (it's on the other side of the triangle
  // just draw the damned thing if this is confusing)
  double sign = 1;
  if (ncross < 0)
    {
      sign = -1;
    }
  int ncr = fabs(ncross);
  double thick = TrackerThickness;
  double c = radius + thick / 2.;
  double b = radius + thick;

  double alpha = 0;
  alpha = ((360. / _sciNum * M_PI / 180.)/2.) * ncr;
  double sinb = sin(alpha) * b / (sqrt(b * b + c * c - 2 * b * c * cos(alpha)));
  double beta = asin(sinb) * 180. / M_PI; // this is already the slat angle
  _sciTilt = beta * sign;
  // print out triangle
//   double tbeta = 180 - beta;
//   double gamma = 180 - (alpha * 180. / M_PI) - tbeta;
//   double a = b * sin(alpha) / sinb;
  //   cout << "triangle length: a: " << a
  //        << ", b: " << b << ", c: " << c << endl;
  //   cout << "triangle angle: alpha: " << alpha*180./M_PI
//        << ", beta: " << tbeta
//        << ", gamma: " << gamma
//        << endl;
  cout << Name() << ": SetTiltViaNcross(" << ncross << ") setting slat angle to: " << _sciTilt << " degrees" <<  endl;
  return;
}
