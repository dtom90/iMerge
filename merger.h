#ifndef MERGER_H
#define MERGER_H

#include <QWidget>
#include "imagemanager.h"

namespace Ui {
class Merger;
}

class Merger : public QWidget
{
    Q_OBJECT
    
public:
    explicit Merger(ImageManager *my_manager, vector<Mat> my_extractedObjects, QWidget *parent = 0);
    ~Merger();

public slots:
    void addAllObjects();
    void removeAllObjects();
    void customObjects();
    
protected:
    void resizeEvent(QResizeEvent* event);
    
private:
    Ui::Merger *ui;
    QSize qportalsize;
    Size portalsize;
    int current_img;
    
    ImageManager* manager;
    vector<Mat> extractedObjects;
    Mat cleanMerge;
    Mat mergedObjects;
    Mat finalImage;
    
    Mat cleanObjects(int imnum);
    void showMergedImage();
};

#endif // MERGER_H
