#include "PHG4BlockCellReco.h"
#include "PHG4BlockGeomContainer.h"
#include "PHG4BlockGeom.h"
#include "PHG4BlockCellGeomContainer.h"
#include "PHG4BlockCellGeom.h"
#include "PHG4CylinderCellv1.h"
#include "PHG4CylinderCellContainer.h"
#include "PHG4CylinderCellDefs.h"

#include <g4main/PHG4Hit.h>
#include <g4main/PHG4HitContainer.h>
#include <fun4all/Fun4AllReturnCodes.h>
#include <fun4all/Fun4AllServer.h>
#include <phool/PHNodeIterator.h>
#include <phool/PHCompositeNode.h>
#include <phool/PHIODataNode.h>
#include <phool/getClass.h>

#include<TROOT.h>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>

using namespace std;

#define ARRAYDIM1 100000
#define ARRAYDIM2 2000

static vector<PHG4CylinderCell*> cellptarray;

PHG4BlockCellReco::PHG4BlockCellReco(const string &name) :
  SubsysReco(name),
  _timer(PHTimeServer::get()->insert_new("PHG4BlockCellReco")),
  chkenergyconservation(0)
{
  memset(nbins, 0, sizeof(nbins));
}

int PHG4BlockCellReco::InitRun(PHCompositeNode *topNode)
{
  PHNodeIterator iter(topNode);

  // Looking for the DST node
  PHCompositeNode *dstNode;
  dstNode = dynamic_cast<PHCompositeNode*>(iter.findFirst("PHCompositeNode", "DST"));
  if (!dstNode)
  {
    std::cout << PHWHERE << "DST Node missing, doing nothing." << std::endl;
    exit(1);
  }

  hitnodename = "G4HIT_" + detector;
  PHG4HitContainer *g4hit = findNode::getClass<PHG4HitContainer>(topNode, hitnodename.c_str());
  if (!g4hit)
  {
    cout << "Could not locate g4 hit node " << hitnodename << endl;
    exit(1);
  }

  cellnodename = "G4CELL_" + detector;
  PHG4CylinderCellContainer *cells = findNode::getClass<PHG4CylinderCellContainer>(topNode , cellnodename);
  if (!cells)
    {
      PHNodeIterator dstiter(dstNode);
      PHCompositeNode *DetNode =
          dynamic_cast<PHCompositeNode*>(dstiter.findFirst("PHCompositeNode",
              detector));
      if (!DetNode)
        {
          DetNode = new PHCompositeNode(detector);
          dstNode->addNode(DetNode);
        }

    cells = new PHG4CylinderCellContainer();
    PHIODataNode<PHObject> *newNode = new PHIODataNode<PHObject>(cells, cellnodename.c_str() , "PHObject");
    DetNode->addNode(newNode);
  }

  geonodename = "BLOCKGEOM_" + detector;
  PHG4BlockGeomContainer *geo =  findNode::getClass<PHG4BlockGeomContainer>(topNode , geonodename.c_str());
  if (!geo)
  {
    cout << "Could not locate geometry node " << geonodename << endl;
    exit(1);

  }

  if (verbosity > 0)
  {
    geo->identify();
  }

  seggeonodename = "BLOCKCELLGEOM_" + detector;
  PHG4BlockCellGeomContainer *seggeo = findNode::getClass<PHG4BlockCellGeomContainer>(topNode , seggeonodename.c_str());
  if (!seggeo)
  {
    seggeo = new PHG4BlockCellGeomContainer();
    PHCompositeNode *runNode = dynamic_cast<PHCompositeNode*>(iter.findFirst("PHCompositeNode", "RUN" ));
    PHIODataNode<PHObject> *newNode = new PHIODataNode<PHObject>(seggeo, seggeonodename.c_str() , "PHObject");
    runNode->addNode(newNode);
  }

  map<int, PHG4BlockGeom *>::const_iterator miter;
  pair <map<int, PHG4BlockGeom *>::const_iterator, map<int, PHG4BlockGeom *>::const_iterator> begin_end = geo->get_begin_end();
  map<int, std::pair <double, double> >::iterator sizeiter;
  for (miter = begin_end.first; miter != begin_end.second; ++miter)
  {
    PHG4BlockGeom *layergeom = miter->second;
    int layer = layergeom->get_layer();
    double radius = sqrt(pow(layergeom->get_center_x(),2) + pow(layergeom->get_center_y(),2));
    double width = layergeom->get_size_x();
    double length_in_z = layergeom->get_size_z();
    double zmin = layergeom->get_center_z() - length_in_z/2.;
    double zmax = zmin + length_in_z;

    sizeiter = cell_size.find(layer);
    if (sizeiter == cell_size.end())
    {
      cout << "no cell sizes for layer " << layer << endl;
      exit(1);
    }
    
    // create geo object and fill with variables common to all binning methods
    PHG4BlockCellGeom *layerseggeo = new PHG4BlockCellGeom();
    layerseggeo->set_layer(layergeom->get_layer());
    // layerseggeo->set_radius(layergeom->get_radius());
    // layerseggeo->set_thickness(layergeom->get_thickness());

    if (binning[layer] == PHG4CylinderCellDefs::etaphibinning)
    {
      // calculate eta at radius+ thickness (outer radius)
      // length via eta coverage is calculated using the outer radius
      double etamin = get_eta(radius + 0.5*layergeom->get_size_y(), zmin);
      double etamax = get_eta(radius + 0.5*layergeom->get_size_y(), zmax);
      zmin_max[layer] = make_pair(etamin, etamax);
      double etastepsize = (sizeiter->second).first;
      double d_etabins;
      double fract = modf((etamax - etamin) / etastepsize, &d_etabins);
      if (fract != 0)
      {
        d_etabins++;
      }
      etastepsize = (etamax - etamin) / d_etabins;
      (sizeiter->second).first = etastepsize;
      int etabins = d_etabins;
      double etahi = etamin + etastepsize;
      for (int i = 0; i < etabins; i++)
      {
        if (etahi > (etamax + 1.e-6)) // etahi is a tiny bit larger due to numerical uncertainties
        {
          cout << "etahi: " << etahi << ", etamax: " << etamax << endl;
        }
        etahi +=  etastepsize;
      }

      double xmin = -layergeom->get_width()/2.;
      //double xmax = -xmin;
      double xstepsize = (sizeiter->second).second;
      double d_xbins;
      fract = modf(width / xstepsize, &d_xbins);
      if (fract != 0)
      {
        d_xbins++;
      }

      xstepsize = width / d_xbins;
      (sizeiter->second).second = xstepsize;
      int xbins = d_xbins;
      // double xlow = xmin;
      // double xhi = xlow + xstepsize;

      // for (int i = 0; i < xbins; i++)
      // {
      //   if (xhi > xmax)
      //   {
      //     cout << "xhi: " << xhi << ", xmax: " << xmax << endl;
      //   }
      //   xlow = xhi;
      //   xhi +=  xstepsize;
      // }

      pair<int, int> x_z_bin = make_pair(xbins, etabins);
      n_x_z_bins[layer] = x_z_bin;
      layerseggeo->set_binning(PHG4CylinderCellDefs::etaphibinning);
      layerseggeo->set_etabins(etabins);
      layerseggeo->set_etamin(etamin);
      layerseggeo->set_etastep(etastepsize);
      layerseggeo->set_xmin(xmin);
      layerseggeo->set_xbins(xbins);
      layerseggeo->set_xstep(xstepsize);
      xstep[layer] = xstepsize;
      etastep[layer] = etastepsize;
    }
    
    // add geo object filled by different binning methods
    seggeo->AddLayerCellGeom(layerseggeo);
    if (verbosity > 1)
    {
      layerseggeo->identify();
    }
  }
  return Fun4AllReturnCodes::EVENT_OK;
}


