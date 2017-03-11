#include "alignwindow.h"
#include "ui_alignwindow.h"
#include "foregroundextract.h"
#include "cvtoqt.h"

#include <QDebug>
#include <iostream>
#include <unistd.h>

AlignWindow::AlignWindow(ImageManager* my_manager, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AlignWindow)
{
    manager = my_manager;
    
    ui->setupUi(this);
    ui->horizontalSlider->setMaximum(manager->num_images-1);
    
    portalsize = manager->images[0].size();
    qportalsize = QSize(portalsize.width, portalsize.height);
}

void AlignWindow::showNewImage(int imnum)
{
    int w = ui->imageportal->width();
    int h = ui->imageportal->height();
    Mat resized;
    cv::resize(manager->images[imnum], resized, portalsize);
    QImage newImage = putImage(resized);
    ui->imageportal->setPixmap((QPixmap::fromImage(newImage)).scaled(w,h,Qt::KeepAspectRatio));
}

void AlignWindow::resizeEvent(QResizeEvent* event){
    
    qportalsize = QSize(manager->imgdims.width,manager->imgdims.height);
    QSize newsize = ui->imageportal->size();
    qportalsize.scale(newsize, Qt::KeepAspectRatio);
    portalsize = Size(qportalsize.width(), qportalsize.height());
    showNewImage(ui->horizontalSlider->value());
}

Mat getWarp(Mat params)
{
    double tx = params.at<double>(0,0);
    double ty = params.at<double>(1,0);
    double a  = params.at<double>(2,0);
    double b  = params.at<double>(3,0);
    
    Mat W = Mat::eye(3, 3, CV_64F);
    W.at<double>(0,0) += a;
    W.at<double>(1,1) += a;
    W.at<double>(0,1) -= b;
    W.at<double>(1,0) += b;
    W.at<double>(0,2) = tx;
    W.at<double>(1,2) = ty;
    
    return W;
}


void dispMat(Mat M){
    
    if(M.channels() > 1){
        for(int c = 0; c < M.channels(); c++){
            for(int i = 0; i < M.rows; i++){
                for(int j = 0; j < M.cols; j++)
                    std::cout << M.at<Vec2d>(i,j)[c] << "\t";
                std::cout << std::endl;
            }
            std::cout << std::endl;
        }
    }
    else{
        for(int c = 0; c < M.channels(); c++){
            for(int i = 0; i < M.rows; i++){
                for(int j = 0; j < M.cols; j++)
                    std::cout << M.at<double>(i,j) << "\t";
                std::cout << std::endl;
            }
        }
    }
}

void AlignWindow::proceed()
{
    ForegroundExtract* extractor = new ForegroundExtract(manager);
    extractor->show();
    this->close();
}

void AlignWindow::alignAllImages()
{
    if(ui->pushButton->text() == "Align"){
        
        // update UI
        ui->horizontalSlider->setDisabled(true);
        ui->pushButton->setDisabled(true);
        ui->pushButton_2->setDisabled(true);
        imgnum0 = ui->horizontalSlider->sliderPosition();
        this->repaint();
        
        // get portal worldspace coords
        getPortalCoords();
        
        // find warps to align each image
        vector<Mat> warps = alignEachImage();
        
        // update UI
        ui->output->setText("Aligning originals...");
        this->repaint();
        qDebug() << "Aligning originals...";
        
        // warp original images
        warpOriginals(warps);
        
        // update UI
        ui->output->setText("Images Aligned.");
        showNewImage(0);
        ui->horizontalSlider->setValue(0);
        ui->horizontalSlider->setDisabled(false);
        ui->pushButton->setText("Write to Image Files");
        ui->pushButton->setDisabled(false);
        ui->pushButton_2->setDisabled(false);
    }
    else{
        writeImages();
    }
}

