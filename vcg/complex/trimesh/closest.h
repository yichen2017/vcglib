/****************************************************************************
* VCGLib                                                            o o     *
* Visual and Computer Graphics Library                            o     o   *
*                                                                _   O  _   *
* Copyright(C) 2004                                                \/)\/    *
* Visual Computing Lab                                            /\/|      *
* ISTI - Italian National Research Council                           |      *
*                                                                    \      *
* All rights reserved.                                                      *
*                                                                           *
* This program is free software; you can redistribute it and/or modify      *   
* it under the terms of the GNU General Public License as published by      *
* the Free Software Foundation; either version 2 of the License, or         *
* (at your option) any later version.                                       *
*                                                                           *
* This program is distributed in the hope that it will be useful,           *
* but WITHOUT ANY WARRANTY; without even the implied warranty of            *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
* GNU General Public License (http://www.gnu.org/licenses/gpl.txt)          *
* for more details.                                                         *
*                                                                           *
****************************************************************************/
/****************************************************************************
  History

$Log: not supported by cvs2svn $
Revision 1.27  2006/12/06 12:59:13  pietroni
added max distance to rayIterator

Revision 1.26  2006/11/21 16:06:54  ponchio
passing VDistFunct() to functions wanting a reference, not a value
(why a reference btw?)

Revision 1.25  2006/11/13 13:13:49  ponchio
Added usual typename.

Revision 1.24  2006/11/12 02:41:03  pietroni
added normalization of normal in DoRay functions

Revision 1.23  2006/11/10 11:41:49  pietroni
added DoRayFuntion that return interpolated normal

Revision 1.22  2006/09/20 17:18:26  ponchio
VDistFunct() at line 292 was passed as a temporary.
Invalid under g++. Fixed.

Revision 1.21  2006/02/09 08:38:04  pietroni
sintax error corrected

Revision 1.20  2006/02/08 17:02:41  pietroni
commented one  GetClosestFace function ... the code is the same then getClosest that return barycentric coordinates

Revision 1.19  2006/01/10 13:31:54  pietroni
correct pass of variable closest_pt by reference in getclosestFace function

Revision 1.18  2005/12/02 00:13:34  cignoni
Added and removed typenames for gcc compiling.
removed also some template arguments specifcation that gcc disliked...
commented out GetInSphereFace and SetMesh that are probably never used and i didnt succeed in compile

Revision 1.17  2005/10/05 17:02:52  pietroni
corrected bugs on GEtKClosestVert and GetInSphereVert

Revision 1.16  2005/10/03 16:19:07  spinelli
fixed some bugs

Revision 1.15  2005/10/03 13:59:39  pietroni
added GetInSphere and GetInBox functions
rensmed Functions respectively with Face suffix or Vertex suffix for query on vertex or faces

Revision 1.14  2005/09/30 13:10:37  pietroni
used functor defined in face/distance.h for distance point-face
used functor defined in intersection3.h for ray-triangle intersection
added GetKClosest and DoRay Functions

Revision 1.13  2005/09/28 08:30:48  cignoni
changed name of include, removed use of an undefined type (scalar instead of Scalar)
removed unused code portions (the old closest code)

Revision 1.12  2005/09/21 09:24:30  pietroni
Added RayIterators.
Added ClosestIterators on Triangles and Vertices.
Added Closest Functions on triangles and Vertices.

Revision 1.11  2005/09/19 13:36:24  pietroni
added ray iterator of faces

Revision 1.10  2005/09/16 11:53:51  cignoni
Small gcc compliling issues

Revision 1.9  2005/09/15 13:16:10  spinelli
fixed bugs

Revision 1.8  2005/09/15 11:15:00  pietroni
minor changes

Revision 1.7  2005/09/14 12:56:47  pietroni
used closest function from grid

Revision 1.6  2005/08/26 09:12:48  cignoni
changed  typedef A2UGridLink  da 'GridStaticPtr<MESH::FaceContainer,double>::Link' a  typedef 'GRID::Link'

Revision 1.5  2005/02/08 17:49:38  pietroni
added  if (!l->Elem()->IsD()) test on each element

Revision 1.4  2005/01/28 12:00:33  cignoni
small gcc compiling issues for namespaces

Revision 1.3  2005/01/24 11:47:23  cignoni
Now used also by the official Metro
Removed using namespace (NEVER IN HEADERS!)
Made  the computation of barycentric coords only when necessary
Renamed Mindistpoint to Closest

Revision 1.2  2005/01/21 17:13:09  pietroni
included distance.h changed Dist to  vcg::face::PointDistance

Revision 1.1  2004/10/04 15:32:16  ganovelli
moved from metro core

Revision 1.6  2004/05/14 00:34:36  ganovelli
header added

****************************************************************************/