int
PHG4BlockCellReco::process_event(PHCompositeNode *topNode)
{
  _timer.get()->restart();

  PHG4HitContainer *g4hit = findNode::getClass<PHG4HitContainer>(topNode, hitnodename.c_str());
  if (!g4hit)
  {
    cout << "Could not locate g4 hit node " << hitnodename << endl;
    exit(1);
  }
  PHG4CylinderCellContainer *cells = findNode::getClass<PHG4CylinderCellContainer>(topNode, cellnodename);
  if (! cells)
  {
    cout << "could not locate cell node " << cellnodename << endl;
    exit(1);
  }

  PHG4BlockCellGeomContainer *seggeo = findNode::getClass<PHG4BlockCellGeomContainer>(topNode , seggeonodename.c_str());
  if (! seggeo)
  {
    cout << "could not locate geo node " << seggeonodename << endl;
    exit(1);
  }

  PHG4HitContainer::LayerIter layer;
  pair<PHG4HitContainer::LayerIter, PHG4HitContainer::LayerIter> layer_begin_end = g4hit->getLayers();
  //   cout << "number of layers: " << g4hit->num_layers() << endl;
  //   cout << "number of hits: " << g4hit->size() << endl;
  //   for (layer = layer_begin_end.first; layer != layer_begin_end.second; layer++)
  //     {
  //       cout << "layer number: " << *layer << endl;
  //     }

  for (layer = layer_begin_end.first; layer != layer_begin_end.second; layer++)
  {
    PHG4HitContainer::ConstIterator hiter;
    PHG4HitContainer::ConstRange hit_begin_end = g4hit->getHits(*layer);
    PHG4BlockCellGeom *geo = seggeo->GetLayerCellGeom(*layer);
    int nxbins = n_x_z_bins[*layer].first;
    int nzbins = n_x_z_bins[*layer].second;
    unsigned int nbins = nxbins*nzbins;

    if(cellptarray.size() < nbins)
      cellptarray.resize(nbins, 0);

    if ( nxbins > ARRAYDIM1)
    {
      cout << "too many bins in x: " << nxbins
           << " change and recompile" << endl;
      exit(1);
    }

    if (nzbins > ARRAYDIM2)
    {
      cout << "too many bins in z: " << nzbins
           << " change and recompile" << endl;
      exit(1);
    }


    // ------- eta/x binning ------------------------------------------------------------------------
    if (binning[*layer] == PHG4CylinderCellDefs::etaphibinning)
    {
      for (hiter = hit_begin_end.first; hiter != hit_begin_end.second; hiter++)
      {
        pair<double, double> etax[2];
        double xbin[2];
        double etabin[2];
        for (int i = 0; i < 2; i++)
        {
          etax[i] = get_etaphi(hiter->second->get_x(i), hiter->second->get_y(i), hiter->second->get_z(i));
          etabin[i] = geo->get_etabin( etax[i].first );
          xbin[i] = geo->get_xbin( etax[i].second );
        }

        // check bin range
        if (xbin[0] < 0 || xbin[0] >= nxbins || xbin[1] < 0 || xbin[1] >= nxbins)
        {
          continue;
        }
        if (etabin[0] < 0 || etabin[0] >= nzbins   || etabin[1] < 0 || etabin[1] >= nzbins)
        {
          continue;
        }

        if (etabin[0] < 0)
        {
          if (verbosity > 0)
          {
            hiter->second->identify();
          }
          continue;
        }

        int intxbin = xbin[0];
        int intetabin = etabin[0];
        int intxbinout = xbin[1];
        int intetabinout = etabin[1];

        // Determine all fired cells

        double ax = (etax[0]).second; // x
        double ay = (etax[0]).first;  // eta
        double bx = (etax[1]).second;
        double by = (etax[1]).first;
        if (intxbin > intxbinout)
        {
          int tmp = intxbin;
          intxbin = intxbinout;
          intxbinout = tmp;
        }

        if (intetabin > intetabinout)
        {
          int tmp = intetabin;
          intetabin = intetabinout;
          intetabinout = tmp;
        }

        double trklen = sqrt((ax - bx) * (ax - bx) + (ay - by) * (ay - by));
	      // if entry and exit hit are the same (seems to happen rarely), trklen = 0
	      // which leads to a 0/0 and an NaN in edep later on
	      // this code does for particles in the same cell a trklen/trklen (vdedx[ii]/trklen)
	      // so setting this to any non zero number will do just fine
	      // I just pick -1 here to flag those strange hits in case I want t oanalyze them
	      // later on
	      if (trklen == 0)
        {
          trklen = -1.;
        }
        vector<int> vx;
        vector<int> veta;
        vector<double> vdedx;

        if (intxbin == intxbinout && intetabin == intetabinout)   // single cell fired
        {
          if (verbosity > 0) cout << "SINGLE CELL FIRED: " << intxbin << " " << intetabin << endl;
          vx.push_back(intxbin);
          veta.push_back(intetabin);
          vdedx.push_back(trklen);
        }
        else
        {
          for (int ibp = intxbin; ibp <= intxbinout; ibp++)
          {
            for (int ibz = intetabin; ibz <= intetabinout; ibz++)
            {
              double cx = geo->get_xcenter(ibp) - geo->get_xstep() / 2.;
              double dx = geo->get_xcenter(ibp) + geo->get_xstep() / 2.;
              double cy = geo->get_etacenter(ibz) - geo->get_etastep() / 2.;
              double dy = geo->get_etacenter(ibz) + geo->get_etastep() / 2.;
              double rr = 0.;
              //cout << "##### line: " << ax << " " << ay << " " << bx << " " << by << endl;
              //cout << "####### cell: " << cx << " " << cy << " " << dx << " " << dy << endl;
              bool yesno = line_and_rectangle_intersect(ax, ay, bx, by, cx, cy, dx, dy, &rr);
              if (yesno)
              {
                if (verbosity > 0) cout << "CELL FIRED: " << ibp << " " << ibz << " " << rr << endl;
                vx.push_back(ibp);
                veta.push_back(ibz);
                vdedx.push_back(rr);
              }
            }
          }
        }
        if (verbosity > 0) cout << "NUMBER OF FIRED CELLS = " << vx.size() << endl;

        double tmpsum = 0.;
        for (unsigned int ii = 0; ii < vx.size(); ii++)
        {
          tmpsum += vdedx[ii];
          vdedx[ii] = vdedx[ii] / trklen;
          if (verbosity > 0) cout << "  CELL " << ii << "  dE/dX = " <<  vdedx[ii] << endl;
        }
        if (verbosity > 0) cout << "    TOTAL TRACK LENGTH = " << tmpsum << " " << trklen << endl;


        for (unsigned int i1 = 0; i1 < vx.size(); i1++)   // loop over all fired cells
        {

          int ixbin = vx[i1];
          int ietabin = veta[i1];
          int ibin = ixbin*nzbins+ietabin;

          if (!cellptarray[ibin])
          {
            cellptarray[ibin] = new PHG4CylinderCellv1();
            cellptarray[ibin]->set_layer(*layer);
            cellptarray[ibin]->set_phibin(ixbin);
            cellptarray[ibin]->set_etabin(ietabin);
          }
          cellptarray[ibin]->add_edep(hiter->first, hiter->second->get_edep()*vdedx[i1], hiter->second->get_light_yield()*vdedx[i1]);
          // just a sanity check - we don't want to mess up by having Nan's or Infs in our energy deposition
          if (! isfinite(hiter->second->get_edep()*vdedx[i1]))
          {
            cout << PHWHERE << " invalid energy dep " << hiter->second->get_edep()
                 << " or path length: " << vdedx[i1] << endl;
          }
        }

        vx.clear();
        veta.clear();
      } // end loop over g4hits

      int numcells = 0;
      for (int ix = 0; ix < nxbins; ix++)
      {
        for (int iz = 0; iz < nzbins; iz++)
        {
          int ibin = ix*nzbins + iz;

          if (cellptarray[ibin])
          {
            cells->AddCylinderCell(*layer, cellptarray[ibin]);
            numcells++;
            if (verbosity > 1)
            {
              cout << "Adding cell in bin x: " << ix
                   << " x: " << geo->get_xcenter(ix) * 180./M_PI
                   << ", eta bin: " << iz
                   << ", eta: " <<  geo->get_etacenter(iz)
                   << ", energy dep: " << cellptarray[ibin]->get_edep()
                   << endl;
            }

            cellptarray[ibin] = 0;
          }
        }
      }

      if (verbosity > 0)
      {
        cout << Name() << ": found " << numcells << " eta/x cells with energy deposition" << endl;
      }
    }

  }

  if (chkenergyconservation)
  {
    CheckEnergy(topNode);
  }

  _timer.get()->stop();
  return Fun4AllReturnCodes::EVENT_OK;
}

