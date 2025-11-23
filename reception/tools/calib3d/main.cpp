#include "settings.h"

//https://github.com/opencv/opencv/blob/4.x/samples/cpp/tutorial_code/calib3d/camera_calibration/camera_calibration.cpp
//https://github.com/JayaoLiu/eye_to_hand_calibrate/blob/main/eye_to_hand_calibrate.py
//https://github.com/RealManRobot/hand_eye_calibration
//https://www.sensemoment.com/fenxiang/84.html
//https://github.com/zivid/zivid-ros
//https://stackoverflow.com/questions/78676561/hand-to-eye-calibration
//https://forum.opencv.org/t/camera-calibration-and-hand-eye-calibration-together/12762/3
//https://github.com/opencv/opencv/blob/b5a9a6793b3622b01fe6c7b025f85074fec99491/modules/calib3d/test/test_calibration_hand_eye.cpp#L145
//https://forum.opencv.org/t/eye-to-hand-calibration/5690/15
//https://docs.opencv.org/4.x/d9/d0c/group__calib3d.html#gaebfc1c9f7434196a374c382abf43439b
//https://support.zivid.com/en/v2.4/academy/applications/hand-eye/hand-eye-calibration-process.html
//https://github.com/opencv/opencv/blob/4.x/modules/calib3d/src/calibration_handeye.cpp
//https://www.zivid.com/hubfs/softwarefiles/releases/2.4.2+1a2e8cfb-1/doc/cpp/HandEye_8h_source.html
//https://docs.opencv.org/4.x/d9/d0c/group__calib3d.html
//https://github.com/cvg25/hand-eye_calibration
//https://github.com/ethz-asl/hand_eye_calibration
//https://github.com/IFL-CAMP/easy_handeye/blob/master/easy_handeye/CMakeLists.txt
//https://xgyopen.github.io/2018/08/16/2018-08-16-imv-calibration-eye-hand/


bool runCalibrationAndSave(Settings& s, Size imageSize, Mat&  cameraMatrix, Mat& distCoeffs,
                           vector<vector<Point2f> > imagePoints, float grid_width, bool release_object);

