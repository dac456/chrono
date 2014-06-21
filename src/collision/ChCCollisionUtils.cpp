//
// PROJECT CHRONO - http://projectchrono.org
//
// Copyright (c) 2013 Project Chrono
// All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can be 
// found in the LICENSE file at the top level of the distribution
// and at http://projectchrono.org/license-chrono.txt.
//

///////////////////////////////////////////////////
//
//   ChCCollisionUtils.cpp
//
// ------------------------------------------------
//             www.deltaknowledge.com
// ------------------------------------------------
///////////////////////////////////////////////////



#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <memory.h>

#include "core/ChTrasform.h"
#include "collision/ChCCollisionUtils.h"
#include "physics/ChGlobal.h"
#include "physics/ChSolvmin.h"
#include "physics/ChNlsolver.h"
#include "geometry/ChCTriangle.h"
#include "geometry/ChCSphere.h"
#include "geometry/ChCBox.h"
#include "geometry/ChCTriangleMeshConnected.h"

namespace chrono
{
namespace collision 
{



//////////////UTILITY

#define EPS 1e-20
#define EPS_TRIDEGEN 1e-10

//
// Calculate the line segment PaPb that is the shortest route between
// two lines P1P2 and P3P4. Calculate also the values of mua and mub where
//    Pa = P1 + mua (P2 - P1)
//    Pb = P3 + mub (P4 - P3)
// Return FALSE if no solution exists.

int ChCollisionUtils::LineLineIntersect(
   Vector p1, Vector p2,Vector p3,Vector p4,Vector *pa,Vector *pb,
   double *mua, double *mub)
{
   Vector p13,p43,p21;
   double d1343,d4321,d1321,d4343,d2121;
   double numer,denom;

   p13.x = p1.x - p3.x;
   p13.y = p1.y - p3.y;
   p13.z = p1.z - p3.z;
   p43.x = p4.x - p3.x;
   p43.y = p4.y - p3.y;
   p43.z = p4.z - p3.z;
   if (fabs(p43.x)  < EPS && fabs(p43.y)  < EPS && fabs(p43.z)  < EPS)
      return(FALSE);
   p21.x = p2.x - p1.x;
   p21.y = p2.y - p1.y;
   p21.z = p2.z - p1.z;
   if (fabs(p21.x)  < EPS && fabs(p21.y)  < EPS && fabs(p21.z)  < EPS)
      return(FALSE);

   d1343 = p13.x * p43.x + p13.y * p43.y + p13.z * p43.z;
   d4321 = p43.x * p21.x + p43.y * p21.y + p43.z * p21.z;
   d1321 = p13.x * p21.x + p13.y * p21.y + p13.z * p21.z;
   d4343 = p43.x * p43.x + p43.y * p43.y + p43.z * p43.z;
   d2121 = p21.x * p21.x + p21.y * p21.y + p21.z * p21.z;

   denom = d2121 * d4343 - d4321 * d4321;
   if (fabs(denom) < EPS)
      return(FALSE);
   numer = d1343 * d4321 - d1321 * d4343;

   *mua = numer / denom;
   *mub = (d1343 + d4321 * (*mua)) / d4343;

   pa->x = p1.x + *mua * p21.x;
   pa->y = p1.y + *mua * p21.y;
   pa->z = p1.z + *mua * p21.z;
   pb->x = p3.x + *mub * p43.x;
   pb->y = p3.y + *mub * p43.y;
   pb->z = p3.z + *mub * p43.z;

   return(TRUE);
}



/////////////////////////////////////

// Calculate distance between a point p and a line identified
// with segment dA,dB. Returns distance. Also, the mu value reference
// tells if the nearest projection of point on line falls into segment (for mu 0...1)

double ChCollisionUtils::PointLineDistance(Vector p, Vector dA, Vector dB, double& mu, int& is_insegment)
{
	mu=-1.0;
	is_insegment = 0;
	double mdist=10e34;

	Vector vseg = Vsub(dB,dA);
	Vector vdir = Vnorm(vseg);
	Vector vray = Vsub(p,dA);

	mdist = Vlenght(Vcross(vray,vdir));
	mu = Vdot(vray,vdir)/Vlenght(vseg);

	if ((mu>=0) && (mu<=1.0))
		is_insegment = 1;

	return mdist;
}



/////////////////////////////////////

// Calculate distance of a point from a triangle surface.
// Also computes if projection is inside the triangle.
//

double ChCollisionUtils::PointTriangleDistance(Vector B, Vector A1, Vector A2, Vector A3,
							   double& mu, double& mv, int& is_into,
							   Vector& Bprojected)
{
	// defaults
	is_into = 0;
	mu=mv=-1;
	double mdistance = 10e22;

	Vector Dx, Dy, Dz, T1, T1p;

	Dx= Vsub (A2, A1);
	Dz= Vsub (A3, A1);
	Dy= Vcross(Dz,Dx);

	double dylen = Vlenght(Dy);

	if(fabs(dylen)<EPS_TRIDEGEN)	// degenere triangle
		return mdistance;

	Dy= Vmul(Dy,1.0/dylen);

	ChMatrix33<> mA;
	ChMatrix33<> mAi;
	mA.Set_A_axis(Dx,Dy,Dz);

	// invert triangle coordinate matrix -if singular matrix, was degenerate triangle-.
	if (fabs(mA.FastInvert(&mAi)) <0.000001)
		return mdistance;

	T1 = mAi.Matr_x_Vect( Vsub (B, A1) );
	T1p = T1;
	T1p.y=0;
	mu = T1.x;
	mv = T1.z;
	if (mu >=0  &&  mv >=0  &&  mv<=1.0-mu)
	{
		is_into = 1;
		mdistance = fabs(T1.y);
		Bprojected = Vadd(A1, mA.Matr_x_Vect(T1p));
	}

	return mdistance;
}



/////////////////////////////////////



 
int DegenerateTriangle(Vector Dx, Vector Dy)
{
	Vector vcr;
	vcr = Vcross(Dx,Dy);
	if (fabs(vcr.x) < EPS_TRIDEGEN && fabs(vcr.y) < EPS_TRIDEGEN && fabs(vcr.z) < EPS_TRIDEGEN )
		return TRUE;
	return FALSE;
}



ChConvexHullLibraryWrapper::ChConvexHullLibraryWrapper()
{
}

void ChConvexHullLibraryWrapper::ComputeHull(std::vector< ChVector<> >& points, geometry::ChTriangleMeshConnected& vshape)
{
	HullLibrary hl;
	HullResult  hresult;
	HullDesc    desc;

	desc.SetHullFlag(QF_TRIANGLES);

	btVector3* btpoints = new btVector3[points.size()];
	for (unsigned int ip = 0; ip < points.size(); ++ip)
	{
		btpoints[ip].setX( (btScalar)points[ip].x );
		btpoints[ip].setY( (btScalar)points[ip].y );
		btpoints[ip].setZ( (btScalar)points[ip].z );
	}
	desc.mVcount       = points.size();
	desc.mVertices     = btpoints;
	desc.mVertexStride = sizeof( btVector3 );

	HullError hret = hl.CreateConvexHull(desc,hresult);

	if ( hret == QE_OK )
	{
		vshape.Clear();

		vshape.getIndicesVertexes().resize(hresult.mNumFaces);
		for (unsigned int it= 0; it < hresult.mNumFaces; ++it)
		{
			hresult;
			vshape.getIndicesVertexes()[it] = 
				ChVector<int> ( hresult.m_Indices[it*3+0],
								hresult.m_Indices[it*3+1],
								hresult.m_Indices[it*3+2] );
		}
		vshape.getCoordsVertices().resize(hresult.mNumOutputVertices);
		for (unsigned int iv= 0; iv < hresult.mNumOutputVertices; ++iv)
		{
			hresult;
			vshape.getCoordsVertices()[iv] = 
				ChVector<>    ( hresult.m_OutputVertices[iv].x(),
								hresult.m_OutputVertices[iv].y(),
								hresult.m_OutputVertices[iv].z() );
		}
	}

	delete[] btpoints;

	hl.ReleaseResult(hresult);
}



} // END_OF_NAMESPACE____
} // END_OF_NAMESPACE____


////// end
