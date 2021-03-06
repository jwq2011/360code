#include <stdlib.h>
#include "topcamerawindows.h"
#include "QDebug"
#include <QTimer>
#include <QVector>
#include <QPainter>
#include <QStringList>
#include "opencv2/opencv.hpp"
#include "opencv2/highgui.hpp"
#include <QFile>
#include <fstream>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

//读v4l2的数据
#include "4_in_read.h"
//g2d
#include "g2d.h"
#include "g2d_yuyv_bgra.h"

#include "loopuptable.h"


using namespace std;
using namespace cv;
//#define  MAXHW  720
//#define  SCREEM_H  720
//#define  SCREEM_W  420

//#define  LEFT_UPx   120
//#define  LEFT_UPy   165

//#define  RIGHT_UPx   300
//#define  RIGHT_UPY   165

//#define  LEFT_DOWNx   120
//#define  LEFT_DOWNy   555

//#define  RIGHT_DOWNx   300
//#define  RIGHT_DOWNY   555
//float K1 = (float)LEFT_UPy/LEFT_UPx;
//float K2 = (float)RIGHT_UPY/(RIGHT_UPx - SCREEM_W);


static const int bufferCount = 1;
static struct g2d_buf *g_g2d_src_buffer[bufferCount];
static struct g2d_buf *g_g2d_dst_buffer[bufferCount];

static ICONWIDGET * lefttopwidget = new ICONWIDGET;
static ICONWIDGET * leftbottomwidget =new ICONWIDGET;
static ICONWIDGET * righttopwidget =new ICONWIDGET;
static ICONWIDGET * rightbottomwidget =new ICONWIDGET;

