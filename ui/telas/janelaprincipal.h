// ui/telas/janelaprincipal.h
#pragma once
#include <QMainWindow>

class PaginaProjetos;
class PaginaAvaliadores;
class PaginaNotas;
class PaginaFichas;  // ← ADICIONAR

namespace Ui { class JanelaPrincipal; }

class JanelaPrincipal : public QMainWindow {
    Q_OBJECT
public:
    explicit JanelaPrincipal(QWidget *parent = nullptr);
    ~JanelaPrincipal();

private:
    Ui::JanelaPrincipal *ui;
    PaginaProjetos*     m_pagProjetos{nullptr};
    PaginaAvaliadores*  m_pagAvaliadores{nullptr};
    PaginaNotas*        m_pagNotas{nullptr};
    PaginaFichas*       m_pagFichas{nullptr};  // ← ADICIONAR
};
