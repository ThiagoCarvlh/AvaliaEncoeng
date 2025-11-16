#include <QApplication>
#include "janelaprincipal.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/img/img/Logo.jpg"));
    JanelaPrincipal w;
    w.setMinimumSize(900, 600);
    w.show();
    return a.exec();
}