int
PHG4BlockCellReco::End(PHCompositeNode *topNode)
{
  return Fun4AllReturnCodes::EVENT_OK;
}

void
PHG4BlockCellReco::cellsize(const int i, const double sr, const double sz)
{
  set_size(i, sr, sz, PHG4CylinderCellDefs::sizebinning);
}

void
PHG4BlockCellReco::etaxsize(const int i, const double deltaeta, const double deltax)
{
  set_size(i, deltaeta, deltax, PHG4CylinderCellDefs::etaphibinning);
  return;
}

void
PHG4BlockCellReco::set_size(const int i, const double sizeA, const double sizeB, const int what)
{
  if (binning.find(i) != binning.end())
  {
    cout << "size for layer " << i << " already set" << endl;
    return;
  }

  binning[i] = what;
  cell_size[i] = std::make_pair(sizeA, sizeB);

  return;
}

pair<double, double>
PHG4BlockCellReco::get_etaphi(const double x, const double y, const double z)
{
  double radius = sqrt(x * x + y * y);
  double phi = atan2(y, x);
  double theta = atan2(radius, z);
  double eta = -log(tan(theta / 2.));
  return make_pair(eta, phi);
}

double
PHG4BlockCellReco::get_eta(const double radius, const double z)
{
  double eta;
  double theta;
  theta = atan2(radius, fabs(z));
  eta = -log(tan(theta / 2.));
  if (z < 0)
  {
    eta = -eta;
  }
  return eta;
}

