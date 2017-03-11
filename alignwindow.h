#ifndef ALIGNWINDOW_H
#define ALIGNWINDOW_H

#include <QWidget>
#include "imagemanager.h"

namespace Ui {
class AlignWindow;
}

class AlignWindow : public QWidget
{
    Q_OBJECT
    
public:
    explicit AlignWindow(ImageManager* my_manager, QWidget *parent = 0);
    ~AlignWindow();
    
public slots:
    void showNewImage(int imnum);
    void alignAllImages();
    void proceed();

protected:
    void resizeEvent(QResizeEvent* event);
    
private:
    Ui::AlignWindow *ui;
    ImageManager* manager;
    
    // alignment variables
    int imgnum0, imgnum1;
    Size origdims, origdims_T;
    Mat bw0, bw1;
    Mat T, I, T_coords, P_coords;
    Mat* origT;
    Mat VTx, VTy, SDa, SDb, H;
    
    // output variables
    Size portalsize;
    QSize qportalsize;
    
    // alignment methods
    vector<Mat> alignEachImage();
    Mat alignImagePair();
    void precomputeMatrices(double scale);
    void getTemplateCoords(Size oirgsize, Size croppedsize);
    void getPortalCoords();
    
    // iteration methods
    Mat refineWarp(Mat W);
    Mat* warpTarget(Mat W, Mat *target, Mat *Coords);
    Mat calcResid(Mat *warped_img);
    
    // output methods
    void warpOriginals(vector<Mat> warps);
    void warpOrig(Mat W, int imgnum, Mat* Coords);
    void writeImages();
    void showCurrentWarp(Mat W);
};

#endif // ALIGNWINDOW_H
