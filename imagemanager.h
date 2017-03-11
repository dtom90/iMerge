#ifndef IMAGEMANAGER_H
#define IMAGEMANAGER_H

#include <QDirIterator>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#define shaky_lounge_path "C:\\Users\\David\\Dropbox\\M. Eng\\Project\\Images\\ECE_lounge\\"
#define slope_path "C:\\Users\\David\\Dropbox\\M. Eng\\Project\\Images\\Slope\\"
#define lot_path "C:\\Users\\David\\Dropbox\\M. Eng\\Project\\Images\\Parkinglot\\"
#define default_path slope_path

#define max_pix 600

using namespace cv;

class ImageManager {

public:
    
    QString directory;
    int num_images;
    
    Size imgdims;
    Size kernel;
    int kernelsize;
    Mat *images;
    
    ImageManager(void);
    ImageManager(QString path);
    ImageManager(QStringList filenames);
    ~ImageManager();
    
    void preprocessImages(void);

private:
    QStringList getImageFiles(QString dir);
    void loadImages(QStringList filenames);
};

#endif // IMAGEMANAGER_H