//---------------------------------------------------------------

bool PHG4BlockCellReco::lines_intersect( double ax,
                                         double ay,
                                         double bx,
                                         double by,
                                         double cx,
                                         double cy,
                                         double dx,
                                         double dy,
                                         double* rx, // intersection point (output)
                                         double* ry
                                         )
{

  // Find if a line segment limited by points A and B
  // intersects line segment limited by points C and D.
  // First check if an infinite line defined by A and B intersects
  // segment (C,D). If h is from 0 to 1 line and line segment intersect
  // Then check in intersection point is between C and D

  double ex = bx - ax; // E=B-A
  double ey = by - ay;
  double fx = dx - cx; // F=D-C
  double fy = dy - cy;
  double px = -ey;     // P
  double py = ex;

  double bottom = fx * px + fy * py; // F*P
  double gx = ax - cx; // A-C
  double gy = ay - cy;
  double top = gx * px + gy * py; // G*P

  double h = 99999.;
  if (bottom != 0.)
  {
    h = top / bottom;
  }

  //intersection point R = C + F*h
  if (h > 0. && h < 1.)
  {
    *rx = cx + fx * h;
    *ry = cy + fy * h;
    //cout << "      line/segment intersection coordinates: " << *rx << " " << *ry << endl;
    if ((*rx > ax && *rx > bx) || (*rx < ax && *rx < bx) || (*ry < ay && *ry < by) || (*ry > ay && *ry > by))
    {
      //cout << "       NO segment/segment intersection!" << endl;
      return false;
    }
    else
    {
      //cout << "       segment/segment intersection!" << endl;
      return true;
    }
  }

  return false;
}

