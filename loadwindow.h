#ifndef LOADWINDOW_H
#define LOADWINDOW_H

#include <QWidget>
#include "imagemanager.h"

#define default_dir "C:\\Users\\David\\Dropbox\\M. Eng\\Project\\Images\\"

namespace Ui {
class LoadWindow;
}

class LoadWindow : public QWidget
{
    Q_OBJECT
    
public:
    explicit LoadWindow(QWidget *parent = 0);
    ~LoadWindow();

public slots:
    void handleExampleSelection();
    void handleFolderSelection();
    void handleFileSelection();

private:
    Ui::LoadWindow *ui;
    ImageManager* manager;
    void proceed();
};

#endif // LOADWINDOW_H
