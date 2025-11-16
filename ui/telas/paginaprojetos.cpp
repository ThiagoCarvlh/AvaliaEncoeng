#include "paginaprojetos.h"
#include "ui_paginaprojetos.h"

#include <QTableView>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QPushButton>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QLabel>
#include <QFileDialog>
#include <QAbstractItemView>
#include <QFont>
#include <QComboBox>
#include <QGroupBox>
#include <QFormLayout>
#include <QDialog>
#include <QDialogButtonBox>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QSortFilterProxyModel>

// ================== Filtro para busca + categoria (Projetos) ==================

class ProjetoFilterModel : public QSortFilterProxyModel {
public:
    explicit ProjetoFilterModel(QObject* parent = nullptr)
        : QSortFilterProxyModel(parent) {}

    void setNomeFiltro(const QString& n) {
        m_nomeFiltro = n.trimmed();
        invalidateFilter();
    }

    void setCategoriaFiltro(const QString& c) {
        m_categoriaFiltro = c.trimmed();
        invalidateFilter();
    }

protected:
    bool filterAcceptsRow(int source_row,
                          const QModelIndex& source_parent) const override
    {
        if (!sourceModel()) return true;

        const QModelIndex idxNome = sourceModel()->index(source_row, 1, source_parent);
        const QModelIndex idxCat  = sourceModel()->index(source_row, 4, source_parent);

        const QString nome = sourceModel()->data(idxNome).toString();
        const QString cat  = sourceModel()->data(idxCat).toString();

        const bool okNome = m_nomeFiltro.isEmpty()
                            || nome.contains(m_nomeFiltro, Qt::CaseInsensitive);
        const bool okCat  = m_categoriaFiltro.isEmpty()
                           || cat.contains(m_categoriaFiltro, Qt::CaseInsensitive);

        return okNome && okCat;
    }

    // ordenar ID como número, não como texto
    bool lessThan(const QModelIndex& left,
                  const QModelIndex& right) const override
    {
        if (left.column() == 0 && right.column() == 0 && sourceModel()) {
            bool ok1 = false, ok2 = false;
            int v1 = sourceModel()->data(left,  Qt::DisplayRole).toInt(&ok1);
            int v2 = sourceModel()->data(right, Qt::DisplayRole).toInt(&ok2);

            if (ok1 && ok2)
                return v1 < v2;
        }
        return QSortFilterProxyModel::lessThan(left, right);
    }

private:
    QString m_nomeFiltro;
    QString m_categoriaFiltro;
};


// ================== Helpers internos (diálogo de projeto) ==================