//---------------------------------------------------------------

bool  PHG4BlockCellReco::line_and_rectangle_intersect( double ax,
                                                       double ay,
                                                       double bx,
                                                       double by,
                                                       double cx,
                                                       double cy,
                                                       double dx,
                                                       double dy,
                                                       double* rr // length of the line segment inside the rectangle (output)
                                                       )
{

  // find if a line isegment limited by points (A,B)
  // intersects with a rectangle defined by two
  // corner points (C,D) two other points are E and F
  //   E--------D
  //   |        |
  //   |        |
  //   C--------F

  if (cx > dx || cy > dy)
  {
    cerr << "ERROR: Bad rectangle definition!" << endl;
    return false;
  }

  double ex = cx;
  double ey = dy;
  double fx = dx;
  double fy = cy;
  double rx = 99999.;
  double ry = 99999.;

  vector<double> vx;
  vector<double> vy;

  bool i1 = lines_intersect(ax, ay, bx, by, cx, cy, fx, fy, &rx, &ry);
  if (i1)
  {
    vx.push_back(rx);
    vy.push_back(ry);
  }
  bool i2 = lines_intersect(ax, ay, bx, by, fx, fy, dx, dy, &rx, &ry);
  if (i2)
  {
    vx.push_back(rx);
    vy.push_back(ry);
  }
  bool i3 = lines_intersect(ax, ay, bx, by, ex, ey, dx, dy, &rx, &ry);
  if (i3)
  {
    vx.push_back(rx);
    vy.push_back(ry);
  }
  bool i4 = lines_intersect(ax, ay, bx, by, cx, cy, ex, ey, &rx, &ry);
  if (i4)
  {
    vx.push_back(rx);
    vy.push_back(ry);
  }

  //cout << "Rectangle intersections: " << i1 << " " << i2 << " " << i3 << " " << i4 << endl;
  //cout << "Number of intersections = " << vx.size() << endl;

  *rr = 0.;
  if (vx.size() == 2)
  {
    *rr = sqrt( (vx[0] - vx[1]) * (vx[0] - vx[1]) + (vy[0] - vy[1]) * (vy[0] - vy[1]) );
    //  cout << "Length of intersection = " << *rr << endl;
  }

  if (vx.size() == 1)
  {
    // find which point (A or B) is within the rectangle
    if (ax > cx && ay > cy && ax < dx && ay < dy)   // point A is inside the rectangle
    {
      //cout << "Point A is inside the rectangle." << endl;
      *rr = sqrt((vx[0] - ax) * (vx[0] - ax) + (vy[0] - ay) * (vy[0] - ay));
    }
    if (bx > cx && by > cy && bx < dx && by < dy)   // point B is inside the rectangle
    {
      //cout << "Point B is inside the rectangle." << endl;
      *rr = sqrt((vx[0] - bx) * (vx[0] - bx) + (vy[0] - by) * (vy[0] - by));
    }
  }

  if (i1 || i2 || i3 || i4)
  {
    return true;
  }

  return false;
}


