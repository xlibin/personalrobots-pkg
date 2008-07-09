#include "smartScan.h"

#include "math.h"
#include <list>

#include "vtkFloatArray.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"
#include "vtkDelaunay2D.h"
#include "vtkDelaunay3D.h"
#include "vtkCellArray.h"
#include "vtkPointLocator.h"
#include "vtkIterativeClosestPointTransform.h"
#include "vtkMatrix4x4.h"
#include "vtkCellLocator.h"
#include "vtkLandmarkTransform.h"

#include "newmat10/newmatap.h"
#include <iomanip>
#include "newmat10/newmatio.h"

using namespace scan_utils;

#define TFNP libTF::TransformReference::NO_PARENT
#define TFRT libTF::TransformReference::ROOT_FRAME

SmartScan::SmartScan()
{
	mNumPoints = 0;
	mNativePoints = NULL;
	mVtkData = NULL;
	mVtkPointLocator = NULL;
}

SmartScan::~SmartScan()
{
	clearData();
}

void SmartScan::clearData()
{
	if (mNativePoints) {
		delete [] mNativePoints;
		mNativePoints = NULL;
	}
	deleteVtkData();
	mNumPoints = 0;
	//set transform to identity
	mTransform.setWithEulers(TFRT, TFNP,0,0,0,0,0,0,0);
}

void SmartScan::setPoints(int numPoints, const std_msgs::Point3DFloat32 *points)
{
	// for now we are making explicit copies, but later this will be changed to not
	// copy the data but just store it
	clearData();
	mNumPoints = numPoints;
	mNativePoints = new std_msgs::Point3DFloat32[mNumPoints];
	for (int i=0; i<mNumPoints; i++) {
		mNativePoints[i] = points[i];
	}
}

/*!
  Writes the data in this scan to an output stream. Format is:
  
  number of points on the first line
  one point per line as  x y z intensity
*/
void SmartScan::writeToFile(std::iostream &output)
{
	float intensity = 0;
	output << mNumPoints << std::endl;
	for (int i=0; i < mNumPoints; i++) {
		output << mNativePoints[i].x << " " << mNativePoints[i].y << " " 
		       << mNativePoints[i].z << " " << intensity << std::endl;
	}
	fprintf(stderr,"Written %d points to file\n", mNumPoints);
}

/*!  
  Reads in data from an input stream. Format is expected to be
  identical to the one created by writeToFile(...)
*/
bool SmartScan::readFromFile(std::iostream &input)
{
	int n;
	float x,y,z,intensity;
	input >> n;
	if (n <= 0) {
		return false;
	}

	clearData();

	mNumPoints = n;
	mNativePoints =  new std_msgs::Point3DFloat32[mNumPoints];

	int i;
	for (i=0; i<mNumPoints; i++) {
		if (input.eof()) break;
		input >> x >> y >> z >> intensity;
		mNativePoints[i].x = x;
		mNativePoints[i].y = y;
		mNativePoints[i].z = z;

	}
	if (i!=mNumPoints) return false;
	return true;
}

void SmartScan::createVtkData()
{
	if (mVtkData) deleteVtkData();

	// Create a float array which represents the points.
	vtkFloatArray* pcoords = vtkFloatArray::New();
	// Note that by default, an array has 1 component.
	// We have to change it to 3 for points
	pcoords->SetNumberOfComponents(3);
	// We ask pcoords to allocate room for all the tuples we need
	pcoords->SetNumberOfTuples(mNumPoints);
	// Assign each tuple. 
	for (int i=0; i<mNumPoints; i++){
		pcoords->SetTuple3(i, mNativePoints[i].x, mNativePoints[i].y, mNativePoints[i].z);
	}
	// Create vtkPoints and assign pcoords as the internal data array.
	vtkPoints* points = vtkPoints::New();
	points->SetData(pcoords);
	// Create vtkPointSet and assign vtkPoints as internal data
	mVtkData = vtkPolyData::New();
	mVtkData->SetPoints(points);

	//for some functions it seems we also need the points represented as "cells"...
	vtkCellArray *cells = vtkCellArray::New();
	for (int i=0; i<mNumPoints; i++) {
		cells->InsertNextCell(1);
		cells->InsertCellPoint(i);
	}
	mVtkData->SetVerts(cells);

	//also create and populate point locator
	mVtkPointLocator = vtkPointLocator::New();
	mVtkPointLocator->SetDataSet(mVtkData);
	mVtkPointLocator->BuildLocator();

}

