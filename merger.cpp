#include "merger.h"
#include "ui_merger.h"
#include "cvtoqt.h"

Merger::Merger(ImageManager* my_manager, vector<Mat> my_extractedObjects, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Merger)
{
    
    manager = my_manager;
    extractedObjects = my_extractedObjects;
    
    double w = 1.0 / manager->num_images;
    cleanMerge = Mat::zeros(manager->images[0].size(), manager->images[0].type());
    mergedObjects = Mat::zeros(extractedObjects[0].size(), extractedObjects[0].type());
    for(int i = 0; i < manager->num_images; i++){
        extractedObjects[i].copyTo(mergedObjects, extractedObjects[i]);
        cleanMerge += w * cleanObjects(i);
    }
    cleanMerge.copyTo(finalImage);
    
    ui->setupUi(this);
    current_img = 0;
}

void Merger::resizeEvent(QResizeEvent* event){
    
    qportalsize = QSize(manager->imgdims.width,manager->imgdims.height);
    QSize newsize = ui->portal->size();
//    qDebug() << newSize.width() << "," << newSize.height();
    qportalsize.scale(newsize, Qt::KeepAspectRatio);
    portalsize = Size(qportalsize.width(), qportalsize.height());
    showMergedImage();
}

Mat Merger::cleanObjects(int imnum){
    
    Mat cleanImage;
    manager->images[imnum].copyTo(cleanImage);
    
    if(imnum < manager->num_images-1)
        manager->images[imnum+1].copyTo(cleanImage, extractedObjects[imnum]);
    if(imnum > 0)
        manager->images[imnum-1].copyTo(cleanImage, extractedObjects[imnum]);
    
    return cleanImage;
}

void Merger::addAllObjects(){
    ui->allButton->setEnabled(false);
    ui->clearButton->setEnabled(false);
    cleanMerge.copyTo(finalImage);
    mergedObjects.copyTo(finalImage, mergedObjects);
    showMergedImage();
}

void Merger::removeAllObjects(){
    ui->allButton->setEnabled(false);
    ui->clearButton->setEnabled(false);
    cleanMerge.copyTo(finalImage);
    showMergedImage();
}

void Merger::customObjects(){
    ui->allButton->setEnabled(true);
    ui->clearButton->setEnabled(true);
}

void Merger::showMergedImage(){
    
    Mat resized;
    cv::resize(finalImage, resized, portalsize);
    QImage newImage = putImage(resized);
    ui->portal->setPixmap(QPixmap::fromImage(newImage));
}

Merger::~Merger()
{
    delete ui;
}
