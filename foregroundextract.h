#ifndef FOREGROUNDEXTRACT_H
#define FOREGROUNDEXTRACT_H

#include <QWidget>
#include "imagemanager.h"

namespace Ui {
class ForegroundExtract;
}

class ForegroundExtract : public QWidget
{
    Q_OBJECT
    
public:
    explicit ForegroundExtract(ImageManager* my_manager, QWidget *parent = 0);
    ~ForegroundExtract();
    
public slots:
    void showImagePair(int imnum);
    void dumpImages();
    void proceed();
    
protected:
    void resizeEvent(QResizeEvent* event);
    
private:
    void extractObjects(int pair, bool display);
    void cleanObjects(int imnum);
    
    // interface variables
    Ui::ForegroundExtract *ui;
    QSize qportalsize;
    Size portalsize;
    
    // image sequence variables
    ImageManager* manager;
    int num_pairs;
    
    // object extraction variables
    vector<Mat> extractedObjects;
    vector<vector<Rect> > objectRects;
    
    int current_pair;
};

#endif // FOREGROUNDEXTRACT_H
