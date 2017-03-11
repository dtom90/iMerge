#include "foregroundextract.h"
#include "ui_foregroundextract.h"
#include "cvtoqt.h"
#include "merger.h"

#include <QDebug>

ForegroundExtract::ForegroundExtract(ImageManager* my_manager, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ForegroundExtract)
{
    // initialize class variables
    manager = my_manager;
    num_pairs = manager->num_images-1;
    
    // initialize strucutres for holding extracted objects
    objectRects = vector<vector<Rect> >(manager->num_images, vector<Rect>());
    extractedObjects = vector<Mat>(manager->num_images);
    for(int i = 0; i < manager->num_images; i++){
        extractedObjects[i] = Mat::zeros(manager->imgdims, manager->images[0].type());
    }
    
    // extract all the objects
    qDebug() << manager->num_images;
    for(int i = 0; i < num_pairs; i++){
        extractObjects(i, false);
    }
    qDebug() << "Done extracting.";
    
    // setup user interface
    ui->setupUi(this);
    ui->horizontalSlider->setMaximum(num_pairs-1);
    current_pair = 0;
}

void ForegroundExtract::proceed()
{
    Merger* extractor = new Merger(manager, extractedObjects);
    extractor->show();
    this->close();
}


void ForegroundExtract::resizeEvent(QResizeEvent* event){
    
    qportalsize = QSize(manager->imgdims.width,manager->imgdims.height);
    QSize newsize = ui->portal1->size();
    qportalsize.scale(newsize, Qt::KeepAspectRatio);
    portalsize = Size(qportalsize.width(), qportalsize.height());
    showImagePair(current_pair);
}