int
PHG4BlockCellReco::CheckEnergy(PHCompositeNode *topNode)
{
  PHG4HitContainer *g4hit = findNode::getClass<PHG4HitContainer>(topNode, hitnodename.c_str());
  PHG4CylinderCellContainer *cells = findNode::getClass<PHG4CylinderCellContainer>(topNode, cellnodename);
  double sum_energy_g4hit = 0.;
  double sum_energy_cells = 0.;

  PHG4HitContainer::ConstRange hit_begin_end = g4hit->getHits();
  PHG4HitContainer::ConstIterator hiter;
  for (hiter = hit_begin_end.first; hiter != hit_begin_end.second; hiter++)
  {
    sum_energy_g4hit += hiter->second->get_edep();
  }

  PHG4CylinderCellContainer::ConstRange cell_begin_end = cells->getCylinderCells();
  PHG4CylinderCellContainer::ConstIterator citer;
  for (citer = cell_begin_end.first; citer != cell_begin_end.second; ++citer)
  {
    sum_energy_cells += citer->second->get_edep();
  }

  // the fractional eloss for particles traversing eta bins leads to minute rounding errors
  if (fabs(sum_energy_cells - sum_energy_g4hit)/sum_energy_g4hit > 1e-6) 
  {
    cout << "energy mismatch between cells: " << sum_energy_cells
         << " and hits: " << sum_energy_g4hit
         << " diff sum(cells) - sum(hits): " << sum_energy_cells - sum_energy_g4hit 
         << endl;
    return -1;
  }

  else
  {
    if (verbosity > 0)
    {
      cout << Name() << ":total energy for this event: " << sum_energy_g4hit << " GeV" << endl;
    }
  }

  return 0;
}