#ifndef __VCG_TRIMESH_CLOSEST
#define __VCG_TRIMESH_CLOSEST
#include <math.h>

#include <vcg/space/point3.h>
#include <vcg/space/box3.h>
#include <vcg/space/point4.h>
#include <vcg/math/base.h>
#include <vcg/simplex/face/distance.h>
#include <vcg/simplex/vertex/distance.h>
#include <vcg/space/intersection3.h>
#include <vcg/space/index/space_iterators.h>

namespace vcg {
	namespace tri {

		//**MARKER CLASSES**//
		template <class MESH_TYPE,class OBJ_TYPE>
		class Tmark
		{
			MESH_TYPE *m;
		public:
			Tmark(){}
			void UnMarkAll(){m->UnMarkAll();}
			bool IsMarked(OBJ_TYPE* obj){return (m->IsMarked(obj));}
			void Mark(OBJ_TYPE* obj){m->Mark(obj);}
			void SetMesh(MESH_TYPE *_m)
			{m=_m;}
		};

		template <class MESH_TYPE>
		class FaceTmark:public Tmark<MESH_TYPE,typename MESH_TYPE::FaceType>
		{};

		template <class MESH_TYPE>
		class VertTmark
		{
		public:
		typedef typename  MESH_TYPE::VertexType VertexType;
		
			VertTmark(){}
			void UnMarkAll(){}
			bool IsMarked(VertexType*) { return false; }
			void Mark(VertexType*){}
			void SetMesh(void *m=0) {}
		};

		//**CLOSEST FUNCTION DEFINITION**//

		/*

		aka MetroCore
		data una mesh m e una ug sulle sue facce trova il punto di m piu' vicino ad
		un punto dato.
		*/

		// input: mesh, punto, griglia (gr), distanza limite (mdist)
		// output: normale (interpolata) alla faccia e punto piu' vicino su di essa, e coord baricentriche del punto trovato

		// Nota che il parametro template GRID non ci dovrebbe essere, visto che deve essere 
		// UGrid<MESH::FaceContainer >, ma non sono riuscito a definirlo implicitamente 

		template <class MESH, class GRID>
			typename MESH::FaceType * GetClosestFace( MESH & mesh, GRID & gr, const typename GRID::CoordType & _p, 
			const typename GRID::ScalarType & _maxDist, typename GRID::ScalarType & _minDist,
			typename GRID::CoordType &_closestPt, typename GRID::CoordType & _normf, 
			typename GRID::CoordType & _ip)
		{
			typedef typename GRID::ScalarType ScalarType;
			typedef Point3<ScalarType> Point3x;

			typedef FaceTmark<MESH> MarkerFace;
			MarkerFace mf;
			mf.SetMesh(&mesh);
			vcg::face::PointDistanceFunctor<ScalarType> FDistFunct;
			_minDist=_maxDist;
			typename MESH::FaceType* bestf= gr.GetClosest(FDistFunct, mf, _p, _maxDist, _minDist, _closestPt);

			if(_maxDist> ScalarType(fabs(_minDist)))
			{
				// f=bestf;
				typename MESH::ScalarType alfa, beta, gamma;
				//calcolo normale con interpolazione trilineare
				InterpolationParameters(*bestf,_closestPt, alfa, beta, gamma);
				_normf =  (bestf->V(0)->cN())*alfa+
					(bestf->V(1)->cN())*beta+
					(bestf->V(2)->cN())*gamma ;
				_ip=Point3x(alfa,beta,gamma);
				//normf.Normalize(); inutile si assume le normali ai vertici benfatte										

				_minDist = fabs(_minDist);
				return(bestf);
			}
			return (0);
		}

