/********************************************************************************
** Form generated from reading UI file 'paginafichas.ui'
**
** Created by: Qt User Interface Compiler version 6.9.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_PAGINAFICHAS_H
#define UI_PAGINAFICHAS_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_PaginaFichas
{
public:
    QVBoxLayout *verticalLayout;

    void setupUi(QWidget *PaginaFichas)
    {
        if (PaginaFichas->objectName().isEmpty())
            PaginaFichas->setObjectName("PaginaFichas");
        PaginaFichas->resize(800, 600);
        verticalLayout = new QVBoxLayout(PaginaFichas);
        verticalLayout->setSpacing(0);
        verticalLayout->setObjectName("verticalLayout");
        verticalLayout->setContentsMargins(0, 0, 0, 0);

        retranslateUi(PaginaFichas);

        QMetaObject::connectSlotsByName(PaginaFichas);
    } // setupUi

    void retranslateUi(QWidget *PaginaFichas)
    {
        PaginaFichas->setWindowTitle(QCoreApplication::translate("PaginaFichas", "Fichas de Avalia\303\247\303\243o", nullptr));
    } // retranslateUi

};

namespace Ui {
    class PaginaFichas: public Ui_PaginaFichas {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_PAGINAFICHAS_H