namespace {

bool nomeValido(const QString& n) {
    return n.trimmed().size() >= 3;
}

bool descValida(const QString& d) {
    return d.trimmed().size() >= 5;
}

bool responsavelValido(const QString& r) {
    return r.trimmed().size() >= 3;
}

struct ProjetoData {
    QString nome;
    QString descricao;
    QString responsavel;
    QString categoria; // "Técnico - Automação Industrial" etc.
};

static void preencherCategorias(QComboBox* combo, bool tecnico) {
    combo->clear();
    if (tecnico) {
        // ----- CURSOS TÉCNICOS -----
        combo->addItem("Automação Industrial");
        combo->addItem("Eletrônica");
        combo->addItem("Eletrotécnica");
        combo->addItem("Informática (com ênfase em Programação)");
        combo->addItem("Mecânica");
        combo->addItem("Mecatrônica");
        combo->addItem("Qualidade");
        combo->addItem("Logística");
        combo->addItem("Segurança do Trabalho");
    } else {
        // ----- CURSOS DE GRADUAÇÃO -----
        combo->addItem("Administração");
        combo->addItem("Ciência da Computação");
        combo->addItem("Engenharia da Computação");
        combo->addItem("Engenharia de Produção");
        combo->addItem("Engenharia de Software");
        combo->addItem("Engenharia Elétrica");
        combo->addItem("Engenharia Mecânica");
    }
}

static bool abrirDialogoProjeto(QWidget* parent, ProjetoData& data, bool edicao)
{
    QDialog dlg(parent);
    dlg.setWindowTitle(edicao ? "Editar Projeto" : "Novo Projeto");
    dlg.setModal(true);

    // herda o mesmo tema da tela de Projetos
    dlg.setStyleSheet(parent->styleSheet());

    auto* mainLayout = new QVBoxLayout(&dlg);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(24, 24, 24, 24);

    auto* titulo = new QLabel(edicao ? "  Editar Projeto" : "  Novo Projeto", &dlg);
    QFont f = titulo->font();
    f.setPointSize(f.pointSize() + 4);
    f.setBold(true);
    titulo->setFont(f);
    titulo->setStyleSheet("color: #00D4FF; padding-bottom: 8px;");
    mainLayout->addWidget(titulo);

    // ---- Dados do projeto ----
    auto* boxDados = new QGroupBox(" Dados do Projeto", &dlg);
    auto* formDados = new QFormLayout(boxDados);
    formDados->setSpacing(12);
    formDados->setContentsMargins(16, 20, 16, 16);

    auto* edNome  = new QLineEdit(data.nome, &dlg);
    auto* edDesc  = new QLineEdit(data.descricao, &dlg);
    auto* edResp  = new QLineEdit(data.responsavel, &dlg);

    edNome->setPlaceholderText("Digite o nome do projeto");
    edDesc->setPlaceholderText("Breve descrição do projeto");
    edResp->setPlaceholderText("Nome do responsável");

    auto* lblNomeStatus = new QLabel(" ", &dlg);
    auto* lblDescStatus = new QLabel(" ", &dlg);
    auto* lblRespStatus = new QLabel(" ", &dlg);

    lblNomeStatus->setStyleSheet("font-size: 11px; padding-left: 4px;");
    lblDescStatus->setStyleSheet("font-size: 11px; padding-left: 4px;");
    lblRespStatus->setStyleSheet("font-size: 11px; padding-left: 4px;");

    formDados->addRow("Nome:", edNome);
    formDados->addRow("", lblNomeStatus);
    formDados->addRow("Descrição:", edDesc);
    formDados->addRow("", lblDescStatus);
    formDados->addRow("Responsável:", edResp);
    formDados->addRow("", lblRespStatus);

    mainLayout->addWidget(boxDados);

    // ---- Categoria / Especialidade ----
    auto* boxCat = new QGroupBox(" Categoria e Especialização", &dlg);
    auto* layCat = new QVBoxLayout(boxCat);
    layCat->setSpacing(12);
    layCat->setContentsMargins(16, 20, 16, 16);

    auto* linhaRadios = new QHBoxLayout();
    auto* rbTec  = new QRadioButton("Técnico", &dlg);
    auto* rbGrad = new QRadioButton("Graduação", &dlg);
    linhaRadios->addWidget(rbTec);
    linhaRadios->addWidget(rbGrad);
    linhaRadios->addStretch();
    layCat->addLayout(linhaRadios);

    auto* cbArea = new QComboBox(&dlg);
    layCat->addWidget(cbArea);

    // Estado inicial
    if (data.categoria.startsWith("Técnico"))
        rbTec->setChecked(true);
    else if (data.categoria.startsWith("Graduação"))
        rbGrad->setChecked(true);
    else
        rbTec->setChecked(true);

    preencherCategorias(cbArea, rbTec->isChecked());

    if (!data.categoria.isEmpty()) {
        const int sep = data.categoria.indexOf('-');
        const QString area = (sep >= 0)
                                 ? data.categoria.mid(sep + 1).trimmed()
                                 : data.categoria.trimmed();
        const int idx = cbArea->findText(area, Qt::MatchContains);
        if (idx >= 0) cbArea->setCurrentIndex(idx);
    }

    QObject::connect(rbTec, &QRadioButton::toggled, &dlg,
                     [&](bool checked){ if (checked) preencherCategorias(cbArea, true); });
    QObject::connect(rbGrad, &QRadioButton::toggled, &dlg,
                     [&](bool checked){ if (checked) preencherCategorias(cbArea, false); });

    mainLayout->addWidget(boxCat);

    // ---- Botões ----
    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        Qt::Horizontal, &dlg);
    auto* btnOk = buttons->button(QDialogButtonBox::Ok);
    auto* btnCancel = buttons->button(QDialogButtonBox::Cancel);

