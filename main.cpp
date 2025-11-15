#include <QApplication>
#include "janelaprincipal.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    JanelaPrincipal w;
    w.setMinimumSize(900, 600);
    w.show();
    return a.exec();
}