void SmartScan::deleteVtkData()
{
	if (!mVtkData) return;
	mVtkData->Delete();
	mVtkData = NULL;
	assert(mVtkPointLocator);
	mVtkPointLocator->Delete();
	mVtkPointLocator = NULL;
}

vtkPolyData* SmartScan::getVtkData()
{
	if (!hasVtkData()) createVtkData();
	assert(mVtkData);
	return mVtkData;
}

vtkPointLocator* SmartScan::getVtkLocator()
{
	if (!hasVtkData()) createVtkData();
	assert(mVtkPointLocator);
	return mVtkPointLocator;
}
/*! Performs a 2D Delaunay triangulation of the point cloud. This is
    equivalent to projecting all point onto the x-y plane (by dropping
    the z coordinate), performing the 2D triangulation then pushing
    the resulting triangles back into 3D space by adding the z
    coordinate back.  

    \param tolerance Points that are closer than this value (after
    projection on x-y plane) will be merged together

    \param alpha No points that are further apart than this values
    will be joined by a triangle

    The output lists the triangles produced by triangulation.  It is
  the responsability of the caller to free this memory.
 */
std::vector<Triangle> *SmartScan::delaunayTriangulation(double tolerance, double alpha)
{
	std::vector<Triangle> *triangles = new std::vector<Triangle>;

	// create the Delaunay triangulation filter
	vtkDelaunay2D *delny = vtkDelaunay2D::New();
	// set our point set as input
	delny->SetInput( getVtkData() );
	delny->SetTolerance(tolerance);
	delny->SetAlpha(alpha);

	// get the output
	vtkPolyData *output = delny->GetOutput();
	// run the filter
	delny->Update();

	//we should now have the result in the output
	int nTri = output->GetNumberOfStrips();
	int nPol = output->GetNumberOfPolys();
	fprintf(stderr,"Delaunay: %d tris and %d polys\n",nTri,nPol);

	//it seems output is as polygons
	vtkCellArray *polygons = output->GetPolys();
	polygons->InitTraversal();
	int nPts, *pts;
	double coords[9];
	while( polygons->GetNextCell(nPts,pts) ){
		if (nPts!=3) {
			fprintf(stderr,"Delaunay cell does not have 3 points\n");
			continue;
		}
		output->GetPoint( pts[0], &(coords[0]) );
		output->GetPoint( pts[1], &(coords[3]) );
		output->GetPoint( pts[2], &(coords[6]) );
		triangles->push_back( Triangle(coords) );

	}

	delny->Delete();
	return triangles;
}

/*! Performs a 3D Delaunay triangulation of the point cloud. This
    actually means creating a volume composed of tetrahedra that best
    approximates the cloud in a Delaunay sense.

    \param tolerance Points that are closer than this value (after
    projection on x-y plane) will be merged together

    \param alpha No points that are further apart than this values
    will be joined by a tetrahedron.

    The output list all four triangles that compose the each of the
    resulting tetrahedra. It automatically contains a surface mesh,
    but many of the returned triangles will be "inside" the volume
    produced.  It is the responsability of the caller to free this
    memory.
 */
std::vector<Triangle> *SmartScan::delaunayTriangulation3D(double tolerance, double alpha)
{
	std::vector<Triangle> *triangles = new std::vector<Triangle>;

	// create the Delaunay triangulation filter
	vtkDelaunay3D *delny = vtkDelaunay3D::New();
	// set our point set as input
	delny->SetInput( getVtkData() );
	// tolerance is distance that nearly coincident points are merged together
	delny->SetTolerance(tolerance);
	// I'm not really sure what this Alpha thing is
	delny->SetAlpha(alpha);

	// get the output
	vtkUnstructuredGrid *output = delny->GetOutput();
	// run the filter
	delny->Update();

	vtkCellArray *cells = output->GetCells();
	cells->InitTraversal();
	int nPts, *pts, tets=0;
	double c1[3], c2[3], c3[3], c4[3];

	while( cells->GetNextCell(nPts,pts) ){
		if (nPts!=4) {
			//not a tetrahedron
			continue;
		}
		output->GetPoint( pts[0], c1 );
		output->GetPoint( pts[1], c2 );
		output->GetPoint( pts[2], c3 );
		output->GetPoint( pts[3], c4 );
		triangles->push_back( Triangle(c2, c1, c3) );
		triangles->push_back( Triangle(c1, c2, c4) );
		triangles->push_back( Triangle(c3, c1, c4) );
		triangles->push_back( Triangle(c2, c3, c4) );
		tets++;
	}
	fprintf(stderr,"3D Delaunay: %d tets\n",tets);
	delny->Delete();
	return triangles;
}