int main(int argc, char* argv[])
{

    const String keys
            = "{help h usage ? |                                                              | print this message            }"
              "{@settings      | /home/gabriel_wang/work/perception/tools/calib3d/in_VID5.xml | input setting file            }"
              "{d              |                                                              | actual distance between top-left and top-right corners of the calibration grid }"
              "{winSize        | 11                                                           | Half of search window for cornerSubPix }";
    CommandLineParser parser(argc, argv, keys);
    parser.about("This is a camera calibration .\n"
                 "Usage: camera_calibration [configuration_file -- default ./in_VID5.xml]\n"
                 "Near the sample file you'll find the configuration file, which has detailed help of "
                 "how to edit it. It may be any OpenCV supported file format XML/YAML.");
    if (!parser.check()) {
        parser.printErrors();
        return 0;
    }

    if (parser.has("help")) {
        parser.printMessage();
        return 0;
    }

    //! [file_read]
    Settings s;
    const string inputSettingsFile = parser.get<string>(0);
    FileStorage fs(inputSettingsFile, FileStorage::READ); // Read the settings
    if (!fs.isOpened())
    {
        cout << "Could not open the configuration file: \"" << inputSettingsFile << "\"" << endl;
        parser.printMessage();
        return -1;
    }
    fs["Settings"] >> s;
    fs.release();                                         // close Settings file
    //! [file_read]

    int winSize = parser.get<int>("winSize");

    float grid_width = s.squareSize * (s.boardSize.width - 1);

    bool release_object = false;
    if (parser.has("d")) {
        grid_width = parser.get<float>("d");
        release_object = true;
    }

    vector<vector<Point2f> > imagePoints;
    Mat cameraMatrix, distCoeffs;
    Size imageSize;
    int mode = CAPTURING;
    clock_t prevTimestamp = 0;
    const Scalar RED(0,0,255), GREEN(0,255,0);
    const char ESC_KEY = 27;
    //! [get_input]

    for(;;)
    {
        Mat view = s.nextImage();
        //-----  If got enough, then stop calibration and show result -------------
        if( imagePoints.size() >= (size_t)s.nrFrames )
        {
            bool result = runCalibrationAndSave(s, imageSize,  cameraMatrix, distCoeffs, imagePoints, grid_width, release_object);
            mode  = result ? CALIBRATED : DETECTION;
        }
        if(view.empty())          // If there are no more images stop the loop
        {
            // if calibration threshold was not reached yet, calibrate now
            if( mode != CALIBRATED && !imagePoints.empty() )
                runCalibrationAndSave(s, imageSize,  cameraMatrix, distCoeffs, imagePoints, grid_width, release_object);
            std::cout <<"no more images to be used for calibrated." << std::endl;
            break;
        }
        //! [get_input]

        imageSize = view.size();  // Format input image.
        if( s.flipVertical )    flip( view, view, 0 );

        vector<Point2f> pointBuf;
        int chessBoardFlags = CALIB_CB_ADAPTIVE_THRESH | CALIB_CB_NORMALIZE_IMAGE | CALIB_CB_FAST_CHECK;

        bool found = findChessboardCorners( view, s.boardSize, pointBuf, chessBoardFlags);
        //! [pattern_found]
        if (found)                // If done with success,
        {
            // improve the found corners' coordinate accuracy for chessboard
            Mat viewGray;
            cvtColor(view, viewGray, COLOR_BGR2GRAY);
            cornerSubPix( viewGray, pointBuf, Size(winSize,winSize), Size(-1,-1), TermCriteria( TermCriteria::EPS+TermCriteria::COUNT, 30, 0.0001 ));
            if( mode == CAPTURING ) {
                imagePoints.push_back(pointBuf);
            }
            // Draw the corners.
            drawChessboardCorners( view, s.boardSize, Mat(pointBuf), found );
        }
        //! [pattern_found]

        //----------------------------- Output Text ------------------------------------------------
        //! [output_text]
        string msg = (mode == CAPTURING) ? "100/100" :
                      mode == CALIBRATED ? "Calibrated" : "Press 'g' to start";
        int baseLine = 0;
        Size textSize = getTextSize(msg, 1, 1, 1, &baseLine);
        Point textOrigin(view.cols - 2*textSize.width - 10, view.rows - 2*baseLine - 10);

        if( mode == CAPTURING )
        {
            if(s.showUndistorted)
                msg = cv::format( "%d/%d Undist", (int)imagePoints.size(), s.nrFrames );
            else
                msg = cv::format( "%d/%d", (int)imagePoints.size(), s.nrFrames );
        }

        putText( view, msg, textOrigin, 1, 1, mode == CALIBRATED ?  GREEN : RED);

        //! [output_text]
        //------------------------- Video capture  output  undistorted ------------------------------
        //! [output_undistorted]
        if( mode == CALIBRATED && s.showUndistorted )
        {
            Mat temp = view.clone();
            undistort(temp, view, cameraMatrix, distCoeffs);
        }
        //! [output_undistorted]
#if 0
        //------------------------------ Show image and check for input commands -------------------
        imshow("Image View", view);
        char key = (char)waitKey(s.inputCapture.isOpened() ? 50 : s.delay);

        if( key  == ESC_KEY )
            break;

        if( key == 'u' && mode == CALIBRATED )
            s.showUndistorted = !s.showUndistorted;
#endif
    }

    // -----------------------Show the undistorted image for the image list ------------------------
    if( s.showUndistorted && !cameraMatrix.empty())
    {
        Mat view, rview, map1, map2;
        initUndistortRectifyMap( cameraMatrix, distCoeffs, Mat(),
                                 getOptimalNewCameraMatrix(cameraMatrix, distCoeffs, imageSize, 1, imageSize, 0), imageSize,
                                 CV_16SC2, map1, map2);

        for(size_t i = 0; i < s.imageList.size(); i++ )
        {
            view = imread(s.imageList[i], IMREAD_COLOR);
            if(view.empty())
                continue;
            remap(view, rview, map1, map2, INTER_LINEAR);

//            imshow("Image View", rview);
//            char c = (char)waitKey();
//            if( c  == ESC_KEY || c == 'q' || c == 'Q' )
//                break;
        }
    }

    return 0;
}