vector<Mat> AlignWindow::alignEachImage(){
    
    // convert to black and white
    cvtColor(manager->images[imgnum0], bw0, CV_RGB2GRAY);
    
    // crop refernce image to template
    origdims = bw0.size();
    origdims_T = Size(0.8*origdims.width, 0.8*origdims.height);
    Point2i tl((origdims.width - origdims_T.width)/2, (origdims.height - origdims_T.height)/2);
    Rect rect_T(tl, origdims_T);
    origT = new Mat(bw0, rect_T);
    origT->convertTo(*origT, CV_64F);
    *origT = *origT / 255.0;
    
    vector<Mat> warps;
    for(imgnum1 = 0; imgnum1 < manager->num_images; imgnum1++){
        if(imgnum1 != imgnum0){
            
            // convert to black and white
            cvtColor(manager->images[imgnum1], bw1, CV_RGB2GRAY);
            
            // update UI
            ui->horizontalSlider->setSliderPosition(imgnum1);
            QString msg = "Aligning image ";
            QTextStream(&msg) << imgnum1 << " with image " << imgnum0 << "...";
            ui->output->setText(msg);
            this->repaint();
            
            // align this pair of images
            Mat W = alignImagePair();
            warps.push_back(W);
            
            // update UI
            msg = "Images ";
            QTextStream(&msg) << imgnum1 << " and " << imgnum0 << " aligned.";
            ui->output->setText(msg);
            showCurrentWarp(W);
            this->repaint();
            sleep(1);
        }
        else
            warps.push_back(Mat::eye(3,3,CV_64F));
    }
    qDebug() << "Done.";
    
    delete origT;
    return warps;
}

Mat AlignWindow::alignImagePair(){
    
    // initialize warp matrix to the identity
    Mat W = Mat::eye(3, 3, CV_64F);
    showCurrentWarp(W);
    sleep(1);
    
    // increment scaling of image through 0.05, 0.1, 0.2, 0.4, 0.8
    for(double scale = 0.05; scale <= 0.8; scale *= 2)
    {
        
        
        qDebug() << "starting at scale " << scale;
        // precompute matrices for iteration
        precomputeMatrices(scale);
        
        qDebug() << "refining...";
        // iteratively refine warp with steepest descent
        W = refineWarp(W);
    }
    
    return W;
}

void AlignWindow::precomputeMatrices(double scale){
    
    // release all previous matrices
    T_coords.release();
    T.release(); I.release();
    VTx.release(); VTy.release(); SDa.release(); SDb.release(); H.release();
    
    // get new dimensions for this scale factor
    Size dims = Size(scale*origdims.width, scale*origdims.height);
    Size dims_T = Size(scale*origdims_T.width, scale*origdims_T.height);
    
    // resize images for template and target images
    cv::resize(*origT, T, dims_T, 0, 0, INTER_AREA );
    cv::resize(bw1, I, dims, 0, 0, INTER_AREA );
    I.convertTo(I, CV_64F);
    I = I / 255.0;
    
    // calculate normalized device coordinates for x and y
    std::vector<double> xT_coords, yT_coords;
    double delta = 1.6/(dims_T.width-1);
    double a = ((double)(dims.height - 1))/((double)(dims.width-1));
    double x = -0.8;
    double y = -0.8*a;
    for(int i = 0; i < dims_T.width; i++){
        xT_coords.push_back(x);
        x += delta;
    }
    for(int i = 0; i < dims_T.height; i++){
        yT_coords.push_back(y);
        y += delta;
    }
    
    // create Matrix objects from vectors
    Mat xT_coords_mat(xT_coords);
    Mat yT_coords_mat(yT_coords);
    
    // repeat vectors
    Mat XT_coords, YT_coords;
    XT_coords = repeat(xT_coords_mat.t(), yT_coords.size(), 1);
    YT_coords = repeat(yT_coords_mat, 1, xT_coords.size());
    
    // get gradient images
    Scharr( T, VTx, -1, 1, 0, 0.03125/delta);
    Scharr( T, VTy, -1, 0, 1, 0.03125/delta);
    
    // get a,b steepest descent images
    SDa = VTx.mul(XT_coords) + VTy.mul(YT_coords);
    SDb = VTy.mul(XT_coords) - VTx.mul(YT_coords);
    vector<Mat> SD;
    SD.push_back(VTx);
    SD.push_back(VTy);
    SD.push_back(SDa);
    SD.push_back(SDb);
    
    // calculate Hessian
    H = Mat::zeros(4,4, CV_64F);
    for(int j = 0; j < 4; j++)
        for(int i = j; i < 4; i++){
            Mat img = SD[i].mul(SD[j]);
            Scalar total = sum(img);
            H.at<double>(i,j) = total[0];
            if(i != j)
                H.at<double>(j,i) = total[0];
        }
    
    // merge matrices into one
    vector<Mat> comps;
    comps.push_back(XT_coords);
    comps.push_back(YT_coords);
    merge(comps, T_coords);
}