/*! Crops the point cloud to a 3D bounding box

  \param x \param y \param z The location of the center of the crop
  bounding box

  \param dx \param \dy \param dz The dimensions of the crop bounding
  box.
*/
void SmartScan::crop(float x, float y, float z, float dx, float dy, float dz)
{
	std_msgs::Point3DFloat32 *newPoints;
	int numNewPoints = 0;

	//we are passed the overall size of the crop box; let's get the halves
	dx = dx/2; dy = dy/2; dz = dz/2;

	//we are doing two passes; one to count how many points we are keeping
	//(so we can allocate memory) and one to actually copy the points

	for (int i=0; i<mNumPoints; i++) {
		if (mNativePoints[i].x > x + dx) continue;
		if (mNativePoints[i].x < x - dx) continue;
		if (mNativePoints[i].y > y + dy) continue;
		if (mNativePoints[i].y < y - dy) continue;
		if (mNativePoints[i].z > z + dz) continue;
		if (mNativePoints[i].z < z - dz) continue;
		numNewPoints++;
	}
	newPoints = new std_msgs::Point3DFloat32[numNewPoints];
	int j=0;
	for (int i=0; i<mNumPoints; i++) {
		if (mNativePoints[i].x > x + dx) continue;
		if (mNativePoints[i].x < x - dx) continue;
		if (mNativePoints[i].y > y + dy) continue;
		if (mNativePoints[i].y < y - dy) continue;
		if (mNativePoints[i].z > z + dz) continue;
		if (mNativePoints[i].z < z - dz) continue;
		newPoints[j] = mNativePoints[i];
		j++;
	}
	assert(j==numNewPoints);
	fprintf(stderr,"Cropped from %d to %d points\n",mNumPoints, numNewPoints);

	mNumPoints = numNewPoints;
	delete [] mNativePoints;
	mNativePoints = newPoints;

	if ( hasVtkData() ) {
		deleteVtkData();
		createVtkData();
	}

}

/*! Computes the transform that registers this scan to another scan.
  
  \param target Scan that we are registering against

  Returns the transform as a 4x4 matrix saved in a double[16] in
  row-major order. It is the responsability of the caller to free this
  memory.

  THIS FUNCTION HAS NOT BEEN EXTENSIVELY TESTED YET.
 */
double* SmartScan::ICPTo(SmartScan* target)
{
	vtkIterativeClosestPointTransform *vtkTransform = vtkIterativeClosestPointTransform::New();
	vtkTransform->SetMaximumNumberOfIterations(1000);
	fprintf(stderr,"max iterations: %d\n",vtkTransform->GetMaximumNumberOfIterations());

	vtkTransform->SetSource( getVtkData() );
	vtkTransform->SetTarget( target->getVtkData() );
	vtkTransform->GetLandmarkTransform()->SetModeToRigidBody();

	fprintf(stderr,"Iterations used at beginning: %d\n", vtkTransform->GetNumberOfIterations() );
	fprintf(stderr,"Points: %d and %d\n",this->getVtkData()->GetNumberOfPoints(), 
		target->getVtkData()->GetNumberOfPoints() );

	/*
	int nb_points = getVtkData()->GetNumberOfPoints();
	vtkCellLocator *loc = vtkCellLocator::New();
	loc->SetDataSet( target->getVtkData() );
	loc->SetNumberOfCellsPerBucket(1);
	loc->BuildLocator();

	vtkIdType cell_id;
	int sub_id;
	double outPoint[3], dist2;

	// Fill points with the closest points to each vertex in input
	for(int i = 0; i < nb_points; i++){
		loc->FindClosestPoint(this->getVtkData()->GetPoint(i),
				      outPoint,
				      cell_id,
				      sub_id,
				      dist2);
		fprintf(stderr,"Closest found: %f %f %f\n",outPoint[0], outPoint[1], outPoint[2]);
		//closestp->SetPoint(i, outPoint);
	}
	*/

	double* transf = new double[16];
	vtkMatrix4x4 *vtkMat = vtkMatrix4x4::New();
	vtkTransform->GetMatrix(vtkMat);

	for(int i=0; i<4; i++) {
		for(int j=0; j<4; j++) {
			transf[4*i+j] = vtkMat->GetElement(i,j);
		}
	}

	fprintf(stderr,"ICP done. Result:\n");
	vtkMat->Print(std::cerr);
	fprintf(stderr,"Iterations used: %d. Mean dist: %f\n",vtkTransform->GetNumberOfIterations(),
		vtkTransform->GetMeanDistance());

	//	fprintf(stderr,"%f %f %f %f\n",transf[0], transf[1], transf[2], transf[3]);
	//fprintf(stderr,"%f %f %f %f\n",transf[4], transf[5], transf[6], transf[7]);
	//fprintf(stderr,"%f %f %f %f\n",transf[8], transf[9], transf[10], transf[11]);
	//fprintf(stderr,"%f %f %f %f\n",transf[12], transf[13], transf[14], transf[15]);

	vtkTransform->Delete();
	vtkMat->Delete();
	return transf;
}