		/*template <class MESH, class GRID>
			typename MESH::FaceType * GetClosestFace( MESH & mesh,GRID & gr,const typename GRID::CoordType & _p, 
			const typename GRID::ScalarType & _maxDist,typename GRID::ScalarType & _minDist,
			typename GRID::CoordType &_closestPt,typename GRID::CoordType & _normf)
		{
			Point3<typename GRID::ScalarType> _ip;
			typedef FaceTmark<MESH> MarkerFace;
			MarkerFace mf;
			mf.SetMesh(&mesh);
			vcg::face::PointDistanceFunctor FDistFunct;
			typename MESH::FaceType* bestf= gr.GetClosest(FDistFunct,mf,_p,_maxDist,_minDist,_closestPt) );
		}*/

		template <class MESH, class GRID>
			typename MESH::FaceType * GetClosestFace( MESH & mesh,GRID & gr,const typename GRID::CoordType & _p, 
			const typename GRID::ScalarType & _maxDist,typename GRID::ScalarType & _minDist,
			typename GRID::CoordType &_closestPt)
		{
			typedef typename GRID::ScalarType ScalarType;
			typedef Point3<ScalarType> Point3x;
			typedef FaceTmark<MESH> MarkerFace;
			MarkerFace mf;
			mf.SetMesh(&mesh);
			vcg::face::PointDistanceFunctor<ScalarType> PDistFunct;
			_minDist=_maxDist;
			return (gr.GetClosest(PDistFunct,mf,_p,_maxDist,_minDist,_closestPt));
		}

		template <class MESH, class GRID>
			typename MESH::VertexType * GetClosestVertex( MESH & mesh,GRID & gr,const typename GRID::CoordType & _p, 
			const typename GRID::ScalarType & _maxDist,typename GRID::ScalarType & _minDist )
		{
			typedef typename GRID::ScalarType ScalarType;
			typedef Point3<ScalarType> Point3x;
			typedef VertTmark<MESH> MarkerVert;
			MarkerVert mv;
			mv.SetMesh(&mesh);
			typedef vcg::vertex::PointDistanceFunctor<typename MESH::ScalarType> VDistFunct;
			VDistFunct fn;
			_minDist=_maxDist;
			Point3x _closestPt;
			return (gr.template GetClosest<VDistFunct,MarkerVert>(fn,mv,_p,_maxDist,_minDist,_closestPt));
		}

		template <class MESH, class GRID>
			typename MESH::VertexType * GetClosestVertexNormal( MESH & mesh,GRID & gr,const typename MESH::VertexType & _p, 
			const typename GRID::ScalarType & _maxDist,typename GRID::ScalarType & _minDist )
		{
			typedef typename GRID::ScalarType ScalarType;
			typedef Point3<ScalarType> Point3x;
			typedef VertTmark<MESH> MarkerVert;
			MarkerVert mv;
			mv.SetMesh(&mesh);
			typedef vcg::vertex::PointNormalDistanceFunctor<typename MESH::VertexType> VDistFunct;
			VDistFunct fn;
			_minDist=_maxDist;
			Point3x _closestPt;
			return (gr.template GetClosest <VDistFunct,MarkerVert>(fn,mv,_p,_maxDist,_minDist,_closestPt));
		}

		template <class MESH, class GRID, class OBJPTRCONTAINER,class DISTCONTAINER, class POINTCONTAINER>
			unsigned int GetKClosestFace(MESH & mesh,GRID & gr, const unsigned int _k, 
			const typename GRID::CoordType & _p, const typename GRID::ScalarType & _maxDist,
			OBJPTRCONTAINER & _objectPtrs,DISTCONTAINER & _distances, POINTCONTAINER & _points)
		{
			typedef FaceTmark<MESH> MarkerFace;
			MarkerFace mf;
			mf.SetMesh(&mesh);
		  vcg::face::PointDistanceFunctor<typename MESH::ScalarType> FDistFunct;
			return (gr.GetKClosest /*<vcg::face::PointDistanceFunctor, MarkerFace,OBJPTRCONTAINER,DISTCONTAINER,POINTCONTAINER>*/
				(FDistFunct,mf,_k,_p,_maxDist,_objectPtrs,_distances,_points));
		}