    if (btnOk) {
        btnOk->setText(" Salvar");
    }
    if (btnCancel) {
        btnCancel->setText("Cancelar");
    }

    mainLayout->addWidget(buttons);

    QObject::connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    // ---- Estilo das mensagens de status ----
    auto setOk = [](QLabel* lbl, const QString& txt){
        lbl->setText(QString("✓ %1").arg(txt));
        lbl->setStyleSheet("color: #00FF88; font-size: 11px; padding-left: 4px;");
    };
    auto setErr = [](QLabel* lbl, const QString& txt){
        lbl->setText(QString("✗ %1").arg(txt));
        lbl->setStyleSheet("color: #FF6B9D; font-size: 11px; padding-left: 4px;");
    };

    auto atualizarValidacao = [&]() {
        const bool okNome = nomeValido(edNome->text());
        const bool okDesc = descValida(edDesc->text());
        const bool okResp = responsavelValido(edResp->text());

        if (okNome)  setOk(lblNomeStatus,  "Nome válido");
        else         setErr(lblNomeStatus, "Nome muito curto (mínimo 3 caracteres)");

        if (okDesc)  setOk(lblDescStatus,  "Descrição válida");
        else         setErr(lblDescStatus, "Descrição muito curta (mínimo 5 caracteres)");

        if (okResp)  setOk(lblRespStatus,  "Responsável válido");
        else         setErr(lblRespStatus, "Responsável muito curto (mínimo 3 caracteres)");

        if (btnOk)
            btnOk->setEnabled(okNome && okDesc && okResp);
    };

    QObject::connect(edNome, &QLineEdit::textChanged,
                     &dlg, [&](const QString&){ atualizarValidacao(); });
    QObject::connect(edDesc, &QLineEdit::textChanged,
                     &dlg, [&](const QString&){ atualizarValidacao(); });
    QObject::connect(edResp, &QLineEdit::textChanged,
                     &dlg, [&](const QString&){ atualizarValidacao(); });

    atualizarValidacao();

    if (dlg.exec() == QDialog::Accepted) {
        data.nome        = edNome->text().trimmed();
        data.descricao   = edDesc->text().trimmed();
        data.responsavel = edResp->text().trimmed();
        const QString tipo = rbTec->isChecked() ? "Técnico" : "Graduação";
        const QString area = cbArea->currentText();
        data.categoria = QString("%1 - %2").arg(tipo, area);
        return true;
    }
    return false;
}

} // namespace


// ================== CONSTRUTOR / SETUP ==================