/*! Removes all point that have fewer than \param nbrs neighbors
    within a sphere of radius \param radius.
 */
void SmartScan::removeOutliers(float radius, int nbrs)
{

	if (! hasVtkData() ) createVtkData();
	vtkIdList *result = vtkIdList::New();

	// allocate memory as if we will keep all points, but later we'll do a copy and a delete
	std_msgs::Point3DFloat32 *newPoints = new std_msgs::Point3DFloat32[mNumPoints];
	int numNewPoints = 0;
	for (int i=0; i<mNumPoints; i++) {
		result->Reset();
		getVtkLocator()->FindPointsWithinRadius( radius, 
							  mNativePoints[i].x,
							  mNativePoints[i].y,
							  mNativePoints[i].z,
							  result );
		//there is always at least one point in the result (the point itself)
		if (result->GetNumberOfIds() > nbrs) {
			//keep this point
			newPoints[numNewPoints] = mNativePoints[i];
			numNewPoints++;
		}
	}
	result->Delete();

	//keep only the new points
	fprintf(stderr,"Removed outliers from %d to %d\n",mNumPoints,numNewPoints);
	//this will copy the points again, so we are doing two copies in order to only
	//compute the nbrs once
	setPoints(numNewPoints,newPoints);
	//we can now delete this array which has worng size anyway
	delete [] newPoints;
	//and re-create vtk data (which does yet another copy...)
	createVtkData();

}

/*!  Removes all points whose normals are perpendicular to the scanner
  direction. For now, we assume the scanner is at location
  (0,0,0). The assumption is that points with this characteristic are
  scanned with high error or are "ghosting" or "veil" artifacts.

  \param threshold The minimum difference in degrees between normal
  direction and scanner perpendicular direction for keeping
  points. For example, if \param threshold = 10 all points whose
  normal is closer than 10 degrees to beeing perpendicular to the
  scanner direction are removed.

  \param removeOutliers What do we do with points that have to few
  neighbors top compute normals: if this flag is true, these points are
  removed (as in removeOutliers(...) )

  For computing point normals we look for at least \nbrs neighbors
  within a sphere of radius \radius.
 */
void SmartScan::removeGrazingPoints(float threshold, bool removeOutliers, float radius, int nbrs)
{
	//this function removes all points whose normals point away from the scanner
	//for now we assume the scanner is at (0,0,0)
	//the threshold is in degrees, between point normal and direction to scanner
	//normals are computed based on point neighbors within radius 
	//if less then nbrs are available, the point is considered an outlier with ill-defined normal
	//depending on flag, outliers are deleted or kept

	// allocate memory as if we will keep all points, but later we'll do a copy and a delete
	std_msgs::Point3DFloat32 *newPoints = new std_msgs::Point3DFloat32[mNumPoints];
	int numNewPoints = 0;

	//we get the threshold in degrees
	threshold = fabs( threshold * M_PI / 180.0 );
	threshold = fabs( cos(M_PI / 2 - threshold) );

	for (int i=0; i<mNumPoints; i++) {
		std_msgs::Point3DFloat32 normal = computePointNormal(i,radius,nbrs);
		//check for outliers
		if ( norm(normal) < 0.5 && removeOutliers) continue;
	
		//compute direction to scanner
		std_msgs::Point3DFloat32 scanner(mNativePoints[i]);
		scanner = normalize(scanner);
		//we don't care about direction; will check dot product in absolute value
		float d = fabs( dot(normal, scanner) );
	
		if (d < threshold) continue; 
		//keep the point
		newPoints[numNewPoints] = mNativePoints[i];
		numNewPoints++;
	}

	//keep only the new points
	fprintf(stderr,"Removed grazing points from %d to %d\n",mNumPoints,numNewPoints);
	//this will copy the points again, so we are doing two copies in order to only
	//compute the nbrs once
	setPoints(numNewPoints,newPoints);
	//we can now delete this array which has worng size anyway
	delete [] newPoints;
	//and re-create vtk data (which does yet another copy...)
	createVtkData();
}


