// ui/telas/janelaprincipal.cpp
#include "janelaprincipal.h"
#include "ui_janelaprincipal.h"

#include "paginaprojetos.h"
#include "paginaavaliadores.h"
#include "paginanotas.h"
#include "paginafichas.h"  // ← ADICIONAR

#include <QToolBar>
#include <QAction>
#include <QStackedWidget>
#include <QMessageBox>

JanelaPrincipal::JanelaPrincipal(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::JanelaPrincipal)
{
    ui->setupUi(this);
    setWindowTitle("Sistema de Avaliações");

    // Localiza o QStackedWidget
    QStackedWidget* stack = findChild<QStackedWidget*>("stack");
    if (!stack) stack = findChild<QStackedWidget*>("stackedWidget");
    if (!stack) {
        QMessageBox::critical(this, "Erro de UI",
                              "Não encontrei um QStackedWidget chamado 'stack' ou 'stackedWidget'.");
        return;
    }

    // Cria as páginas
    m_pagProjetos    = new PaginaProjetos(this);
    m_pagAvaliadores = new PaginaAvaliadores(this);
    m_pagNotas       = new PaginaNotas(this);
    m_pagFichas      = new PaginaFichas(this);  // ← ADICIONAR

    // Adiciona ao stacked
    stack->addWidget(m_pagProjetos);
    stack->addWidget(m_pagAvaliadores);
    stack->addWidget(m_pagNotas);
    stack->addWidget(m_pagFichas);  // ← ADICIONAR
    stack->setCurrentWidget(m_pagProjetos);

    // Toolbar de navegação
    auto tb      = addToolBar("Navegar");
    auto acProj  = tb->addAction("📁 Projetos");
    auto acAval  = tb->addAction("👥 Avaliadores");
    auto acFichas= tb->addAction("📋 Fichas");  // ← ADICIONAR
    auto acNotas = tb->addAction("✍️ Notas");

    connect(acProj,   &QAction::triggered, this, [stack, this]{ stack->setCurrentWidget(m_pagProjetos); });
    connect(acAval,   &QAction::triggered, this, [stack, this]{ stack->setCurrentWidget(m_pagAvaliadores); });
    connect(acFichas, &QAction::triggered, this, [stack, this]{ stack->setCurrentWidget(m_pagFichas); });  // ← ADICIONAR
    connect(acNotas,  &QAction::triggered, this, [stack, this]{ stack->setCurrentWidget(m_pagNotas); });
}

JanelaPrincipal::~JanelaPrincipal() { delete ui; }
