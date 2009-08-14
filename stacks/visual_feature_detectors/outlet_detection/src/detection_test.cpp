/*
 *  main.cpp
 *  outlet_model
 *
 *  Created by Victor  Eruhimov on 1/16/09.
 *  Copyright 2009 Argus Corp. All rights reserved.
 *
 */

//*****************************************************************************************
// Warning: this is research code with poor architecture, performance and no documentation!
//*****************************************************************************************

#include <stdio.h>
#include <sys/stat.h>
#include <vector>

#include <cv.h>
#include <highgui.h>

#include "outlet_detection/outlet_detector_test.h"


int _LoadCameraParams(char* filename, CvMat** intrinsic_matrix, CvMat** distortion_coeffs)
{

	CvFileStorage* fs = cvOpenFileStorage( filename, 0, CV_STORAGE_READ );
	if (fs==NULL) return 0;
	
    *intrinsic_matrix = (CvMat*)cvReadByName( fs,0,"camera_matrix");
	*distortion_coeffs = (CvMat*)cvReadByName( fs,0,"distortion_coefficients");
    
    cvReleaseFileStorage(&fs);
	
	return 1;
}

int main(int argc,char** argv)
{    


	char mode[1024], config_filename[1024], camera_filename[1024], output_path[1024], 
        test_path[1024], train_config[1024], pca_config[1024], output_test_config[1024];
	int accuracy = 5;
	
    
	if(argc != 7 && argc != 8)
	{
		printf("Usage: detection_test <mode[test|modify|generate]> <images_path> <test_config_filename> <camera_config> <output_path> <accuracy|output_test_config_filename|output_test_config_filename>\n");
        printf("Usage: detection_test <mode[test|modify|generate]> <images_path> <test_config_filename> <camera_config> <train_config_path> <output_path> <accuracy|output_test_config_filename|output_test_config_filename>\n");
		return(0);
	}
    
	
	strcpy(mode, argv[1]);
	strcpy(test_path, argv[2]);
	strcpy(config_filename, argv[3]);
	strcpy(camera_filename, argv[4]);
    
    
    outlet_template_t outlet_template;
    if(argc == 7)
    {
        strcpy(output_path, argv[5]);
    }
    else
    {
        strcpy(train_config, argv[5]);
        strcpy(output_path, argv[6]);
    }

	if ((strcmp(mode,"test")==0) )
	{
		sscanf(argv[argc-1],"%d",&accuracy);
	}
	else
	{
		strcpy(output_test_config,argv[argc-1]);
	}
	
		// reading camera params
	CvMat* intrinsic_matrix = 0;
	CvMat* distortion_params = 0; 
	_LoadCameraParams(camera_filename, &intrinsic_matrix, &distortion_params);

	vector<outlet_test_elem> test_data;

	if (readTestFile(config_filename,test_path,test_data) > 0)
	{
		if (strcmp(mode,"modify")==0)
		{
			//Code for modify mode
			printf("Use <Space> and <Backspace> to get next and previous image\n");
			printf("Press <Enter> key to enter modification mode\n");
			printf("Press <Esc> key to approve outlet positions and start the test\n");
			char window_name[100];
			strcpy(window_name,"Image");

			int key;
			int i = 0;
			bool isEnd = false;
			IplImage* img = getRealOutletImage(test_data[0],intrinsic_matrix, distortion_params);
			cvNamedWindow(window_name);
			cvShowImage(window_name,img);
			while (!isEnd)
			{
				key = cvWaitKey();
				switch(key)
				{
					case 13:
						cvReleaseImage(&img);
						cvDestroyWindow(window_name);
						setRealOutlet(test_data[i],intrinsic_matrix, distortion_params);
						img = getRealOutletImage(test_data[i],intrinsic_matrix, distortion_params);
						cvNamedWindow(window_name);
						cvShowImage(window_name,img);
						break;
					case 27: 
						cvReleaseImage(&img);
						cvDestroyWindow(window_name);
						isEnd = true;
						break;
					case 32://SPACE
						if (i<((size_t)test_data.size()-1))
						{
							cvReleaseImage(&img);
							img = getRealOutletImage(test_data[++i],intrinsic_matrix, distortion_params);
							cvNamedWindow(window_name);
							cvShowImage(window_name,img);
						}
						else
						{
							printf("The last image. Unable to get the next one\n");
						}
						break;
					case 8://Backspace
						if (i>0)
						{
							cvReleaseImage(&img);
							img = getRealOutletImage(test_data[--i],intrinsic_matrix, distortion_params);
							cvNamedWindow(window_name);
							cvShowImage(window_name,img);
						}
						else
						{
							printf("The first image. Unable to get the previous one\n");
						}
						break;
				}
			}
		}
    
	    
	#if defined(_VERBOSE)
		char pathname[1024];
		sprintf(pathname, "mkdir %s", output_path);
		system(pathname);
	    
	#if !defined(_GHT)
			sprintf(pathname, "mkdir %s/output_filt", output_path);
			system(pathname);
	        
			sprintf(pathname, "mkdir %s/output", output_path);
			system(pathname);
	        
			sprintf(pathname, "mkdir %s/keyout", output_path);
			system(pathname);
	        
			sprintf(pathname, "mkdir %s/holes", output_path);
			system(pathname);
	        
			sprintf(pathname, "mkdir %s/warped", output_path);
			system(pathname);
	#else
#if defined(_L_DETECTOR)
			sprintf(pathname, "mkdir %s/correspondence", output_path);
			system(pathname);

			sprintf(pathname, "mkdir %s/outlets", output_path);
			system(pathname);
	        
			sprintf(pathname, "mkdir %s/features", output_path);
			system(pathname);

#else
			sprintf(pathname, "mkdir %s/output_filt", output_path);
			system(pathname);

			sprintf(pathname, "mkdir %s/outlets", output_path);
			system(pathname);
	        
			sprintf(pathname, "mkdir %s/features", output_path);
			system(pathname);
	        
			sprintf(pathname, "mkdir %s/features_filtered", output_path);
			system(pathname);
#endif // _L_DETECTOR
	#endif
	#endif //_VERBOSE
			
#if !defined(_L_DETECTOR)	
		if ((argc !=7) && (strcmp(mode,"modify")!=0))
		{

			outlet_template.load(train_config);
		}
#endif //L

		//Running the test
		if (strcmp(mode,"modify")==0)
		{
			writeTestFile(output_test_config,test_data);
			printf("New test config was successfully generated into %s\n",output_test_config);
		}
		else
#if defined(_L_DETECTOR)
			//runLOutletDetectorTest(intrinsic_matrix, distortion_params, train_config, test_data, output_path);
			runFernsLOutletDetectorTest(intrinsic_matrix, distortion_params, train_config, test_data, output_path);
#else
	#if defined(_FERNS_ONEWAY_DETECTOR)
			runFernsOneWayOutletDetectorTest(intrinsic_matrix, distortion_params,  outlet_template, train_config, test_data, output_path);
			//runFernsOutletDetectorTest(intrinsic_matrix, distortion_params, train_config, test_data, output_path);
	#else
			runOutletDetectorTest(intrinsic_matrix, distortion_params, outlet_template, test_data, output_path);
	#endif //FERNS
#endif //L

		if (strcmp(mode,"generate")==0)
		{
			convertTestToReal(test_data);
			writeTestFile(output_test_config,test_data);
			printf("New test config was successfully generated into %s\n",output_test_config);
		}

		if (strcmp(mode,"test")==0)
		{

			compareAllOutlets(test_data,accuracy);
			char filename[1024];
			if (output_path)
				sprintf(filename,"%s/results.txt",output_path);
			else
				sprintf(filename,"results.txt",output_path);
			writeTestResults(filename,test_data);
			printf ("Test results were successfully written into %s\n", filename);
		}
	    
		  
		cvReleaseMat(&intrinsic_matrix);
		cvReleaseMat(&distortion_params);
	        
	}
	else
	{
		printf("Unable to read test configuration file\n");
	}
		
	return 0;
}