/*!  Computes the dominant plane in the scan by histograming point
  normals. First, all point normals are computed and
  histogramed. Then, the dominant normal direction is selected. Then,
  for all points that share this normal, the plane distance from the
  origin is histogramed.

  This approach is much less memory intensive than a traditional Hough
  transform as it performs a 2D histogram and a 1D histogram rather
  than a 3D histogram. However, it is also less accurate.

  \param planePoint \param planeNormal Output values, holding the plane found by the function

  For computing point normals we look for at least \nbrs neighbors
  within a sphere of radius \radius.
*/
void SmartScan::normalHistogramPlane(std_msgs::Point3DFloat32 &planePoint, std_msgs::Point3DFloat32 &planeNormal,
				     float radius, int nbrs)
{
	float binSize = 1.0 * M_PI / 180.0;

	float maxDist = 10.0;
	float distBinSize = 0.01;

	int d1 = ceil( (M_PI/2.0) / binSize);
	int d2 = ceil( 2.0 * M_PI / binSize);
	int dd = ceil( 2.0 * maxDist / distBinSize );

	int max,b1,b2,sb1,sb2,sbd; float theta, phi, dist;

	//prepare grid for normals
	Grid2D *grid2 = new Grid2D(d1,d2);
	//prepare grid for distances
	Grid1D *grid1 = new Grid1D(dd);

	//histogram normals
	for (int i=0; i<mNumPoints; i++) {
		std_msgs::Point3DFloat32 normal = computePointNormal(i,radius,nbrs);
		if ( norm(normal) < 0.5 ) continue;
		//we only care about a halfspace. This should also ensure theta <= M_PI/2
		if(normal.z < 0) {
			normal.x = -normal.x;
			normal.y = -normal.y;
			normal.z = -normal.z;
		}
		theta = acos(normal.z);
		phi = atan2( normal.y, normal.x );
		//fprintf(stderr,"theta %f and phi %f\n",theta, phi);
		if (phi < 0) phi += 2 * M_PI;

		b1 = floor(theta / binSize);
		b2 = floor(phi / binSize);

		//try to get around singularity problem with spherical coords
		if ( b1 == 0) b2 = 0;

		grid2->addGrid(b1,b2,&Grid2D::MASK);
	}

	//take largest value
	grid2->getMax(max, sb1, sb2);
	theta = sb1 * binSize;
	phi = sb2 * binSize;

	planeNormal.x = sin(theta) * cos(phi);
	planeNormal.y = sin(theta) * sin(phi);
	planeNormal.z = cos(theta);

	fprintf(stderr,"Normal theta %f and phi %f with %d votes\n",theta, phi, max);

	//histogram distances
	for(int i=0; i<mNumPoints; i++) {
		std_msgs::Point3DFloat32 normal = computePointNormal(i,radius,nbrs);
		if ( norm(normal) < 0.5 ) continue;
		double d = fabs( dot(normal, planeNormal) );
		if ( d < 0.986 ) continue; //10 degrees as threshold 
		//histogram the point
		if(normal.z < 0) {
			normal.x = -normal.x;
			normal.y = -normal.y;
			normal.z = -normal.z;
		}
		dist = dot(planeNormal, mNativePoints[i]);
		if (dist < -maxDist) dist = -maxDist;
		if (dist > maxDist) dist = maxDist;
		b1 = floor( (dist + maxDist)/distBinSize );
		grid1->addGrid(b1,&Grid1D::MASK);
	}

	//find plane
	grid1->getMax(max,sbd);
	dist = sbd * distBinSize - maxDist;

	planePoint.x = dist * planeNormal.x;
	planePoint.y = dist * planeNormal.y;
	planePoint.z = dist * planeNormal.z;

	fprintf(stderr,"Distance %f with %d votes\n",dist,max);

	delete grid2;
	delete grid1;
}

