#include "loadwindow.h"
#include "ui_loadwindow.h"

#include <QFileDialog>
#include "foregroundextract.h"
#include "alignwindow.h"

LoadWindow::LoadWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LoadWindow)
{
    ui->setupUi(this);
}

void LoadWindow::proceed(){
    
    AlignWindow* aligner = new AlignWindow(manager);
    aligner->show();
    this->close();
}

void LoadWindow::handleExampleSelection(){

    manager = new ImageManager;
    if(manager->num_images > 1)
        proceed();
}

void LoadWindow::handleFolderSelection(){
    
    QFileDialog dir_selector(this);
    dir_selector.setFileMode(QFileDialog::ExistingFile);
    dir_selector.setOption(QFileDialog::ShowDirsOnly, false);
    
    QStringList list;
    if(dir_selector.exec())
        list = dir_selector.selectedFiles();
    
    if(!list.isEmpty()){
        QString dir = list.at(0);
        manager = new ImageManager(dir);
        if(manager->num_images > 1)
            proceed();
    }
}

void LoadWindow::handleFileSelection(){

    QStringList filenames = QFileDialog::getOpenFileNames(this,
                                                          "Select at least two images to merge",
                                                          default_dir,
                                                          "Images (*.jpg)");
    if(filenames.size() < 2){
        ui->error_label->setText("You must select at least\ntwo images to merge.");
        return;
    }

    manager = new ImageManager(filenames);
    if(manager->num_images > 1)
        proceed();
}

void handleCustomSelection();

LoadWindow::~LoadWindow()
{
    delete ui;
}