		template <class MESH, class GRID, class OBJPTRCONTAINER,class DISTCONTAINER, class POINTCONTAINER>
			unsigned int GetKClosestVertex(MESH & mesh,GRID & gr, const unsigned int _k, 
			const typename GRID::CoordType & _p, const typename GRID::ScalarType & _maxDist,
			OBJPTRCONTAINER & _objectPtrs,DISTCONTAINER & _distances, POINTCONTAINER & _points)
		{
			typedef VertTmark<MESH> MarkerVert;
			MarkerVert mv;
			mv.SetMesh(&mesh);			
			typedef vcg::vertex::PointDistanceFunctor<typename MESH::ScalarType> VDistFunct;
			VDistFunct distFunct;
			return (gr.GetKClosest/* <VDistFunct,MarkerVert,OBJPTRCONTAINER,DISTCONTAINER,POINTCONTAINER>*/
				(distFunct,mv,_k,_p,_maxDist,_objectPtrs,_distances,_points));
		}

		template <class MESH, class GRID, class OBJPTRCONTAINER, class DISTCONTAINER, class POINTCONTAINER>
			unsigned int GetInSphereFace(MESH & mesh,
			GRID & gr,
			const typename GRID::CoordType & _p,
			const typename GRID::ScalarType & _r,
			OBJPTRCONTAINER & _objectPtrs,
			DISTCONTAINER & _distances, 
			POINTCONTAINER & _points)
		{
			typedef FaceTmark<MESH> MarkerFace;
			MarkerFace mf;
			mf.SetMesh(&mesh);
			typedef vcg::face::PointDistanceFunctor<typename MESH::ScalarType> FDistFunct;
			return (gr.GetInSphere/*<FDistFunct,MarkerFace,OBJPTRCONTAINER,DISTCONTAINER,POINTCONTAINER>*/
				(FDistFunct(),mf,_p,_r,_objectPtrs,_distances,_points));
		}

		template <class MESH, class GRID, class OBJPTRCONTAINER, class DISTCONTAINER, class POINTCONTAINER>
			unsigned int GetInSphereVertex(MESH & mesh,
			GRID & gr,
			const typename GRID::CoordType & _p,
			const typename GRID::ScalarType & _r,
			OBJPTRCONTAINER & _objectPtrs,
			DISTCONTAINER & _distances, 
			POINTCONTAINER & _points)
		{
			typedef VertTmark<MESH> MarkerVert;
			MarkerVert mv;
			mv.SetMesh(&mesh);
			typedef vcg::vertex::PointDistanceFunctor<typename MESH::ScalarType> VDistFunct;
			VDistFunct fn;
			return (gr.GetInSphere/*<VDistFunct,MarkerVert,OBJPTRCONTAINER,DISTCONTAINER,POINTCONTAINER>*/
				(fn, mv,_p,_r,_objectPtrs,_distances,_points));
		}

		template <class MESH, class GRID, class OBJPTRCONTAINER>
			unsigned int GetInBoxFace(MESH & mesh,
			GRID & gr,
			const vcg::Box3<typename GRID::ScalarType> _bbox,
			OBJPTRCONTAINER & _objectPtrs) 
		{
			typedef FaceTmark<MESH> MarkerFace;
			MarkerFace mf;
			mf.SetMesh(&mesh);
			return(gr.GetInBox/*<MarkerFace,OBJPTRCONTAINER>*/(mf,_bbox,_objectPtrs));
		}

		template <class MESH, class GRID, class OBJPTRCONTAINER>
			unsigned int GetInBoxVertex(MESH & mesh,
			GRID & gr,
			const vcg::Box3<typename GRID::ScalarType> _bbox,
			OBJPTRCONTAINER & _objectPtrs) 
		{
			typedef VertTmark<MESH> MarkerVert;
			MarkerVert mv;
			mv.SetMesh(&mesh);
			return(gr.GetInBox/*<MarkerVert,OBJPTRCONTAINER>*/(mv,_bbox,_objectPtrs));
		}

		template <class MESH, class GRID>
			typename GRID::ObjPtr DoRay(MESH & mesh,GRID & gr, const Ray3<typename GRID::ScalarType> & _ray,
			const typename GRID::ScalarType & _maxDist, typename GRID::ScalarType & _t) 
		{
			typedef typename MESH::FaceType FaceType;
			typedef typename MESH::ScalarType ScalarType;
			typedef FaceTmark<MESH> MarkerFace;
			MarkerFace mf;
			mf.SetMesh(&mesh);
			Ray3<typename GRID::ScalarType> _ray1=_ray;
			_ray1.Normalize();
			typedef vcg::RayTriangleIntersectionFunctor<true> FintFunct;
			return(gr.DoRay(FintFunct(),mf,_ray1,_maxDist,_t));
		}
		