PaginaProjetos::PaginaProjetos(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::PaginaProjetos)
    , m_table(new QTableView(this))
    , m_model(new QStandardItemModel(0, 5, this)) // ID, Nome, Descrição, Responsável, Área/Categoria
    , m_filter(new ProjetoFilterModel(this))
    , m_btnNovo(new QPushButton(" Adicionar", this))
    , m_btnEditar(new QPushButton(" Editar", this))
    , m_btnRemover(new QPushButton(" Excluir", this))
    , m_btnRecarregar(new QPushButton(" Recarregar", this))
    , m_btnExportCsv(new QPushButton(" Exportar CSV", this))
    , m_labelTotal(new QLabel(this))
    , m_editBusca(new QLineEdit(this))
    , m_comboCategoria(new QComboBox(this))
{
    ui->setupUi(this);

    // para o stylesheet pegar
    this->setObjectName("paginaprojetos");

    // ===== Mesmo tema da tela de Avaliadores =====
    this->setStyleSheet(R"(
        QWidget#paginaprojetos {
           background-color: #05070d;
           background-image: url(:/img/img/Logo.jpg);
           background-position: center bottom;
           background-repeat: no-repeat;
        }

        QLabel#titulo {
            color: #00D4FF;
            font-size: 24px;
            font-weight: bold;
            padding: 8px 0px;
        }

        QTableView {
            background-color: transparent;
            alternate-background-color: rgba(32, 42, 60, 0.85);
            gridline-color: #2a3f5f;
            selection-color: #FFFFFF;
            border: 2px solid #2a3f5f;
            border-radius: 10px;
            color: #E0E0E0;
        }

        QTableView::viewport {
            background-color: rgba(26, 35, 50, 0.85);
            background-image: url(:/img/img/Logo.jpg);
            background-position: center;
            background-repeat: no-repeat;
        }

        QTableView::item {
            padding: 10px 8px;
            border: none;
        }

        QTableView::item:hover {
            background-color: rgba(0, 168, 204, 0.15);
        }

        QTableView::item:selected {
            background-color: rgba(0, 220, 255, 0.35);
            border: none;
            color: #FFFFFF;
        }

        QHeaderView::section {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #1a2840, stop:1 #15202f);
            color: #00D4FF;
            padding: 12px 8px;
            border: none;
            border-bottom: 2px solid #00A8CC;
            border-right: 1px solid #2a3f5f;
            font-weight: bold;
            font-size: 13px;
        }

        QHeaderView::section:first {
            border-top-left-radius: 8px;
        }

        QHeaderView::section:last {
            border-top-right-radius: 8px;
            border-right: none;
        }

        QLineEdit {
            background-color: rgba(26, 35, 50, 0.9);
            border: 2px solid #2a3f5f;
            border-radius: 8px;
            padding: 10px 15px;
            color: #FFFFFF;
            selection-background-color: #00D4FF;
        }

        QLineEdit:focus {
            border: 2px solid #00D4FF;
            background-color: rgba(26, 35, 50, 1);
        }

        QComboBox {
            background-color: rgba(26, 35, 50, 0.9);
            border: 2px solid #2a3f5f;
            border-radius: 0px;
            padding: 10px 30px 10px 15px;
            color: #FFFFFF;
            min-width: 150px;
        }

        QComboBox:focus {
            border: 2px solid #00D4FF;
        }

        QComboBox::drop-down {
            border: none;
            width: 20px;
        }

        QComboBox::down-arrow {
            image: url(:/img/img/Seta.png);
            width: 18px;
            height: 18px;
            margin-right: 6px;
            background: transparent;
            border: none;
        }

        QComboBox QAbstractItemView {
            background-color: #1a2332;
            border: 2px solid #00D4FF;
            border-radius: 6px;
            selection-background-color: #00A8CC;
            color: #FFFFFF;
            padding: 4px;
        }

        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #00A8CC, stop:1 #0088FF);
            border: none;
            border-radius: 0px;
            padding: 12px 24px;
            color: #FFFFFF;
            font-weight: bold;
            font-size: 13px;
        }

        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #00D4FF, stop:1 #00A8FF);
        }

        QPushButton:pressed {
            background: #006688;
        }

        QPushButton#btnDangerProjeto {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #D946A6, stop:1 #FF6B9D);
        }

        QPushButton#btnDangerProjeto:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #E956B6, stop:1 #FF7BAD);
        }

        QPushButton#btnSecondaryProjeto {
            background: rgba(42, 63, 95, 0.8);
            border: 2px solid #2a3f5f;
        }

        QPushButton#btnSecondaryProjeto:hover {
            background: rgba(52, 73, 105, 1);
            border: 2px solid #3a4f6f;
        }

        QLabel#labelTotalProjetos {
            color: #00D4FF;
            font-size: 13px;
            font-weight: bold;
            padding: 8px 0px;
        }

        QLabel#labelBuscar, QLabel#labelCategoria {
            color: #A0A0A0;
            font-weight: bold;
            font-size: 12px;
        }

        /* diálogo / groupbox / radios / etc herdados pelos QDialogs */
        QDialog {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #0a0e1a, stop:1 #1a1f2e);
        }
        QLabel {
            color: #E0E0E0;
        }
        QGroupBox {
            border: 2px solid #2a3f5f;
            border-radius: 8px;
            margin-top: 12px;
            padding-top: 12px;
            color: #00D4FF;
            font-weight: bold;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            padding: 4px 10px;
            background: #1a2332;
            border-radius: 4px;
        }
        QRadioButton {
            color: #E0E0E0;
            spacing: 8px;
        }
        QRadioButton::indicator {
            width: 18px;
            height: 18px;
            border-radius: 9px;
            border: 2px solid #2a3f5f;
            background: #1a2332;
        }
        QRadioButton::indicator:checked {
            background: qradialgradient(cx:0.5, cy:0.5, radius:0.5,
                fx:0.5, fy:0.5, stop:0 #00D4FF, stop:0.7 #0088FF);
            border: 2px solid #00D4FF;
        }
    )");

    m_btnRemover->setObjectName("btnDangerProjeto");
    m_btnRecarregar->setObjectName("btnSecondaryProjeto");
    m_labelTotal->setObjectName("labelTotalProjetos");

    auto* root = ui->verticalLayout;
    root->setContentsMargins(120, 40, 120, 40);
    root->setSpacing(20);

    // ===== Header: título + Adicionar + Exportar CSV =====
    auto* headerLayout = new QHBoxLayout();
    auto* titulo = new QLabel(" Projetos Cadastrados", this);
    titulo->setObjectName("titulo");

    headerLayout->addWidget(titulo);
    headerLayout->addStretch();
    headerLayout->addWidget(m_btnNovo);
    headerLayout->addWidget(m_btnExportCsv);
    root->addLayout(headerLayout);

    // ===== Linha de filtros: Buscar + Categoria =====
    auto* filterLayout = new QHBoxLayout();
    filterLayout->setSpacing(12);

    auto* lblBuscar = new QLabel(" Buscar:", this);
    lblBuscar->setObjectName("labelBuscar");
    filterLayout->addWidget(lblBuscar);

    m_editBusca->setPlaceholderText("Digite o nome do projeto...");
    m_editBusca->setMinimumWidth(250);
    filterLayout->addWidget(m_editBusca);

    filterLayout->addSpacing(20);

    auto* lblCat = new QLabel(" Categoria:", this);
    lblCat->setObjectName("labelCategoria");
    filterLayout->addWidget(lblCat);

    m_comboCategoria->addItem("Todos");
    m_comboCategoria->addItem("Técnico");
    m_comboCategoria->addItem("Graduação");
    filterLayout->addWidget(m_comboCategoria);
    filterLayout->addStretch();

    root->addLayout(filterLayout);

    // ===== Tabela =====
    root->addWidget(m_table, 1);

    // ===== Linha de botões: Editar / Excluir / Recarregar =====
    auto* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(12);
    btnLayout->addWidget(m_btnEditar);
    btnLayout->addWidget(m_btnRemover);
    btnLayout->addStretch();
    btnLayout->addWidget(m_btnRecarregar);
    root->addLayout(btnLayout);

    // ===== Rodapé: total de projetos =====
    root->addWidget(m_labelTotal);

    // Modelo + filtro
    m_model->setHorizontalHeaderLabels(
        {"ID","Nome","Descrição","Responsável","Área/Categoria"});
    m_filter->setSourceModel(m_model);
    m_table->setModel(m_filter);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->horizontalHeader()->setStretchLastSection(false);
    m_table->verticalHeader()->setVisible(false);
    m_table->setAlternatingRowColors(true);
    m_table->setSortingEnabled(true);
    m_table->sortByColumn(0, Qt::AscendingOrder);
    m_table->setFocusPolicy(Qt::NoFocus);

    // Ajuste de largura das colunas (igual Avaliadores)
    auto header = m_table->horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(2, QHeaderView::Stretch);
    header->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(4, QHeaderView::ResizeToContents);

    // Duplo clique = editar
    connect(m_table, &QTableView::doubleClicked,
            this, [this](const QModelIndex&) { onEditar(); });

    // Botões
    connect(m_btnNovo,       &QPushButton::clicked, this, &PaginaProjetos::onNovo);
    connect(m_btnEditar,     &QPushButton::clicked, this, &PaginaProjetos::onEditar);
    connect(m_btnRemover,    &QPushButton::clicked, this, &PaginaProjetos::onRemover);
    connect(m_btnRecarregar, &QPushButton::clicked, this, &PaginaProjetos::onRecarregar);
    connect(m_btnExportCsv,  &QPushButton::clicked, this, &PaginaProjetos::onExportCsv);

    // Filtros
    connect(m_editBusca, &QLineEdit::textChanged,
            this, &PaginaProjetos::onBuscaChanged);
    connect(m_comboCategoria,
            static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &PaginaProjetos::onCategoriaChanged);

    // Carrega dados e atualiza contador
    carregarDoArquivo();
    atualizarTotal();
}