void ForegroundExtract::extractObjects(int pair, bool display){
    
    // instantiate variables
    Mat dispimg;
    Size dispsize(960, 720);
    Mat diff_image, fgd_mask, bgd_mask, big_bgd_mask, watershedMask1, watershedMask2;
    Mat ext1, ext2, ext_img1, ext_img2, halo1, halo2, halo_img1, halo_img2;
    vector<vector<Point> > contours;
    vector<Rect> boundingBoxes;
    double minVal, maxVal;
    
    // calculate the absolute difference image of blurred black & white images
    qDebug() << "converting pair " << pair << "...";
    Mat imageBW, blurredImage1, blurredImage2;
    cvtColor(manager->images[pair], imageBW, CV_RGB2GRAY);
    GaussianBlur(imageBW, blurredImage1, manager->kernel, 0);
    cvtColor(manager->images[pair+1], imageBW, CV_RGB2GRAY);
    GaussianBlur(imageBW, blurredImage2, manager->kernel, 0);
    cv::absdiff(blurredImage1, blurredImage2, diff_image);
    qDebug() << "converted.";
    
    if(display){
        cv::resize(blurredImage1, dispimg, dispsize);
        imshow("Image 1", dispimg);
        cv::resize(blurredImage2, dispimg, dispsize);
        imshow("Image 2", dispimg);
        cv::resize(diff_image, dispimg, dispsize);
        imshow("Absdiff", dispimg);
        waitKey(0);
    }
    
    // erode, threshold, erode for foreground mask
    cv::erode(diff_image, fgd_mask, cv::Mat(), Point(-1,-1), 4);
    cv::minMaxLoc(fgd_mask, &minVal, &maxVal);
    cv::threshold(fgd_mask, fgd_mask, 0.1*maxVal, 255, THRESH_BINARY);
    cv::erode(fgd_mask, fgd_mask, cv::Mat(), Point(-1,-1), 4);
//    if(display){
//        cv::resize(fgd_mask, dispimg, dispsize);
//        imshow("Erode Thresh Erode Diff", dispimg);
//    }
    
    // dilate, blur, threshold for background mask
    cv::dilate(diff_image, bgd_mask, cv::Mat(), Point(-1,-1), 8);
    cv::GaussianBlur( bgd_mask, bgd_mask, manager->kernel, 0, 0, BORDER_DEFAULT);
    cv::minMaxLoc(bgd_mask, &minVal, &maxVal);
    cv::threshold(bgd_mask, bgd_mask, 0.1*maxVal, 255, THRESH_BINARY);
//    if(display){
//        cv::resize(bgd_mask, dispimg, dispsize);
//        imshow("Background Mask", dispimg);
//    }
    
    // extract only the eroded elements inside the background mask for the final foreground mask
    fgd_mask = fgd_mask & bgd_mask;
//    if(display){
//        cv::resize(fgd_mask, dispimg, dispsize);
//        imshow("Foreground Mask", dispimg);
//    }
    
    // dilate for big background mask, invert smaller background mask for watershed
    cv::dilate(bgd_mask, big_bgd_mask, cv::Mat(), Point(-1,-1), 2);
    cv::threshold(bgd_mask, bgd_mask, 0.5*maxVal, 128, THRESH_BINARY_INV);
    
    // combine foreground and background masks for watershed mask
    watershedMask1 = Mat(manager->images[0].size(),CV_8U,cv::Scalar(0));
    watershedMask1 = fgd_mask + bgd_mask;
//    if(display){
//        cv::resize(watershedMask1, dispimg, dispsize);
//        imshow("Pre Watershed Mask", dispimg);
//    }
    
    // make a copy of the mask
    watershedMask1.copyTo(watershedMask2);
    
    // watershed image 1
    watershedMask1.convertTo(watershedMask1, CV_32S);
    watershed(manager->images[pair], watershedMask1);
    watershedMask1.convertTo(watershedMask1, CV_8U);
    
    // watershed image 2
    watershedMask2.convertTo(watershedMask2, CV_32S);
    watershed(manager->images[pair+1], watershedMask2);
    watershedMask2.convertTo(watershedMask2, CV_8U);
    
    // extract foreground objects from each image
    ext1 = watershedMask1 > 130;
    ext2 = watershedMask2 > 130;
    manager->images[pair].copyTo(ext_img1, ext1);
    manager->images[pair+1].copyTo(ext_img2, ext2);
//    if(display){
//        cv::resize(ext_img1, dispimg, dispsize);
//        imshow("Extracted Image 1", dispimg);
//        cv::resize(ext_img2, dispimg, dispsize);
//        imshow("Extracted Image 2", dispimg);
//    }
    
    // extract halo areas of each image
    bitwise_xor(big_bgd_mask, ext1, halo1);
    bitwise_xor(big_bgd_mask, ext2, halo2);
    manager->images[pair].copyTo(halo_img1, halo1);
    manager->images[pair+1].copyTo(halo_img2, halo2);
//    if(display){
//        cv::resize(halo_img1, dispimg, dispsize);
//        imshow("Halo Image 1", dispimg);
//        cv::resize(halo_img2, dispimg, dispsize);
//        imshow("Halo Image 2", dispimg);
//    }
    
    // separate object regions
    findContours(big_bgd_mask, contours, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);
    
    // get bounding boxes
    boundingBoxes = vector<Rect>(contours.size());
    for( uint i = 0; i < contours.size(); i++ )
        boundingBoxes[i] = boundingRect(contours[i]);
    
    // histogram calculation for each object area
    MatND hist1in, hist1out, hist2in, hist2out;
    int hbins = 30, sbins = 32;
    int histSize[] = {hbins, sbins};
    float hranges[] = { 0, 180 };
    float sranges[] = { 0, 256 };
    const float* ranges[] = { hranges, sranges };
    int channels[] = {0, 1};
    Mat innermask1, innermask2, outermask, content, hsv;
    Mat innercontent1, innercontent2;
    
    for( uint i = 0; i < boundingBoxes.size(); i++ ){
        
        Rect box = boundingBoxes[i];
        
        innermask1 = Mat(ext1, box);
        innermask2 = Mat(ext2, box);
        
        if(countNonZero(innermask1) > 0 && countNonZero(innermask2) > 0){
        
            // get image 1 histograms
            outermask = Mat(halo1, box);
            content = Mat(manager->images[pair], box);
            cvtColor(content, hsv, CV_RGB2HSV);
            calcHist( &hsv, 1, channels, innermask1,
                      hist1in, 2, histSize, ranges);
            calcHist( &hsv, 1, channels, outermask,
                      hist1out, 2, histSize, ranges);
            
            // get image 2 histograms
            outermask = Mat(halo2, box);
            content = Mat(manager->images[pair+1], box);
            cvtColor(content, hsv, CV_RGB2HSV);
            calcHist( &hsv, 1, channels, innermask2,
                      hist2in, 2, histSize, ranges);
            calcHist( &hsv, 1, channels, outermask,
                      hist2out, 2, histSize, ranges);
            
            // get the correlation with the background
            double backstrength1 = compareHist(hist1in, hist1out, CV_COMP_CORREL);
            double backstrength2 = compareHist(hist2in, hist2out, CV_COMP_CORREL);
            
            // get the object content
            innercontent1 = Mat(ext_img1, box);
            innercontent2 = Mat(ext_img2, box);
            
//            if(display){
//                qDebug() << "backstrength1 = " << backstrength1;
//                qDebug() << "backstrength2 = " << backstrength2;
//                Mat outerconent1  = Mat(halo_img1, box);
//                Mat outerconent2  = Mat(halo_img2, box);
//                imshow("innermask 1", innercontent1);
//                imshow("outermask 1", outerconent1);
//                imshow("innermask 2", innercontent2);
//                imshow("outermask 2", outerconent2);
//                waitKey(0);
//            }
            
            if(backstrength1 > backstrength2){               // image 1 is more background, image 2 has the object
                innercontent2.copyTo(extractedObjects[pair+1](box));
                objectRects.at(pair+1).push_back(box);
            }
            else if(backstrength2 > backstrength1){          // image 2 is more background, image 1 has the object
                innercontent1.copyTo(extractedObjects[pair](box));
                objectRects.at(pair).push_back(box);
            }
        }
    }
}