TopCameraWindows::TopCameraWindows(QWidget *parent, Qt::WindowFlags f)
    :QWidget(parent,f)
{

//    this->hide();

//    MscalFig = Mat::zeros(800,1280,CV_8UC4);

    std::string filename = "/data/user/caliberation_result/top/result.xml";
    cv::FileStorage file_in(filename, cv::FileStorage::READ);

    cv::Mat tmp1;
    file_in["intrinsic_matrix"] >>tmp1;
    cv::Mat  intrinsic_matrix(tmp1);
    this->intrinsic_matrix= intrinsic_matrix;

    cv::Mat tmp2;
    file_in["distortion_coeffs"] >> tmp2;
    cv::Mat distortion_coeffs(tmp2);
    this->distortion_coeffs = distortion_coeffs;

    file_in.release();

    firstview = false;
    initWarpPerspective = true;

    srcWidth = 1280;
    srcheight = 800;

    //cameraBuffer = (unsigned char *) malloc(4*1024*1024);//建立jbarray数组

    //if (cameraBuffer == NULL)
    //{
        //qDebug() << "can not malloc memory \n";
        //exit(-1);
    //}

    int in_buffer_size = srcWidth * srcheight * 2;

    for (int i=0;i<bufferCount;i++)
    {
        g_g2d_src_buffer[i] = g2d_alloc(in_buffer_size, 0);

        if (g_g2d_src_buffer[i] == 0)
        {
            qDebug() << "can not malloc memory for g2d source";
            free(cameraBuffer);
            exit(-1);
        }
    }

    int out_buffer_size = srcWidth * srcheight *4;

    for (int i=0;i<bufferCount;i++)
    {
        g_g2d_dst_buffer[i] =g2d_alloc(out_buffer_size, 0);

        if (g_g2d_dst_buffer[i] == 0)
        {
            qDebug() << "can not malloc memory for g2d dst";
            free(cameraBuffer);
            g2d_free(g_g2d_src_buffer[i]);
            exit(-1);
        }

        memset(g_g2d_dst_buffer[i]->buf_vaddr,0x0,srcWidth * srcheight *4);
    }

    rgb = cv::Mat::zeros( srcheight,srcWidth,CV_8UC4 );

    QFile file("/data/user/point/top/point.txt");

    if (file.exists())
    {
        if (!file.open(QIODevice::ReadWrite)) {
            qDebug() << "open fail" ;
            return;
        }

        char file_buffer[1024];
        qint64 length  = file.readLine(file_buffer,1024);

        if (length == -1)
        {
            qDebug() << "read file fail";
            return ;
        }
        int pos_temp;
        pos_temp = atoi(file_buffer);
        qDebug("%d",pos_temp);

        lefttopwidget->setMouseX(pos_temp);

        length  =  file.readLine(file_buffer,1024);
        if (length == -1)
        {
            qDebug() << "read file fail";
            return ;
        }

        pos_temp = atoi(file_buffer);
        qDebug("%d",pos_temp);
        lefttopwidget->setMouseY(pos_temp);

        length  =  file.readLine(file_buffer,1024);
        if (length == -1)
        {
            qDebug() << "read file fail";
            return ;
        }

        pos_temp = atoi(file_buffer);
        qDebug("%d",pos_temp);
        righttopwidget->setMouseX(pos_temp);

        length  =  file.readLine(file_buffer,1024);
        if (length == -1)
        {
            qDebug() << "read file fail";
            return ;
        }

        pos_temp = atoi(file_buffer);
        qDebug("%d",pos_temp);
        righttopwidget->setMouseY(pos_temp);


        length  =  file.readLine(file_buffer,1024);
        if (length == -1)
        {
            qDebug() << "read file fail";
            return ;
        }

        pos_temp = atoi(file_buffer);
        qDebug("%d",pos_temp);
        rightbottomwidget->setMouseX(pos_temp);

        length  =  file.readLine(file_buffer,1024);
        if (length == -1)
        {
            qDebug() << "read file fail";
            return ;
        }

        pos_temp = atoi(file_buffer);
        qDebug("%d",pos_temp);
        rightbottomwidget->setMouseY(pos_temp);


        length  =  file.readLine(file_buffer,1024);
        if (length == -1)
        {
            qDebug() << "read file fail";
            return ;
        }

        pos_temp = atoi(file_buffer);
        qDebug("%d",pos_temp);
        leftbottomwidget->setMouseX(pos_temp);

        length  =  file.readLine(file_buffer,1024);
        if (length == -1)
        {
            qDebug() << "read file fail";
            return ;
        }

        pos_temp = atoi(file_buffer);
        qDebug("%d",pos_temp);
        leftbottomwidget->setMouseY(pos_temp);


        length  =  file.readLine(file_buffer,1024);
        if (length == -1)
        {
            qDebug() << "read file fail";
            return ;
        }

        zoomDouble = atof(file_buffer);
        file.close();
    }

  //  finishImg = new QImage(420,165,QImage::Format_ARGB32);
finishImg = new QImage(388,155,QImage::Format_ARGB32);

    camera  =  new CameraWorkTop();
    camera->setCameraName("/dev/video0");
    camera->setCameraData(&cameraBuffer);
    connect(camera,SIGNAL(whichTopFrame(int)),this,SLOT(readFrame(int)));
}

void TopCameraWindows::run()
{
//    while(1)
//    {
//        usleep(40);
//    }
}

void TopCameraWindows::readFrame(int frameOffset)
{

    //disconnect(&camera,SIGNAL(whichFrame(int)));
    //frameCount ++;
    //QTime time;
    //time.start();
    //mutex.lock();
    //qDebug() << "frameOffset = " << frameOffset;

    //if (frameCount >= 3)
    //{

        //frameCount = 0;
    //}
    //else
    //{
        CaptureView(frameOffset);
    //}

    //mutex.unlock();
    //qDebug()<<"readFrame time is"<<time.elapsed()/1000.0<<"s";
    //connect(&camera,SIGNAL(whichFrame(int)),this,SLOT(readFrame(int)));
    camera->exit(0);
}

