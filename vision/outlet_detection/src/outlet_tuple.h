/*
 *  outlet_tuple.h
 *  rectify_outlets
 *
 *  Created by Victor  Eruhimov on 1/25/09.
 *  Copyright 2009 Argus Corp. All rights reserved.
 *
 */

//*****************************************************************************************
// Warning: this is research code with poor architecture, performance and no documentation!
//*****************************************************************************************

#if !defined(_OUTLET_TUPLE_H)
#define _OUTLET_TUPLE_H

#include <cv.h>

inline CvPoint cvPoint(CvPoint2D32f point)
{
	return cvPoint(int(point.x), int(point.y));
}

inline CvPoint rect_center(CvRect rect)
{
	return cvPoint(rect.x + rect.width/2, rect.y + rect.height/2);
}

inline CvPoint2D32f operator +(CvPoint2D32f p1, CvPoint2D32f p2)
{
	return cvPoint2D32f(p1.x + p2.x, p1.y + p2.y);
}

inline CvPoint2D32f operator -(CvPoint2D32f p1, CvPoint2D32f p2)
{
	return cvPoint2D32f(p1.x - p2.x, p1.y - p2.y);
}

inline CvPoint3D32f operator -(CvPoint3D32f p1, CvPoint3D32f p2)
{
	return cvPoint3D32f(p1.x - p2.x, p1.y - p2.y, p1.z - p2.z);
}

inline float length(const CvPoint2D32f& p)
{
	return sqrt(p.x*p.x + p.y*p.y);
}

inline float length(const CvPoint3D32f& p)
{
	return sqrt(p.x*p.x + p.y*p.y + p.z*p.z);
}

typedef struct
{
	CvPoint2D32f centers[4];
} outlet_tuple_t;

typedef struct 
{
	CvPoint2D32f center;
	float angle;
	int idx;
	CvSeq* seq;
} outlet_elem_t;


CvPoint2D32f calc_center(CvSeq* seq);
int find_dir(const CvPoint2D32f* dir, int xsign, int ysign);
int order_tuple(CvPoint2D32f* centers);
int order_tuple2(vector<outlet_elem_t>& tuple);

int find_outlet_centroids(IplImage* img, CvPoint2D32f* centers, IplImage* outlet_mask, const char* output_path, const char* filename);
int find_tuple(vector<outlet_elem_t>& candidates, CvPoint2D32f* centers);

void calc_outlet_homography(const CvPoint2D32f* centers, CvMat* map_matrix, CvMat* inverse_map_matrix = 0);
//void calc_image_homography(const CvPoint2D32f* centers, CvSize src_size, CvMat** xmap, CvMat** ymap, CvSize* dst_size);
void calc_image_homography(const CvPoint2D32f* centers, CvSize src_size, CvMat* map_matrix, CvSize* dst_size);
int calc_image_homography(IplImage* src, CvMat* map_matrix, CvSize* dst_size, CvPoint2D32f* hor_dir = 0, 
						  CvPoint3D32f* origin = 0, CvPoint2D32f* scale = 0, const char* output_path = 0, 
						  const char* filename = 0, CvPoint2D32f* _centers = 0);
IplImage* find_templates(IplImage* img, IplImage* templ);

void calc_camera_pose(CvMat* intrinsic_mat, CvMat* distortion_coeffs, CvPoint2D32f* centers, 
					  CvMat* rotation_vector, CvMat* translation_vector);

#endif //_OUTLET_TUPLE_H