Mat AlignWindow::refineWarp(Mat W){
    
    // initialize del_p so the norm condition is true in first pass
    Mat del_p = Mat::ones(4,1,CV_64F);
    int i = 0;
    double tol = 0.0001;
    
    while (i < 40 && norm(del_p) > tol){
        
        // warp image
        Mat* warped_img = warpTarget(W, &I, &T_coords);
        
        // calculate residual
        Mat resid = calcResid(warped_img);
        delete warped_img;
        
        // compute incremental parameters
        solve(H, resid, del_p, DECOMP_CHOLESKY);
        
        // update warp
        Mat dW = getWarp(del_p);
        W = W * dW.inv();
        
        // show current warp
        showCurrentWarp(W);
    }
    
    return W;
}

Mat* AlignWindow::warpTarget(Mat W, Mat* target, Mat* Coords){
    
    // get 2x3 inverse warp matrix
    Mat invW = W.inv();
    Mat T = Mat(invW, Range(0,2), Range::all());
    
    // transform coordinates to warped image normalized space
    Mat WCoords;
    transform(*Coords, WCoords, T);
    
    // convert to pixel coords
    Size mydims = target->size();
    double S = (mydims.width-1);
    Scalar offset(mydims.width, mydims.height);
    WCoords = (S * WCoords + offset)/2.0 - 0.5;
    
    // remap target image
    Mat* warpedI = new Mat();
    WCoords.assignTo(WCoords, CV_32FC2);
    remap(*target,*warpedI,WCoords, Mat(),INTER_CUBIC);
    
    return warpedI;
}

Mat AlignWindow::calcResid(Mat* warped_img){
    
    Mat diff_img = T - *warped_img;
    
    Mat residMat = Mat(4,1,CV_64F);
    
    residMat.at<double>(0,0) = sum(diff_img.mul(VTx))[0];
    residMat.at<double>(1,0) = sum(diff_img.mul(VTy))[0];
    residMat.at<double>(2,0) = sum(diff_img.mul(SDa))[0];
    residMat.at<double>(3,0) = sum(diff_img.mul(SDb))[0];
    
    return residMat;
}

void AlignWindow::getPortalCoords(){
    
    // get x coordinates
    std::vector<double> xP_coords;
    double S = (portalsize.width-1);
    double delta = 2.0/S;
    double x = -1.0;
    for(int i = 0; i < portalsize.width; i++){
        xP_coords.push_back(x);
        x += delta;
    }
    // get y coordinates
    std::vector<double> yP_coords;
    double a = ((double)(portalsize.height - 1))/S;
    double y = -a;
    for(int i = 0; i < portalsize.height; i++){
        yP_coords.push_back(y);
        y += delta;
    }
    
    // create Matrix objects from vectors
    Mat XP_coords = Mat(xP_coords);
    Mat YP_coords = Mat(yP_coords);
    
    // repeat vectors
    XP_coords = repeat(XP_coords.t(), portalsize.height, 1);
    YP_coords = repeat(YP_coords    , 1, portalsize.width );
    
    // merge matrices into one
    vector<Mat> comps;
    comps.push_back(XP_coords);
    comps.push_back(YP_coords);
    merge(comps, P_coords);
    XP_coords.release(); YP_coords.release();
}