void TopCameraWindows::Start_warpPerspective()
{
    cv::Mat src;
    src = sourcePicture;

    if (initWarpPerspective)
    {
//        warp_dst = cv::Mat::zeros( 165,420,src.type() );
        warp_dst = cv::Mat::zeros( 155,388,src.type() );

//        srcTri[0] = cv::Point2f(lefttopwidget->getMouseX(),lefttopwidget->getMouseY());
//        srcTri[1] = cv::Point2f(leftbottomwidget->getMouseX(),leftbottomwidget->getMouseY());
//        srcTri[2] = cv::Point2f(righttopwidget->getMouseX(),righttopwidget->getMouseY());
//        srcTri[3] = cv::Point2f(rightbottomwidget->getMouseX(),rightbottomwidget->getMouseY());

//        dstTri[0] = cv::Point2f( 120,0 );
//        dstTri[1] = cv::Point2f( 120,165 );
//        dstTri[2] = cv::Point2f( 300,0 );
//        dstTri[3] = cv::Point2f( 300,165);

                srcTri[0] = cv::Point2f(lefttopwidget->getMouseX(),lefttopwidget->getMouseY());
                srcTri[1] = cv::Point2f(righttopwidget->getMouseX(),righttopwidget->getMouseY());
                srcTri[2] = cv::Point2f(rightbottomwidget->getMouseX(),rightbottomwidget->getMouseY());
                srcTri[3] = cv::Point2f(leftbottomwidget->getMouseX(),leftbottomwidget->getMouseY());

 //               dstTri[0] = cv::Point2f( 0,0 );
 //               dstTri[1] = cv::Point2f( 420,0 );
 //               dstTri[2] = cv::Point2f( 322,135 );
//                dstTri[3] = cv::Point2f( 98,135);
              dstTri[0] = cv::Point2f( 0,0 );
              dstTri[1] = cv::Point2f( 388,0 );
              dstTri[2] = cv::Point2f( 264,155 );
              dstTri[3] = cv::Point2f( 124,155);

#if 1
        for (int i=0;i<warp_dst.rows;i++)
        {
            //startx
//            startX[i] =  (((float)120/165) * i);
               startX[i] =  (((float)124/155) * i);
        }

        for (int i=0;i<warp_dst.rows;i++)
        {
            //end
//            endX[i] = warp_dst.cols - (((float)120/165) * i);
            endX[i] = warp_dst.cols - (((float)124/155) * i);
        }
#endif

        warp_matrix = cv::Mat( 3, 3, CV_32FC1);
        warp_matrix=cv::getPerspectiveTransform(srcTri  ,dstTri  );
        warp_matrix_inv = warp_matrix.inv();
        array = new cv::Point [warp_dst.rows * warp_dst.cols];

        int nr= warp_dst.rows; // number of rows
        int nc= warp_dst.cols; // number of columns
        for (int j=0; j<nr; j++)
        {
            //if ( (j>=dstTri[0].y) && (j<=dstTri[1].y) )
            {
                for (int i=0; i<nc; i++)
                {
                    if ( (i>=startX[j]) && (i<=endX[j]) )
                    {
                          cv::Mat source_matrix  = (cv::Mat_<double>(3,1)<<i,j,1); //样例点 (400,240)
                          cv::Mat sourceMat = warp_matrix_inv * source_matrix;  //变换矩阵乘以样例点，得到结果点的其次坐标
                          double amy = sourceMat.at<double>(2,0);  //归一化尺度

                          int  x = ceil(sourceMat.at<double>(0,0)/amy);  //别忘了除以 s 哦
                          int  y = ceil(sourceMat.at<double>(1,0)/amy);

                          if ( (x<0) || (x > src.cols) || (y<0) || (y>src.rows) )
                          {
                              continue;
                          }

                           array[ j*warp_dst.cols + i].x= x;
                           array[ j*warp_dst.cols + i].y= y;
                    }
                } // end of row
            }
        }

        testLoopupTable();

        initWarpPerspective = false;
    }

#if 0
          int nr= warp_dst.rows; // number of rows
          int nc= warp_dst.cols; // number of columns
          for (int j=0,k=0; j<nr; j++) {

              //if ( (j>=dstTri[0].y) && (j<=dstTri[1].y) )
              {
                  for (int i=0; i<nc; i++) {

                      if ( (i>=startX[j]) && (i<=endX[j]) )
                      {
                             //warp_dst.at<cv::Vec4b>(j,i)[0]=     src.at<cv::Vec4b>(array[k].y,array[ k].x)[0];
                             //warp_dst.at<cv::Vec4b>(j,i)[1]=     src.at<cv::Vec4b>(array[k].y,array[ k].x)[1];
                             //warp_dst.at<cv::Vec4b>(j,i)[2]=     src.at<cv::Vec4b>(array[ k].y,array[k].x)[2];
                             //warp_dst.at<cv::Vec4b>(j,i)[3]=     src.at<cv::Vec4b>(array[k].y,array[k].x)[3];
                          memcpy(&warp_dst.at<cv::Vec4b>(j,i)[0],&src.at<cv::Vec4b>(array[k].y,array[ k].x)[0],4);
                      }

                      k++;
              } // end of row
         }
     }

    //finishImg = mat_to_qimage_ref(warp_dst[0],QImage::Format_ARGB32);
    //finishImg[0] = cvMat2QImage(warp_dst[0]);
    //emit updateTopImage();
#endif

    //qDebug()<<time.elapsed()/1000.0<<"s";
}

