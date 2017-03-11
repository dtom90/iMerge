#include "imagemanager.h"
#include <QDebug>

ImageManager::ImageManager(void) {
/**
 * Loads all images from the default directory (defined by default_path in header file)
 */

    /** CHECK FOR IMAGE FILES **/
    QStringList imageFiles = getImageFiles(default_path);

    /** LOAD THE SELECTED FILES **/
    loadImages(imageFiles);
}

ImageManager::ImageManager(QString path){
/**
 * Loads all images from the directory specified by QString path
 * if dir is null, load images from default directory (defined by path at top)
 */

    /** CHECK FOR IMAGE FILES **/
    QStringList imageFiles = getImageFiles(path);

    /** LOAD THE SELECTED FILES **/
    loadImages(imageFiles);
}

ImageManager::ImageManager(QStringList filenames) {

    /** LOAD THE SELECTED FILES **/
    loadImages(filenames);
}

QStringList ImageManager::getImageFiles(QString dir){

    QStringList imageFiles;
    QDirIterator it(dir);
    while (it.hasNext()) {
        QString path = it.next();
        QFileInfo file = it.fileInfo();
        QString ext = file.completeSuffix();
        if(ext.toUpper() == "JPG"){
            imageFiles.append(path);
        }
    }

    return imageFiles;
}

void ImageManager::loadImages(QStringList filenames){
/**
 * Loads images specified by the given QStringList, which specifies a list of paths to image files
 * if filenames is null, load images from default directory (defined by path at top)
 */
    
    /** INITALIZE VARIABLES **/
    directory = QString(filenames.at(0));
    qDebug() << directory;
    directory.truncate(directory.lastIndexOf("/"));
    qDebug() << directory;
    num_images   = filenames.size();
    images       = new Mat[num_images];
    
    /** READ IMAGE FILES **/
    qDebug() << "Loading " << num_images << " images...";
    for (int i = 0; i < num_images; i++)
    {
        QString filename = filenames.at(i);
        qDebug() << "loading image " << i << filename << endl;
        images[i] = imread(filename.toLocal8Bit().constData());
    }

    /** CHECK FOR SUCCESSFUL READ, **/
    for(int i = 0; i < num_images; i++)
    {
        if (images[i].empty()){	// check for failed read
            num_images = 0;
            qDebug() << "Cannot open image " << i << "!" << endl;
        }
    }
    
    imgdims = images[0].size();
    kernelsize = this->imgdims.height/100;
    if(kernelsize % 2 == 0)
        kernelsize++;
    qDebug() << "kernelsize = " << kernelsize;
    kernel = Size(kernelsize,kernelsize);
}

ImageManager::~ImageManager()
{
    delete [] images;
}