static double computeReprojectionErrors( const vector<vector<Point3f> >& objectPoints,
                                         const vector<vector<Point2f> >& imagePoints,
                                         const vector<Mat>& rvecs, const vector<Mat>& tvecs,
                                         const Mat& cameraMatrix , const Mat& distCoeffs,
                                         vector<float>& perViewErrors)
{
    vector<Point2f> imagePoints2;
    size_t totalPoints = 0;
    double totalErr = 0, err;
    perViewErrors.resize(objectPoints.size());

    for(size_t i = 0; i < objectPoints.size(); ++i )
    {
        projectPoints(objectPoints[i], rvecs[i], tvecs[i], cameraMatrix, distCoeffs, imagePoints2);
        err = norm(imagePoints[i], imagePoints2, NORM_L2);

        size_t n = objectPoints[i].size();
        perViewErrors[i] = (float) std::sqrt(err*err/n);
        totalErr        += err*err;
        totalPoints     += n;
    }

    return std::sqrt(totalErr/totalPoints);
}
static void calcBoardCornerPositions(Size boardSize, float squareSize, vector<Point3f>& corners)
{
    corners.clear();
    for (int i = 0; i < boardSize.height; ++i) {
        for (int j = 0; j < boardSize.width; ++j) {
            corners.push_back( Point3f ( j*squareSize, i*squareSize, 0 ) );
        }
    }
}
static bool runCalibration( Settings& s, Size& imageSize, Mat& cameraMatrix, Mat& distCoeffs,
                            vector<vector<Point2f> > imagePoints, vector<Mat>& rvecs, vector<Mat>& tvecs,
                            vector<float>& reprojErrs,  double& totalAvgErr, vector<Point3f>& newObjPoints,
                            float grid_width, bool release_object)
{
    //! [fixed_aspect]
    cameraMatrix = Mat::eye(3, 3, CV_64F);
    if( s.flag & CALIB_FIX_ASPECT_RATIO ){
        cameraMatrix.at<double>(0,0) = s.aspectRatio;
    }

    distCoeffs = Mat::zeros(8, 1, CV_64F);

    vector<vector<Point3f> > objectPoints(1);
    calcBoardCornerPositions(s.boardSize, s.squareSize, objectPoints[0]);
    objectPoints[0][s.boardSize.width - 1].x = objectPoints[0][0].x + grid_width;

    newObjPoints = objectPoints[0];
    objectPoints.resize(imagePoints.size(),objectPoints[0]);

    //Find intrinsic and extrinsic camera parameters
    double rms;
    int iFixedPoint = -1;
    if ( release_object )

    iFixedPoint = s.boardSize.width - 1;
    rms = calibrateCameraRO(objectPoints, imagePoints, imageSize, iFixedPoint,
                            cameraMatrix, distCoeffs, rvecs, tvecs, newObjPoints,
                            s.flag | CALIB_USE_LU);

    if (release_object) {
        cout << "New board corners: " << endl;
        cout << newObjPoints[0] << endl;
        cout << newObjPoints[s.boardSize.width - 1] << endl;
        cout << newObjPoints[s.boardSize.width * (s.boardSize.height - 1)] << endl;
        cout << newObjPoints.back() << endl;
    }

    cout << "Re-projection error reported by calibrateCamera: "<< rms << endl;
    bool ok = checkRange(cameraMatrix) && checkRange(distCoeffs);

    objectPoints.clear();
    objectPoints.resize(imagePoints.size(), newObjPoints);
    totalAvgErr = computeReprojectionErrors(objectPoints, imagePoints, rvecs, tvecs, cameraMatrix,distCoeffs, reprojErrs);
    return ok;
}