/*! Computes the dominant plane in the scan using RANSAC.

  \param planePoint \param planeNormal Output values, holding the plane found by the function

  \param iterations Number of RANSAC iterations to be performed

  \param distThresh Internal RANSAC threshold: point that are closer
  than this value to a hypothesis plane are considered inliers

 */
void SmartScan::ransacPlane(std_msgs::Point3DFloat32 &planePoint, std_msgs::Point3DFloat32 &planeNormal,
			    int iterations, float distThresh)
{
	int selPoints = 3;

	//if this is true, each plane is re-fit to all inliers after inliers are calulated
	//otehrwise, planes are just fit to the initial random chosen points.
	bool refitToConsensus = true;

	int it = 0, consensus, maxConsensus = 0;
	int i,k;
	float dist;

	NEWMAT::Matrix M(selPoints,3);
	std_msgs::Point3DFloat32 mean, consMean, normal, dif;
	std_msgs::Point3DFloat32 zero; zero.x = zero.y = zero.z = 0.0;
	std::list<std_msgs::Point3DFloat32> consList;

	//seed random generator
	srand( (unsigned)time(NULL) );

	while (1) {
		//randomly select selPoints from data
		//compute the centroid as well while we're at it
		mean = zero;
		for (i=0; i<selPoints; i++) {
			int index = floor( ((double)(mNumPoints-1)) * ( (double)rand() / RAND_MAX ) );
			assert (index >=0 && index < mNumPoints);
			mean.x += mNativePoints[index].x;
			mean.y += mNativePoints[index].y;
			mean.z += mNativePoints[index].z;
			M.element(i,0) = mNativePoints[index].x;
			M.element(i,1) = mNativePoints[index].y;
			M.element(i,2) = mNativePoints[index].z;
		}
		mean.x = mean.x / selPoints; mean.y = mean.y / selPoints; mean.z = mean.z / selPoints;
		for (i=0; i<selPoints; i++) {
			M.element(i,0) = M.element(i,0) - mean.x;
			M.element(i,1) = M.element(i,1) - mean.y;
			M.element(i,2) = M.element(i,2) - mean.z;
		}

		//fit a plane to the points
		normal = SVDPlaneNormal(&M, selPoints);

		//find all other consensus points
		consensus = 0;
		if (refitToConsensus) {
			consList.clear();
			consMean = zero;
		}
		for(int i=0; i<mNumPoints; i++) {
			dif.x = mNativePoints[i].x - mean.x;
			dif.y = mNativePoints[i].y - mean.y;
			dif.z = mNativePoints[i].z - mean.z;
			dist = dot(dif,normal);
			if ( fabs(dist) > distThresh ) continue;

			consensus++;
			if (refitToConsensus) {
				consList.push_back(mNativePoints[i]);
				consMean.x += mNativePoints[i].x;
				consMean.y += mNativePoints[i].y;
				consMean.z += mNativePoints[i].z;
			}
		}
		if (refitToConsensus) {
			consMean.x /= consensus;
			consMean.y /= consensus;
			consMean.z /= consensus;
		}

		//break for low number of consenting points. Has to be at least 3 if 
		//refitToConsensus is being used (otehrwise can not to SVD)
		if (consensus < 3) continue;

		//save solution if it is the best
		if (consensus >= maxConsensus) {
			assert(consensus >= 3);
			maxConsensus = consensus;
			if (refitToConsensus) {
				NEWMAT::Matrix *CM = new NEWMAT::Matrix(consensus, 3);
				std::list<std_msgs::Point3DFloat32>::iterator it;
				for (k=0, it = consList.begin(); it!=consList.end(); it++, k++) {
					CM->element(k,0) = (*it).x - consMean.x;
					CM->element(k,1) = (*it).y - consMean.y;
					CM->element(k,2) = (*it).z - consMean.z;
				}
				planeNormal = SVDPlaneNormal(CM,consensus);
				planePoint = consMean;
				delete CM;
			} else {
				planePoint = mean;
				planeNormal = normal;
			}
		}

		it++;
		if (it >= iterations) break;
	}
	consList.clear();
	fprintf(stderr,"RANSAC consensus: %d\n",maxConsensus);
}

/*!  Removes all points that are closer than \param thres to the plane
  defined by \param planePoint and \param planeNormal
 */