PaginaProjetos::~PaginaProjetos() {
    delete ui;
}

// ================== HELPERS DE MODELO ==================

void PaginaProjetos::addProjeto(const QString& nome,
                                const QString& desc,
                                const QString& resp,
                                const QString& categoria)
{
    QList<QStandardItem*> row;
    auto idItem = new QStandardItem(QString::number(m_nextId++));
    idItem->setEditable(false);
    row << idItem
        << new QStandardItem(nome)
        << new QStandardItem(desc)
        << new QStandardItem(resp)
        << new QStandardItem(categoria);
    m_model->appendRow(row);
}

int PaginaProjetos::selectedRow() const {
    if (!m_table->model()) return -1;
    const QModelIndex proxyIdx = m_table->currentIndex();
    if (!proxyIdx.isValid()) return -1;
    const QModelIndex srcIdx = m_filter ? m_filter->mapToSource(proxyIdx) : proxyIdx;
    return srcIdx.row();
}

// ================== SLOTS: AÇÕES ==================

void PaginaProjetos::onNovo() {
    ProjetoData data;
    if (!abrirDialogoProjeto(this, data, false))
        return;

    addProjeto(data.nome, data.descricao, data.responsavel, data.categoria);
    salvarNoArquivo();
    atualizarTotal();
}