void ForegroundExtract::dumpImages(){
    
    extractObjects(current_pair, true);
}

void ForegroundExtract::showImagePair(int imnum)
{
    qDebug() << "Showing pair " << imnum;
    
    qDebug() << manager->images[imnum].size().width << "," << manager->images[imnum].size().height;
    qDebug() << extractedObjects[imnum].size().width << "," << extractedObjects[imnum].size().height;
    
    Mat img1 = 0.5*manager->images[imnum] + extractedObjects[imnum];
    Mat img2 = 0.5*manager->images[imnum+1] + extractedObjects[imnum+1];
    
    qDebug() << "op complete.";
    
    Scalar color = Scalar(0,0,255);
    for( uint i = 0; i < objectRects[imnum].size(); i++ )
        rectangle(img1, objectRects[imnum][i], color, 4);
    for( uint i = 0; i < objectRects[imnum+1].size(); i++ )
        rectangle(img2, objectRects[imnum+1][i], color, 4);
    
    Mat resized;
    cv::resize(img1, resized, portalsize);
    QImage newImage = putImage(resized);
    ui->portal1->setPixmap(QPixmap::fromImage(newImage));
    
    QString msg = "Image ";
    QTextStream(&msg) << imnum << " ";
    ui->label1->setText(msg);
    
    cv::resize(img2, resized, portalsize);
    newImage = putImage(resized);
    ui->portal2->setPixmap(QPixmap::fromImage(newImage));
    
    msg = "Image ";
    QTextStream(&msg) << imnum+1;
    ui->label2->setText(msg);
    
    current_pair = imnum;
}

ForegroundExtract::~ForegroundExtract()
{
    delete ui;
    QApplication::quit();
}