//
void TopCameraWindows::testLoopupTable()
{
    cv::Mat src;
    src = sourcePicture.clone();



    for(int i=0,k=0;i<warp_dst.rows;i++)
    {
        for(int j=0;j<warp_dst.cols;j++)
        {
            //
            if ( (j>=startX[i]) && (j<=endX[i]) )
            {
                tablePos tempTablePos;
                tempTablePos = caleLoopupTablePos(0,(UTL1Array[ (array[k].y * mapx.cols) + (array[k].x) ].x),( UTL1Array[ (array[k].y * mapx.cols) + (array[k].x) ].y),0);
                writeLoopTablePos(i,j,tempTablePos);
            }
            k++;
        }
    }

    emit updateTopImage();

    saveLoopupTable();
}

void TopCameraWindows::use_fisheye_show()
{

    uchar* srcdata;

    tablePos tempTablePos;

    int nr= finishImg->height(); // number of rows
    int nc= finishImg->width(); // number of columns
    int lenght = 4;
#if 0
        for (int j=0; j<nr; j++) {

        //if ( (j>=dstTri[0].y) && (j<=dstTri[1].y) )
        {
            char* destdata = (char *)finishImg->scanLine(j);
            for (int i=0; i<nc; i++) {

                if ( (i>=startX[j]) && (i<=endX[j]) )
                {
                    tempTablePos = readLoopupTablePos(j,i);

                    srcdata = (uchar *)(g_g2d_dst_buffer[frameCount]->buf_vaddr);
                    srcdata += ((tempTablePos.y)*srcWidth*lenght);
                    srcdata += ((tempTablePos.x)*lenght);

                    memcpy(destdata,srcdata,lenght);
                }
                destdata += lenght;
            } // end of row
        }
    }
#endif

     emit updateTopImage();

#if 0
            int nr= warp_dst[index].rows; // number of rows
            int nc= warp_dst[index].cols; // number of columns
            int lenght = warp_dst[index].channels();

            for (int j=0; j<nr; j++) {

            //if ( (j>=dstTri[0].y) && (j<=dstTri[1].y) )
            {
                destdata= warp_dst[index].ptr<uchar>(j);
                for (int i=0; i<nc; i++) {

                    if ( (i>=startX[j]) && (i<=endX[j]) )
                    {
                        tempTablePos = readLoopupTablePos(j,i);

                        srcdata = rgb.ptr<uchar>(tempTablePos.y);
                        srcdata += tempTablePos.x*lenght;

                        memcpy(destdata,srcdata,lenght);
                    }
                    destdata += lenght;
                } // end of row
            }

        }
        //warp_dst =
        finishImg[index] = mat_to_qimage_ref(warp_dst[index],QImage::Format_ARGB32);
    //finishImg = cvMat2QImage(warp_dst);
    emit updateTopImage();
#endif
}