void PaginaProjetos::onEditar() {
    const int r = selectedRow();
    if (r < 0) {
        QMessageBox::information(this, "Editar Projeto",
                                 "Selecione um projeto.");
        return;
    }

    ProjetoData data;
    data.nome        = m_model->item(r,1)->text();
    data.descricao   = m_model->item(r,2)->text();
    data.responsavel = m_model->item(r,3)->text();
    data.categoria   = m_model->item(r,4)->text();

    if (!abrirDialogoProjeto(this, data, true))
        return;

    m_model->item(r,1)->setText(data.nome);
    m_model->item(r,2)->setText(data.descricao);
    m_model->item(r,3)->setText(data.responsavel);
    m_model->item(r,4)->setText(data.categoria);

    salvarNoArquivo();
    atualizarTotal();
}

void PaginaProjetos::onRemover() {
    const int r = selectedRow();
    if (r < 0) {
        QMessageBox::information(this, "Excluir Projeto",
                                 "Selecione um projeto.");
        return;
    }

    const QString id   = m_model->item(r,0)->text();
    const QString nome = m_model->item(r,1)->text();
    const QString resp = m_model->item(r,3)->text();

    QString texto = QString(
                        "Projeto encontrado:\n\n"
                        "ID: %1\n"
                        "Nome: %2\n"
                        "Responsável: %3\n\n"
                        "Esta ação não pode ser desfeita.\n\n"
                        "Deseja realmente excluir?")
                        .arg(id, nome, resp);

    QMessageBox box(this);
    box.setWindowTitle("Confirmar Exclusão");
    box.setIcon(QMessageBox::Warning);
    box.setText(texto);
    box.setStandardButtons(QMessageBox::Cancel | QMessageBox::Yes);

    if (auto btnYes = box.button(QMessageBox::Yes)) {
        btnYes->setText("Excluir");
    }

    box.setStyleSheet(R"(
        QMessageBox {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #0a0e1a, stop:1 #1a1f2e);
        }
        QLabel {
            color: #E0E0E0;
            padding: 10px;
        }
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #00A8CC, stop:1 #0088FF);
            border-radius: 6px;
            padding: 8px 20px;
            color: #FFFFFF;
            font-weight: bold;
        }
    )");

    if (box.exec() == QMessageBox::Yes) {
        m_model->removeRow(r);
        salvarNoArquivo();
        atualizarTotal();
    }
}

void PaginaProjetos::onRecarregar() {
    carregarDoArquivo();
    atualizarTotal();
}

