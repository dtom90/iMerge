#include "loadwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    LoadWindow m;
    m.show();
    
    return a.exec();
}
