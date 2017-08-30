#include "visp-plugin-pose-estimation.h"
extern "C" {

	vpImage<unsigned char> image;
	vpDot2 blob;
	vpImagePoint germ;
	vpImagePoint cog;
	vector<vpPoint> point;
  vpHomogeneousMatrix cMo;
	list<vpDot2> blob_list;
	vpCameraParameters cam;
  vpPose pose;

	// Greyscale bitmap from Unity to vpImage
	void passFrame(unsigned char* const bitmap, int height, int width){
		// Resize frame according to Webcam input from Unity
		image.resize(height,width);

		// Grey Scale Image to be passed in the tracker pipeline
		image.bitmap = bitmap;
	}

	void initBlobTracker(double getMouseX, double getMouseY, unsigned int* const init_done)
	{
		// Define Blob initial tracking pixel as a vpImagePoint
		germ.set_ij(getMouseX, getMouseY);

		//Initialize blob pixels
		blob.initTracking(image, germ);
		init_done[0] = 1;
	}

	void trackBlob()
	{
		blob.track(image);
	}

	void getBlobCoordinates(double* cogX, double* cogY, unsigned int* const init_done)
	{
		try {
			blob.track(image);
			// Get the Center of Gravity of the tracked blob
			vpImagePoint cog = blob.getCog();
			cogX[0] = cog.get_i();
			cogY[0] = cog.get_j();
		}
		catch(...) {
			*init_done = 0;
		}
	}

	void initFourBlobTracker(unsigned int* const init_pose)
	{
		if (0) {
			// code used to learn the characteristics of a blob that we want to retrieve automatically
      // Learn the characteristics of the blob to auto detect
      blob.initTracking(image);
      blob.track(image);
    }

		// Set blob characteristics for the auto detection
		blob.setWidth(40);
    blob.setHeight(40);
    blob.setArea(1000);
    blob.setGrayLevelMin(0);
    blob.setGrayLevelMax(150);
    blob.setSizePrecision(0.65);
    blob.setEllipsoidShapePrecision(0.65);

		// Define the 3D model of a target defined by 4 blobs arranged as a square
		point.push_back( vpPoint(-0.06, -0.06, 0) );
    point.push_back( vpPoint( 0.06, -0.06, 0) );
    point.push_back( vpPoint( 0.06,  0.06, 0) );
    point.push_back( vpPoint(-0.06,  0.06, 0) );

		cam.initPersProjWithoutDistortion(840, 840, image.getWidth()/2, image.getHeight()/2);
		init_pose[0] = 1;
	}

	void getNumberOfBlobs(unsigned int* const numOfBlobs)
	{
		blob.searchDotsInArea(image, 0, 0, image.getWidth(), image.getHeight(), blob_list);

		// Make a seprate track function that takes a list of blobs into consideration.
		for(std::list<vpDot2>::iterator it=blob_list.begin(); it != blob_list.end(); ++it) {
			(*it).track(image);
		}
		numOfBlobs[0] = blob_list.size();
	}

	void estimatePose(unsigned int* const init_pose, double* cMo_pass)
	{
	  double x=0, y=0;
	  unsigned int i = 0;
	  for (std::list<vpDot2>::const_iterator it=blob_list.begin(); it != blob_list.end(); ++it) {
	    vpPixelMeterConversion::convertPoint(cam, (*it).getCog(), x, y);
	    point[i].set_x(x);
	    point[i].set_y(y);
	    pose.addPoint(point[i]);
	    i++;
	  }

	  if (init_pose[0] == 1) {
	    vpHomogeneousMatrix cMo_dem;
	    vpHomogeneousMatrix cMo_lag;
	    pose.computePose(vpPose::DEMENTHON, cMo_dem);
	    pose.computePose(vpPose::LAGRANGE, cMo_lag);
	    double residual_dem = pose.computeResidual(cMo_dem);
	    double residual_lag = pose.computeResidual(cMo_lag);
	    if (residual_dem < residual_lag)
	      cMo = cMo_dem;
	    else
	      cMo = cMo_lag;
	  }
	  pose.computePose(vpPose::VIRTUAL_VS, cMo);

		vpImagePoint temp;
		vpImagePoint temp1;

		// Converting pose coordinates from meter to pixels
		vpMeterPixelConversion::convertPoint(cam, cMo[0][3], cMo[1][3], temp);
		vpMeterPixelConversion::convertPoint(cam, cMo[0][3], cMo[2][3], temp1);

		// Passing the cMo values via cMo_pass
		for(int i = 0; i < 3; i++)
		{
			for(int j = 0; j < 4; j++)
			{
				cMo_pass[4*i+j] = cMo[i][j];
			}
		}

		// X - coordinate in pixel unit
		cMo_pass[3] = temp.get_i();
		// Y - coordinate in pixel unit
		cMo_pass[7] = temp.get_j();
		// Z - coordinate in pixel unit
		cMo_pass[11] = temp1.get_i();
	}
}