static void saveCameraParams( Settings& s, Size& imageSize, Mat& cameraMatrix, Mat& distCoeffs,
                              const vector<Mat>& rvecs, const vector<Mat>& tvecs,
                              const vector<float>& reprojErrs, const vector<vector<Point2f> >& imagePoints,
                              double totalAvgErr, const vector<Point3f>& newObjPoints )
{
    FileStorage fs( s.outputFileName, FileStorage::WRITE );

    time_t tm;
    time( &tm );
    struct tm *t2 = localtime( &tm );
    char buf[1024];
    strftime( buf, sizeof(buf), "%c", t2 );

    fs << "calibration_time" << buf;

    if( !rvecs.empty() || !reprojErrs.empty() )
    {
        fs << "nr_of_frames" << (int)std::max(rvecs.size(), reprojErrs.size());
    }

    fs << "image_width"  << imageSize.width;
    fs << "image_height" << imageSize.height;
    fs << "board_width"  << s.boardSize.width;
    fs << "board_height" << s.boardSize.height;
    fs << "square_size"  << s.squareSize;
    fs << "marker_size"  << s.markerSize;

    if( s.flag & CALIB_FIX_ASPECT_RATIO )
        fs << "fix_aspect_ratio" << s.aspectRatio;

    if (s.flag)
    {
        std::stringstream flagsStringStream;

        flagsStringStream << "flags:"
                          << (s.flag & CALIB_USE_INTRINSIC_GUESS ? " +use_intrinsic_guess" : "")
                          << (s.flag & CALIB_FIX_ASPECT_RATIO ? " +fix_aspectRatio" : "")
                          << (s.flag & CALIB_FIX_PRINCIPAL_POINT ? " +fix_principal_point" : "")
                          << (s.flag & CALIB_ZERO_TANGENT_DIST ? " +zero_tangent_dist" : "")
                          << (s.flag & CALIB_FIX_K1 ? " +fix_k1" : "")
                          << (s.flag & CALIB_FIX_K2 ? " +fix_k2" : "")
                          << (s.flag & CALIB_FIX_K3 ? " +fix_k3" : "")
                          << (s.flag & CALIB_FIX_K4 ? " +fix_k4" : "")
                          << (s.flag & CALIB_FIX_K5 ? " +fix_k5" : "");

        fs.writeComment(flagsStringStream.str());
    }

    fs << "flags" << s.flag;
    fs << "camera_matrix" << cameraMatrix;
    fs << "distortion_coefficients" << distCoeffs;

    fs << "avg_reprojection_error" << totalAvgErr;
    if (s.writeExtrinsics && !reprojErrs.empty()) {
        fs << "per_view_reprojection_errors" << Mat(reprojErrs);
    }

    if(s.writeExtrinsics && !rvecs.empty() && !tvecs.empty() )
    {
        CV_Assert(rvecs[0].type() == tvecs[0].type());
        Mat bigmat((int)rvecs.size(), 6, CV_MAKETYPE(rvecs[0].type(), 1));
        bool needReshapeR = rvecs[0].depth() != 1 ? true : false;
        bool needReshapeT = tvecs[0].depth() != 1 ? true : false;

        for( size_t i = 0; i < rvecs.size(); i++ )
        {
            Mat r = bigmat(Range(int(i), int(i+1)), Range(0,3));
            Mat t = bigmat(Range(int(i), int(i+1)), Range(3,6));

            if(needReshapeR)
            {
                rvecs[i].reshape(1, 1).copyTo(r);
            }
            else
            {
                //*.t() is MatExpr (not Mat) so we can use assignment operator
                CV_Assert(rvecs[i].rows == 3 && rvecs[i].cols == 1);
                r = rvecs[i].t();
            }

            if(needReshapeT)
                tvecs[i].reshape(1, 1).copyTo(t);
            else
            {
                CV_Assert(tvecs[i].rows == 3 && tvecs[i].cols == 1);
                t = tvecs[i].t();
            }
        }
        fs.writeComment("a set of 6-tuples (rotation vector + translation vector) for each view");
        fs << "extrinsic_parameters" << bigmat;
    }

    if(s.writePoints && !imagePoints.empty() )
    {
        Mat imagePtMat((int)imagePoints.size(), (int)imagePoints[0].size(), CV_32FC2);
        for( size_t i = 0; i < imagePoints.size(); i++ )
        {
            Mat r = imagePtMat.row(int(i)).reshape(2, imagePtMat.cols);
            Mat imgpti(imagePoints[i]);
            imgpti.copyTo(r);
        }
        fs << "image_points" << imagePtMat;
    }

    if( s.writeGrid && !newObjPoints.empty() )
    {
        fs << "grid_points" << newObjPoints;
    }
}

bool runCalibrationAndSave(Settings& s, Size imageSize, Mat& cameraMatrix, Mat& distCoeffs,
                           vector<vector<Point2f> > imagePoints, float grid_width, bool release_object)
{
    vector<Mat> rvecs, tvecs;
    vector<float> reprojErrs;
    double totalAvgErr = 0;
    vector<Point3f> newObjPoints;

    bool ok = runCalibration(s, imageSize, cameraMatrix, distCoeffs, imagePoints, rvecs, tvecs, reprojErrs,
                             totalAvgErr, newObjPoints, grid_width, release_object);
    cout << (ok ? "Calibration succeeded" : "Calibration failed")
         << ". avg re projection error = " << totalAvgErr << endl;

    if (ok)
        saveCameraParams(s, imageSize, cameraMatrix, distCoeffs, rvecs, tvecs, reprojErrs, imagePoints, totalAvgErr, newObjPoints);
    return ok;
}