void TopCameraWindows::CaptureView(int frameOffset)
{    

    //QTime time;
    //time.start();

    //mutex.lock();

    //frameCount = frameOffset;
    frameCount = 0;
    memcpy(g_g2d_src_buffer[frameCount]->buf_vaddr,((cameraBuffer) + (frameOffset * 2 * srcWidth * srcheight)),2*srcWidth*srcheight);
    YUYV2BGRA(g_g2d_src_buffer[frameCount],srcWidth,srcheight,g_g2d_dst_buffer[frameCount],srcWidth,srcheight);
#if 1


        if (firstview == false)
        {
            rgb = cv::Mat(srcheight, srcWidth,CV_8UC4,g_g2d_dst_buffer[frameCount]->buf_vaddr );

//            for (int i =0; i<rgb.rows; i++)
//            {
//                for(int j = 0 ; j < rgb.cols; j ++)
//                {
////                       MscalFig.row(i) = mimg[i][]j
//                  memcpy(&MscalFig.at<cv::Vec4b>(i,j)[0],&rgb.at<cv::Vec4b>(i,j)[0],4);
//                }
//            }



            //CV_LOAD_IMAGE_COLOR
            //data_encode = (unsigned char)(*(unsigned char *)g_g2d_dst_buffer->buf_vaddr);
            //rgb = cv::imdecode((unsigned char *)g_g2d_dst_buffer->buf_vaddr,CV_8UC4);

            //rgb = cv::Mat(srcheight, srcWidth,CV_8UC4,g_g2d_dst_buffer->buf_vaddr );
            //ScanImageAndReduceC(rgb,(unsigned char *)g_g2d_dst_buffer->buf_vaddr);

            if (rgb.empty() == true)
            {
                qDebug() << "frame is empty";
                //mutex.unlock();
                return ;
            }




            cv::Size image_size = rgb.size();
            this->mapx = cv::Mat(image_size ,CV_32FC1);
            this->mapy = cv::Mat(image_size ,CV_32FC1);
            cv::Mat R = cv::Mat::eye(3,3,CV_32F);

            cv::Mat new_intrinsic_mat;    //Mat new_intrinsic_mat(3, 3, CV_64FC1, Scalar(0))亦可，注意数据类型;


            //fx,fy变大（小），视场变小（大），裁剪较多（少），但细节清晰（模糊）；很关键，new_intrinsic_mat决定输出的畸变校正图像的范围
            intrinsic_matrix.copyTo(new_intrinsic_mat);

            //调整输出校正图的视场
            new_intrinsic_mat.at<double>(0, 0) *= zoomDouble;      //注意数据类型，非常重要
            new_intrinsic_mat.at<double>(1, 1) *= zoomDouble;

            //调整输出校正图的中心
            new_intrinsic_mat.at<double>(0, 2) += 0.5;
            new_intrinsic_mat.at<double>(1, 2) += 0.5;

           cv::fisheye::initUndistortRectifyMap(intrinsic_matrix,distortion_coeffs,R,new_intrinsic_mat,image_size ,CV_32FC1,mapx,mapy);


            initUTL1Array();
            sourcePicture = rgb.clone();
            my_show_fun();
            Start_warpPerspective();
            firstview = true;
        }


        //sourcePicture = rgb.clone();
        //qDebug()<<time.elapsed()/1000.0<<"s";
        //cv::remap(rgb,sourcePicture,mapx, mapy, cv::INTER_NEAREST);
        //image = cvMat2QImage(rgb);
        //my_show_fun();

        //Start_warpPerspective();

        use_fisheye_show();

#else

        uchar * srcData = (uchar *)(g_g2d_dst_buffer[frameCount]->buf_vaddr);

        for (int y = 0,k=0;y<srcheight;y++)
        {
            char* line = (char*)image->scanLine(y);
            //for (int x=0;x<srcWidth;x++)
            {
                memcpy(line,srcData,srcWidth*4);
                srcData+=srcWidth*4;
                //memcpy(line+k,srcData+k,4);
                //k+=4;
            }
        }

    //if (image.)
    //image = QImage((const uchar *)g_g2d_dst_buffer->buf_vaddr,srcWidth,srcheight,QImage::Format_ARGB32);
    emit updateTopImage();
#endif

        //mutex.unlock();
        //qDebug()<<time.elapsed()/1000.0<<"s";
}

void TopCameraWindows::initUTL1Array()
{
    UTL1Array = new cv::Point[mapx.rows*mapx.cols];

    for (int i=0,k=0;i<mapx.rows;i++)
    {
        for(int j=0;j<mapx.cols;j++)
        {
            //
            UTL1Array[k].x = (int)(mapx.at<float>(i,j));
            k++;
        }
    }

    for (int i=0,k=0;i<mapy.rows;i++)
    {
        for(int j=0;j<mapy.cols;j++)
        {
            //
            UTL1Array[k].y = (int)(mapy.at<float>(i,j));
            k++;
        }
    }
}