// ================== PERSISTÊNCIA (projetos.txt) ==================

bool PaginaProjetos::salvarNoArquivo() const {
    QFile f(m_arquivo);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(nullptr, "Salvar Projetos",
                             "Não foi possível abrir o arquivo de projetos para escrita.");
        return false;
    }

    QTextStream out(&f);
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    out.setCodec("UTF-8");
#endif

    for (int r = 0; r < m_model->rowCount(); ++r) {
        QStringList cols;
        for (int c = 0; c < m_model->columnCount(); ++c) {
            QString s = m_model->item(r, c)->text();
            s.replace(';', ','); // segurança
            cols << s;
        }
        out << cols.join(';') << '\n';
    }

    return true;
}

bool PaginaProjetos::carregarDoArquivo() {
    QFile f(m_arquivo);
    if (!f.exists()) {
        m_model->removeRows(0, m_model->rowCount());
        m_nextId = 1;
        return true;
    }
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(nullptr, "Carregar Projetos",
                             "Não foi possível abrir o arquivo de projetos para leitura.");
        return false;
    }

    m_model->removeRows(0, m_model->rowCount());

    QTextStream in(&f);
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    in.setCodec("UTF-8");
#endif

    while (!in.atEnd()) {
        const QString line = in.readLine();
        if (line.trimmed().isEmpty()) continue;
        const QStringList p = line.split(';');
        if (p.size() < 4) continue;

        QList<QStandardItem*> row;
        auto idItem = new QStandardItem(p[0]);
        idItem->setEditable(false);
        row << idItem
            << new QStandardItem(p.value(1))
            << new QStandardItem(p.value(2))
            << new QStandardItem(p.value(3));

        if (p.size() >= 5)
            row << new QStandardItem(p[4]);
        else
            row << new QStandardItem("");

        m_model->appendRow(row);
    }

    recomputarNextId();
    return true;
}

void PaginaProjetos::recomputarNextId() {
    int maxId = 0;
    for (int r = 0; r < m_model->rowCount(); ++r) {
        bool ok = false;
        int id = m_model->item(r,0)->text().toInt(&ok);
        if (ok && id > maxId) maxId = id;
    }
    m_nextId = maxId + 1;
}

// ================== FOOTER ==================

void PaginaProjetos::atualizarTotal() {
    const int total = m_filter ? m_filter->rowCount() : m_model->rowCount();
    if (total == 1)
        m_labelTotal->setText(" 1 projeto encontrado");
    else
        m_labelTotal->setText(QString(" %1 projetos encontrados").arg(total));
}

// ================== EXPORTAR CSV ==================

void PaginaProjetos::onExportCsv() {
    QString filename = QFileDialog::getSaveFileName(
        this,
        "Exportar projetos para CSV",
        "projetos.csv",
        "Arquivos CSV (*.csv);;Todos os arquivos (*.*)"
        );

    if (filename.isEmpty())
        return;

    QFile f(filename);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Exportar CSV",
                             "Não foi possível abrir o arquivo para escrita.");
        return;
    }

    QTextStream out(&f);
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    out.setCodec("UTF-8");
#endif

    out << "ID;Nome;Descricao;Responsavel;Categoria\n";

    for (int r = 0; r < m_model->rowCount(); ++r) {
        QStringList cols;
        for (int c = 0; c < m_model->columnCount(); ++c) {
            QString s = m_model->item(r, c)->text();
            s.replace(';', ',');
            cols << s;
        }
        out << cols.join(';') << '\n';
    }

    QMessageBox::information(this, "Exportar CSV",
                             "Projetos exportados com sucesso.");
}

// ================== FILTROS ==================

void PaginaProjetos::onBuscaChanged(const QString& texto) {
    if (m_filter) {
        m_filter->setNomeFiltro(texto);
        atualizarTotal();
    }
}

void PaginaProjetos::onCategoriaChanged(int index) {
    if (!m_filter) return;

    QString filtro;
    if (index == 1)      filtro = "Técnico";
    else if (index == 2) filtro = "Graduação";
    else                 filtro.clear();

    m_filter->setCategoriaFiltro(filtro);
    atualizarTotal();
}