void SmartScan::removePlane(const std_msgs::Point3DFloat32 &planePoint, 
			    const std_msgs::Point3DFloat32 &planeNormal, float thresh)
{
	//remove points that are close to plane
	std_msgs::Point3DFloat32 *newPoints = new std_msgs::Point3DFloat32[mNumPoints];
	std_msgs::Point3DFloat32 dif;
	int numNewPoints = 0;
	float dist;
	for(int i=0; i<mNumPoints; i++) {
		dif.x = mNativePoints[i].x - planePoint.x;
		dif.y = mNativePoints[i].y - planePoint.y;
		dif.z = mNativePoints[i].z - planePoint.z;
		dist = dot(dif,planeNormal);
		if ( fabs(dist) < thresh ) continue;

		//keep this point
		newPoints[numNewPoints] = mNativePoints[i];
		numNewPoints++;
	}

	fprintf(stderr,"Kept %d out of %d points\n",numNewPoints,mNumPoints);
	setPoints(numNewPoints,newPoints);
	delete [] newPoints;
}

/*!Computes the normal of the point with index \param id by fitting a
   plane to neighboring points. It uses all neighbors within a sphere
   of radius \param radius. If less than \param nbrs such neighbors
   are found, the normal is considered unreliable and the function
   returns (0,0,0)

 */
std_msgs::Point3DFloat32 SmartScan::computePointNormal(int id, float radius, int nbrs)
{
	// radius - how large is the radius in which we look for nbrs for computing normal
	// nbrs - min number of nbrs we use for normal computation
	assert(id >=0 && id < mNumPoints);
	int nbrId;

	vtkIdList *result = vtkIdList::New();
	std_msgs::Point3DFloat32 zero; zero.x = zero.y = zero.z = 0.0;

	getVtkLocator()->FindPointsWithinRadius( radius,
						  mNativePoints[id].x,
						  mNativePoints[id].y,
						  mNativePoints[id].z,
						  result );
	//we don't have enough nbrs for a reliable normal
	int n = result->GetNumberOfIds();
	if ( n < nbrs ) {
		result->Delete();
		return zero;
	}

	//find the mean point for normalization
	std_msgs::Point3DFloat32 mean = zero;
	for (int i=0; i<n; i++) {
		nbrId = result->GetId(i);
		assert(nbrId >= 0 && nbrId < mNumPoints);
		mean.x += mNativePoints[nbrId].x;
		mean.y += mNativePoints[nbrId].y;
		mean.z += mNativePoints[nbrId].z;
	}
	mean.x = mean.x / n; mean.y = mean.y / n; mean.z = mean.z / n;
	//fprintf(stderr,"%f %f %f \n",mean.x, mean.y, mean.z);

	//create and populate matrix with normalized nbrs
	NEWMAT::Matrix M(n,3);
	for (int i=0; i<n; i++) {
		nbrId = result->GetId(i);
		assert(nbrId >= 0 && nbrId < mNumPoints);
		//we are assuming the points in the vtk data match the id's of the 
		//mNativePoints perfectly

		M.element(i,0) = mNativePoints[nbrId].x - mean.x;
		M.element(i,1) = mNativePoints[nbrId].y - mean.y;
		M.element(i,2) = mNativePoints[nbrId].z - mean.z;
	}
	result->Delete();

	return SVDPlaneNormal(&M,n);
}

/*! Given an \param n by 3 matrix \param M in NEWMAT format, performs
  SVD and returns the direction of the least significant singular
  vector. If the matrix is a list of vertices, this corresponds to the
  direction of the normal of a plane fit to those points. Vertices are
  expected to be normalized first (of mean 0).
*/
std_msgs::Point3DFloat32 SmartScan::SVDPlaneNormal(NEWMAT::Matrix *M, int n)
{
	NEWMAT::Matrix U(n,3);
	NEWMAT::DiagonalMatrix D(3);
	NEWMAT::Matrix V(3,3);
	SVD(*M, D, U, V);

	std_msgs::Point3DFloat32 normal;
	normal.x = V.element(0,2);
	normal.y = V.element(1,2);
	normal.z = V.element(2,2);
	//std::cout << *M << std::endl;
	//std::cout << U << std::endl;
	//std::cout << D << std::endl;
	//std::cout << V << std::endl;
	//fprintf(stderr,"%f %f %f \n",normal.x, normal.y, normal.z);
	return normal;
}