void AlignWindow::warpOriginals(vector<Mat> warps){
    
    // release all unnecessary matrices
    T_coords.release();
    T.release(); I.release();
    VTx.release(); VTy.release(); SDa.release(); SDb.release(); H.release();
    
    // calculate normalized device coordinates for x and y
    std::vector<double> xT_coords, yT_coords;
    double delta = 1.6/(origdims_T.width-1);
    double a = ((double)(origdims.height - 1))/((double)(origdims.width-1));
    double x = -0.8;
    double y = -0.8*a;
    for(int i = 0; i < origdims_T.width; i++){
        xT_coords.push_back(x);
        x += delta;
    }
    for(int i = 0; i < origdims_T.height; i++){
        yT_coords.push_back(y);
        y += delta;
    }
    
    // create Matrix objects from vectors
    Mat xT_coords_mat(xT_coords);
    Mat yT_coords_mat(yT_coords);
    
    // repeat vectors
    Mat XT_coords, YT_coords;
    XT_coords = repeat(xT_coords_mat.t(), yT_coords.size(), 1);
    YT_coords = repeat(yT_coords_mat, 1, xT_coords.size());    
    
    // merge matrices into one
    vector<Mat> comps;
    comps.push_back(XT_coords);
    comps.push_back(YT_coords);
    merge(comps, T_coords);
    XT_coords.release(); YT_coords.release();
    
    // warp original images
    for(int i = 0; i < manager->num_images; i++){
        warpOrig(warps[i], i, &T_coords);
    }
    manager->imgdims = manager->images[0].size();
}

void AlignWindow::warpOrig(Mat W, int imgnum, Mat* Coords){
    // get 2x3 inverse warp matrix
    Mat invW = W.inv();
    Mat T = Mat(invW, Range(0,2), Range::all());
    
    // transform coordinates to warped image normalized space
    Mat WCoords;
    transform(*Coords, WCoords, T);
    
    // convert to pixel coords
    Size mydims = manager->images[imgnum].size();
    double S = (mydims.width-1);
    Scalar offset(mydims.width, mydims.height);
    WCoords = (S * WCoords + offset)/2.0 - 0.5;
    
    // remap target image
    WCoords.assignTo(WCoords, CV_32FC2);
    remap(manager->images[imgnum],manager->images[imgnum],WCoords, Mat(),INTER_CUBIC);
}

void AlignWindow::showCurrentWarp(Mat W){
    
    Mat refimg;
    cv::resize(manager->images[imgnum0], refimg, portalsize);
    
    Mat* portalImage = new Mat();
    cv::resize(manager->images[imgnum1], *portalImage, portalsize);
    Mat* warpedImg = warpTarget(W, portalImage, &P_coords);
    delete portalImage;
    
    Mat blend;
    addWeighted(refimg, 0.5, *warpedImg, 0.5, 0.0, blend);
    delete warpedImg;
    
    QImage newImage = putImage(blend);
    ui->imageportal->setPixmap(QPixmap::fromImage(newImage));
    this->repaint();
}

void AlignWindow::writeImages(){
    
    qDebug() << "Writing images to ";
    
    QString filename = manager->directory;
    QDir directory = QDir(filename);
    directory.mkdir("aligned");
    directory.cd("aligned");
    
    qDebug() << directory.path();
    
    for(int i = 0; i < manager->num_images; i++){
        
        QString filename = directory.path();
        filename.append("/image");
        filename.append(QString::number(i));
        filename.append(".jpg");
        
        qDebug() << "Writing to " << filename;
        imwrite(filename.toLocal8Bit().constData(), manager->images[i]);
        qDebug() << filename << " written.";
    }
}

AlignWindow::~AlignWindow()
{
    delete ui;
}
