#include "mainwindow.h"
#include <QApplication>
#include "glview.h"

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);

  //glView win;
  //win.show();
  //win.showFullScreen();

  MainWindow w;
  w.show();

  return a.exec();
}