void TopCameraWindows::my_show_fun()
{
    //sourcePicture =cv::Mat::zeros( 480,720,sourcePicture.type() );

    //QTime time;
    //time.start();

    int x = 0;
    int y = 0;

    int lenght = sourcePicture.channels();

    for (int i=0,k=0;i<sourcePicture.rows;i++)
    {
        for (int j=0;j<sourcePicture.cols;j++)
        {
            x = UTL1Array[k].x;
            y = UTL1Array[k].y;

            //sourcePicture.at<cv::Vec4b>(i,j)[0] =     rgb.at<cv::Vec4b>(y, x)[0];
            //sourcePicture.at<cv::Vec4b>(i,j)[1] =     rgb.at<cv::Vec4b>(y, x)[1];
            //sourcePicture.at<cv::Vec4b>(i,j)[2] =     rgb.at<cv::Vec4b>(y, x)[2];
            //sourcePicture.at<cv::Vec4b>(i,j)[3] =     rgb.at<cv::Vec4b>(y, x)[3];
            memcpy(&sourcePicture.at<cv::Vec4b>(i,j)[0], &rgb.at<cv::Vec4b>(y, x)[0],lenght);
            k++;
        }
    }
    //qDebug()<<time.elapsed()/1000.0<<"s";
}


//QImage TopCameraWindows::mat_to_qimage_ref(cv::Mat &mat, QImage::Format format)
//{
//    return QImage(mat.data, mat.cols, mat.rows, mat.step, format);
//}

QImage TopCameraWindows::cvMat2QImage(const cv::Mat& mat)
{
    // 8-bits unsigned, NO. OF CHANNELS = 1
    if(mat.type() == CV_8UC1)
    {
        QImage image(mat.cols, mat.rows, QImage::Format_Indexed8);
        // Set the color table (used to translate colour indexes to qRgb values)
        image.setColorCount(256);
        for(int i = 0; i < 256; i++)
        {
            image.setColor(i, qRgb(i, i, i));
        }
        // Copy input Mat
        uchar *pSrc = mat.data;
        for(int row = 0; row < mat.rows; row ++)
        {
            uchar *pDest = image.scanLine(row);
            memcpy(pDest, pSrc, mat.cols);
            pSrc += mat.step;
        }
        return image;
    }
    // 8-bits unsigned, NO. OF CHANNELS = 3
    else if(mat.type() == CV_8UC3)
    {
        // Copy input Mat
        const uchar *pSrc = (const uchar*)mat.data;
        // Create QImage with same dimensions as input Mat
        QImage image(pSrc, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);
        return image.rgbSwapped();
    }
    else if(mat.type() == CV_8UC4)
    {
        //qDebug() << "CV_8UC4";
        // Copy input Mat
        const uchar *pSrc = (const uchar*)mat.data;
        // Create QImage with same dimensions as input Mat
        QImage image(pSrc, mat.cols, mat.rows, mat.step, QImage::Format_ARGB32);
        return image.copy();
    }
    else
    {
        //qDebug() << "ERROR: Mat could not be converted to QImage.";
        return QImage();
    }
}


//cv::Mat& TopCameraWindows::ScanImageAndReduceC(cv::Mat& I, const uchar* const table)
//{
//     // accept only char type matrices
//    CV_Assert(I.depth() != sizeof(uchar));

//    int channels = I.channels();
//    int nRows = I.rows;
//    int nCols = I.cols * channels;

//    if (I.isContinuous())
//    {
//        nCols *= nRows;
//        nRows = 1;
//    }

//    int i,j;
//    uchar* p;
//    for( i = 0; i < nRows; ++i)
//    {
//        p = I.ptr<uchar>(i);
//        for ( j = 0; j < nCols; ++j)
//        {
//            p[j] = table[p[j]];
//        }
//    }
//     return I;
//}

//QImage * TopCameraWindows::getFinishImage()
//{
//    return finishImg;
//}

//QImage * TopCameraWindows::getFisheyeImage()
//{
//    return image;
//}

TopCameraWindows::~TopCameraWindows()
{
    free(cameraBuffer);
    for (int i=0;i<bufferCount;i++)
    {
        g2d_free(g_g2d_src_buffer[i]);
        g2d_free(g_g2d_dst_buffer[i]);
    }
}