		template <class MESH, class GRID>
			typename GRID::ObjPtr DoRay(MESH & mesh,GRID & gr, const Ray3<typename GRID::ScalarType> & _ray,
										const typename GRID::ScalarType & _maxDist, 
										typename GRID::ScalarType & _t, 
										typename GRID::CoordType & _normf) 
		{
			typedef typename MESH::FaceType FaceType;
			typedef typename MESH::ScalarType ScalarType;
			typedef FaceTmark<MESH> MarkerFace;
			MarkerFace mf;
			mf.SetMesh(&mesh);
			typedef vcg::RayTriangleIntersectionFunctor<true> FintFunct;
			Ray3<typename GRID::ScalarType> _ray1=_ray;
			_ray1.Normalize();
			FaceType *f=gr.DoRay(FintFunct(),mf,_ray1,_maxDist,_t);
			typename GRID::CoordType dir=_ray.Direction();
			dir.Normalize();
			typename GRID::CoordType int_point=_ray.Origin()+_ray1.Direction()*_t;
			typename GRID::ScalarType alfa,beta,gamma;
			if (f!=NULL)
			{
				f->InterpolationParameters(int_point, alfa, beta, gamma);
				_normf =  (f->V(0)->cN())*alfa+
						  (f->V(1)->cN())*beta+
						  (f->V(2)->cN())*gamma ;
			}
			return f;
		}

		//**ITERATORS DEFINITION**//

		template <class GRID,class MESH>
		class ClosestFaceIterator:public vcg::ClosestIterator<GRID,
			vcg::face::PointDistanceFunctor<typename MESH::ScalarType>, FaceTmark<MESH> >
		{
		public:
			typedef GRID GridType;
			typedef MESH MeshType;
			typedef FaceTmark<MESH> MarkerFace;
			typedef vcg::face::PointDistanceFunctor<typename MESH::ScalarType> PDistFunct;
			typedef vcg::ClosestIterator<GRID,PDistFunct, FaceTmark<MESH> > ClosestBaseType;
			typedef typename MESH::FaceType FaceType;
			typedef typename MESH::ScalarType ScalarType;

			//ClosestFaceIterator(GridType &_Si):ClosestBaseType(_Si,PDistFunct<FaceType,ScalarType>()){}
			ClosestFaceIterator(GridType &_Si):ClosestBaseType(_Si,PDistFunct()){}

//    Commented out: it seems unuseful and make gcc complain. p.
      void SetMesh(MeshType *m)
			{this->tm.SetMesh(m);}
		};

		template <class GRID,class MESH>
		class ClosestVertexIterator:public vcg::ClosestIterator<GRID, vcg::vertex::PointDistanceFunctor<typename MESH::ScalarType>, VertTmark<MESH> >
		{
		public:
			typedef GRID GridType;
			typedef MESH MeshType;
			typedef VertTmark<MESH> MarkerVert;
			typedef vcg::vertex::PointDistanceFunctor<typename MESH::ScalarType> VDistFunct;
			typedef vcg::ClosestIterator<GRID, VDistFunct, VertTmark<MESH> > ClosestBaseType;
            VDistFunct fn;
			ClosestVertexIterator(GridType &_Si):ClosestBaseType(_Si,fn){}

//    Commented out: it seems unuseful and make gcc complain. p.
			void SetMesh(MeshType *m)
			{this->tm.SetMesh(m);}
		};

		template <class GRID,class MESH>
		class TriRayIterator:public vcg::RayIterator<GRID,vcg::RayTriangleIntersectionFunctor<true>,FaceTmark<MESH> >
		{
		public:
			typedef GRID GridType;
			typedef MESH MeshType;
			typedef FaceTmark<MESH> MarkerFace;
			typedef vcg::RayTriangleIntersectionFunctor<true> FintFunct;
			typedef vcg::RayIterator<GRID,FintFunct, FaceTmark<MESH> > RayBaseType;
			typedef typename MESH::FaceType FaceType;
			typedef typename MESH::ScalarType ScalarType;

			TriRayIterator(GridType &_Si,const ScalarType &max_d):RayBaseType(_Si,FintFunct(),max_d){}

//    Commented out: it  seems unuseful and make gcc complain. p.
			void SetMesh(MeshType *m)
			{this->tm.SetMesh(m);}

		};

	}	 // end namespace tri
}	 // end namespace vcg

#endif
