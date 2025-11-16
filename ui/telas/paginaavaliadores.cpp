#include "paginaavaliadores.h"
#include "ui_paginaavaliadores.h"

#include <QTableView>
#include <QStandardItemModel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QGroupBox>
#include <QFormLayout>
#include <QDialog>
#include <QDialogButtonBox>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QRegularExpression>
#include <QSortFilterProxyModel>
#include <QFileDialog>

// ====== Filtro para busca + categoria ======
class AvaliadorFilterModel : public QSortFilterProxyModel {
public:
    explicit AvaliadorFilterModel(QObject* parent = nullptr)
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

private:
    QString m_nomeFiltro;
    QString m_categoriaFiltro;
};

// ====== Helpers internos ======
namespace {

bool nomeValido(const QString& n) {
    return n.trimmed().size() >= 3;
}

bool emailValido(const QString& e) {
    if (e.trimmed().isEmpty()) return false;
    static const QRegularExpression rx(R"(^\S+@\S+\.\S+$)");
    return rx.match(e.trimmed()).hasMatch();
}

bool cpfValido(const QString& cpf) {
    QString num = cpf;
    num.remove(QRegularExpression("\\D"));
    if (num.size() != 11) return false;

    bool allEq = true;
    for (int i = 1; i < 11; ++i) {
        if (num[i] != num[0]) { allEq = false; break; }
    }
    if (allEq) return false;

    auto calcDigit = [](const QString& n, int len)->int {
        int sum = 0;
        for (int i = 0; i < len; ++i)
            sum += n[i].digitValue() * (len + 1 - i);
        int r = sum % 11;
        return (r < 2) ? 0 : 11 - r;
    };

    int d1 = calcDigit(num, 9);
    int d2 = calcDigit(num, 10);
    return d1 == num[9].digitValue() && d2 == num[10].digitValue();
}

struct AvaliadorData {
    QString nome;
    QString email;
    QString cpf;
    QString categoria;
    QString senha;
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

// ==== Dialog com design moderno ====
static bool abrirDialogoAvaliador(QWidget* parent, AvaliadorData& data, bool edicao)
{
    QDialog dlg(parent);
    dlg.setWindowTitle(edicao ? "Editar Avaliador" : "Novo Avaliador");
    dlg.setModal(true);
    dlg.setMinimumWidth(500);

    // Estilo moderno para o dialog
    dlg.setStyleSheet(R"(
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
        QLineEdit {
            background-color: #1a2332;
            border: 2px solid #2a3f5f;
            border-radius: 6px;
            padding: 8px 12px;
            color: #FFFFFF;
            selection-background-color: #00D4FF;
        }
        QLineEdit:focus {
            border: 2px solid #00D4FF;
        }
QComboBox {
    background-color: #1a2332;
    border: 2px solid #2a3f5f;
    border-radius: 6px;
    padding: 8px 30px 8px 12px;  /* espaço à direita pra seta */
    color: #FFFFFF;
}
QComboBox:focus {
    border: 2px solid #00D4FF;
}
QComboBox::drop-down {
    border: none;
    width: 24px;                 /* área da seta */
}
QComboBox::down-arrow {
    image: url(:/img/img/Seta.png);
    width: 16px;
    height: 16px;
    margin-right: 6px;
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
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #00A8CC, stop:1 #0088FF);
            border: none;
            border-radius: 6px;
            padding: 10px 24px;
            color: #FFFFFF;
            font-weight: bold;
            min-width: 100px;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #00D4FF, stop:1 #00A8FF);
        }
        QPushButton:pressed {
            background: #006688;
        }
        QPushButton:disabled {
            background: #2a3f5f;
            color: #666666;
        }
        QPushButton#btnCancel {
            background: #2a3f5f;
        }
        QPushButton#btnCancel:hover {
            background: #3a4f6f;
        }
    )");

    auto *mainLayout = new QVBoxLayout(&dlg);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(24, 24, 24, 24);

    auto *titulo = new QLabel(edicao ? "️  Editar Avaliador" : "  Novo Avaliador", &dlg);
    QFont f = titulo->font();
    f.setPointSize(f.pointSize() + 4);
    f.setBold(true);
    titulo->setFont(f);
    titulo->setStyleSheet("color: #00D4FF; padding-bottom: 8px;");
    mainLayout->addWidget(titulo);

    // --- Dados pessoais ---
    auto *boxDados = new QGroupBox(" Dados Pessoais", &dlg);
    auto *formDados = new QFormLayout(boxDados);
    formDados->setSpacing(12);
    formDados->setContentsMargins(16, 20, 16, 16);

    auto *edNome  = new QLineEdit(data.nome, &dlg);
    auto *edEmail = new QLineEdit(data.email, &dlg);
    auto *edCpf   = new QLineEdit(data.cpf, &dlg);
    auto *edSenha = new QLineEdit(data.senha, &dlg);
    edSenha->setEchoMode(QLineEdit::Password);

    edNome->setPlaceholderText("Digite o nome completo");
    edEmail->setPlaceholderText("exemplo@email.com");
    edCpf->setPlaceholderText("000.000.000-00");
    edSenha->setPlaceholderText("Senha de acesso");

    auto *lblNomeStatus  = new QLabel(" ", &dlg);
    auto *lblEmailStatus = new QLabel(" ", &dlg);
    auto *lblCpfStatus   = new QLabel(" ", &dlg);

    lblNomeStatus->setStyleSheet("font-size: 11px; padding-left: 4px;");
    lblEmailStatus->setStyleSheet("font-size: 11px; padding-left: 4px;");
    lblCpfStatus->setStyleSheet("font-size: 11px; padding-left: 4px;");

    formDados->addRow("Nome:", edNome);
    formDados->addRow("", lblNomeStatus);
    formDados->addRow("Email:", edEmail);
    formDados->addRow("", lblEmailStatus);
    formDados->addRow("CPF:", edCpf);
    formDados->addRow("", lblCpfStatus);
    formDados->addRow("Senha:", edSenha);

    mainLayout->addWidget(boxDados);

    // --- Categoria e área ---
    auto *boxCat = new QGroupBox(" Categoria e Especialização", &dlg);
    auto *layCat = new QVBoxLayout(boxCat);
    layCat->setSpacing(12);
    layCat->setContentsMargins(16, 20, 16, 16);

    auto *linhaRadios = new QHBoxLayout();
    auto *rbTec  = new QRadioButton("Técnico", &dlg);
    auto *rbGrad = new QRadioButton("Graduação", &dlg);
    linhaRadios->addWidget(rbTec);
    linhaRadios->addWidget(rbGrad);
    linhaRadios->addStretch();
    layCat->addLayout(linhaRadios);

    auto *cbArea = new QComboBox(&dlg);
    layCat->addWidget(cbArea);

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

    // --- Botões ---
    auto *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(12);

    auto *btnCancel = new QPushButton("Cancelar", &dlg);
    btnCancel->setObjectName("btnCancel");
    auto *btnSave = new QPushButton(edicao ? " Salvar" : " Adicionar", &dlg);

    btnLayout->addStretch();
    btnLayout->addWidget(btnCancel);
    btnLayout->addWidget(btnSave);

    mainLayout->addLayout(btnLayout);

    QObject::connect(btnSave, &QPushButton::clicked, &dlg, &QDialog::accept);
    QObject::connect(btnCancel, &QPushButton::clicked, &dlg, &QDialog::reject);

    // Validação em tempo real
    auto setOk = [](QLabel* lbl, const QString& txt){
        lbl->setText(QString("✓ %1").arg(txt));
        lbl->setStyleSheet("color: #00FF88; font-size: 11px; padding-left: 4px;");
    };
    auto setErr = [](QLabel* lbl, const QString& txt){
        lbl->setText(QString("✗ %1").arg(txt));
        lbl->setStyleSheet("color: #FF6B9D; font-size: 11px; padding-left: 4px;");
    };

    auto atualizarValidacao = [&]() {
        const bool okNome  = nomeValido(edNome->text());
        const bool okEmail = emailValido(edEmail->text());
        const bool okCpf   = cpfValido(edCpf->text());

        if (okNome)  setOk(lblNomeStatus,  "Nome válido");
        else         setErr(lblNomeStatus, "Nome muito curto (mínimo 3 caracteres)");

        if (okEmail) setOk(lblEmailStatus,"Email válido");
        else         setErr(lblEmailStatus,"Email inválido");

        if (okCpf)   setOk(lblCpfStatus,  "CPF válido");
        else         setErr(lblCpfStatus, "CPF inválido (11 dígitos)");

        btnSave->setEnabled(okNome && okEmail && okCpf);
    };

    QObject::connect(edNome,  &QLineEdit::textChanged, &dlg,
                     [&](const QString&){ atualizarValidacao(); });
    QObject::connect(edEmail, &QLineEdit::textChanged, &dlg,
                     [&](const QString&){ atualizarValidacao(); });
    QObject::connect(edCpf,   &QLineEdit::textChanged, &dlg,
                     [&](const QString&){ atualizarValidacao(); });

    atualizarValidacao();

    if (dlg.exec() == QDialog::Accepted) {
        data.nome   = edNome->text().trimmed();
        data.email  = edEmail->text().trimmed();
        data.cpf    = edCpf->text().trimmed();
        data.senha  = edSenha->text();
        const QString tipo = rbTec->isChecked() ? "Técnico" : "Graduação";
        const QString area = cbArea->currentText();
        data.categoria = QString("%1 - %2").arg(tipo, area);
        return true;
    }
    return false;
}

} // namespace

// ====== Implementação da página ======

PaginaAvaliadores::PaginaAvaliadores(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::PaginaAvaliadores)
    , m_table(new QTableView(this))
    , m_model(new QStandardItemModel(0, 6, this))
    , m_filter(new AvaliadorFilterModel(this))
    , m_btnNovo(new QPushButton(" Adicionar", this))
    , m_btnEditar(new QPushButton("️ Editar", this))
    , m_btnRemover(new QPushButton(" Excluir", this))
    , m_btnRecarregar(new QPushButton(" Recarregar", this))
    , m_btnExportCsv(new QPushButton(" Exportar CSV", this))
    , m_editBusca(new QLineEdit(this))
    , m_comboCategoria(new QComboBox(this))
    , m_labelTotal(new QLabel(this))
{
    ui->setupUi(this);

    m_comboCategoria->setEditable(false);

    this->setObjectName("paginaavaliadores");

    // ===== DESIGN MODERNO COM GRADIENTE TECH =====
    this->setStyleSheet(R"(
        QWidget#paginaavaliadores {
           background-color: #05070d;                      /* fundo bem escuro */
           background-image: url(:/img/img/Logo.jpg);      /* caminho do resources.qrc */
           background-position: center bottom;             /* centralizado lá embaixo */
           background-repeat: no-repeat;

        }

        QLabel#titulo {
            color: #00D4FF;
            font-size: 24px;
            font-weight: bold;
            padding: 8px 0px;
        }


        QTableView {
            background-color: transparent;                  /* deixa o viewport mandar no fundo */
            alternate-background-color: rgba(32, 42, 60, 0.85);
            gridline-color: #2a3f5f;
            selection-color: #FFFFFF;
            border: 2px solid #2a3f5f;
            border-radius: 10px;
            color: #E0E0E0;
        }

        QTableView::viewport {
            background-color: rgba(26, 35, 50, 0.85);
            background-image: url(:/img/img/Logo.jpg);     /* caminho do resources.qrc */
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
            padding: 10px 30px 10px 15px;   /* espaço pra seta */
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

        QPushButton#btnDanger {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #D946A6, stop:1 #FF6B9D);
        }

        QPushButton#btnDanger:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #E956B6, stop:1 #FF7BAD);
        }

        QPushButton#btnSecondary {
            background: rgba(42, 63, 95, 0.8);
            border: 2px solid #2a3f5f;
        }

        QPushButton#btnSecondary:hover {
            background: rgba(52, 73, 105, 1);
            border: 2px solid #3a4f6f;
        }

        QLabel#labelTotal {
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
    )");

    m_btnRemover->setObjectName("btnDanger");
    m_btnRecarregar->setObjectName("btnSecondary");
    m_labelTotal->setObjectName("labelTotal");

    auto *root = ui->verticalLayout;
    root->setContentsMargins(120, 40, 120, 40);
   root->setSpacing(20);

    // ---- Header com título estilizado ----
    auto *headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(16);

    auto *titulo = new QLabel(" Avaliadores Cadastrados", this);
    titulo->setObjectName("titulo");

    headerLayout->addWidget(titulo);
    headerLayout->addStretch();
    headerLayout->addWidget(m_btnNovo);
    headerLayout->addWidget(m_btnExportCsv);
    root->addLayout(headerLayout);

    // ---- Barra de filtros modernizada ----
    auto *filterLayout = new QHBoxLayout();
    filterLayout->setSpacing(12);

    auto *lblBuscar = new QLabel(" Buscar:", this);
    lblBuscar->setObjectName("labelBuscar");
    filterLayout->addWidget(lblBuscar);

    m_editBusca->setPlaceholderText("Digite o nome do avaliador...");
    m_editBusca->setMinimumWidth(250);
    filterLayout->addWidget(m_editBusca);

    filterLayout->addSpacing(20);

    auto *lblCat = new QLabel(" Categoria:", this);
    lblCat->setObjectName("labelCategoria");
    filterLayout->addWidget(lblCat);

    m_comboCategoria->addItem("Todos");
    m_comboCategoria->addItem("Técnico");
    m_comboCategoria->addItem("Graduação");
    filterLayout->addWidget(m_comboCategoria);
    filterLayout->addStretch();

    root->addLayout(filterLayout);

    // ---- Tabela ----
    root->addWidget(m_table, 1);

    // ---- Botões de ação ----
    auto *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(12);
    btnLayout->addWidget(m_btnEditar);
    btnLayout->addWidget(m_btnRemover);
    btnLayout->addStretch();
    btnLayout->addWidget(m_btnRecarregar);
    root->addLayout(btnLayout);

    // ---- Rodapé ----
    root->addWidget(m_labelTotal);

    // Configuração do modelo
    m_model->setHorizontalHeaderLabels({"ID","Nome","Email","CPF","Categoria","Senha"});
    m_filter->setSourceModel(m_model);
    m_table->setModel(m_filter);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->setAlternatingRowColors(true);
    m_table->setSortingEnabled(true);
    m_table->sortByColumn(0, Qt::AscendingOrder);
    m_table->setColumnHidden(5, true);
    m_table->setFocusPolicy(Qt::NoFocus);

    // Ajustar largura das colunas
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);

    // Duplo clique para editar
    connect(m_table, &QTableView::doubleClicked,
            this, [this](const QModelIndex&){ onEditar(); });

    // Conexões dos botões
    connect(m_btnNovo,       &QPushButton::clicked, this, &PaginaAvaliadores::onNovo);
    connect(m_btnEditar,     &QPushButton::clicked, this, &PaginaAvaliadores::onEditar);
    connect(m_btnRemover,    &QPushButton::clicked, this, &PaginaAvaliadores::onRemover);
    connect(m_btnRecarregar, &QPushButton::clicked, this, &PaginaAvaliadores::onRecarregar);
    connect(m_btnExportCsv,  &QPushButton::clicked, this, &PaginaAvaliadores::onExportCsv);

    // Filtros
    connect(m_editBusca, &QLineEdit::textChanged,
            this, &PaginaAvaliadores::onBuscaChanged);
    connect(m_comboCategoria,
            static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &PaginaAvaliadores::onCategoriaChanged);

    carregarDoArquivo();
    atualizarTotal();
}

PaginaAvaliadores::~PaginaAvaliadores() {
    delete ui;
}

int PaginaAvaliadores::selectedRow() const {
    if (!m_table->model()) return -1;
    const QModelIndex proxyIdx = m_table->currentIndex();
    if (!proxyIdx.isValid()) return -1;
    const QModelIndex srcIdx = m_filter->mapToSource(proxyIdx);
    return srcIdx.row();
}

void PaginaAvaliadores::addAvaliador(const QString& nome,
                                     const QString& email,
                                     const QString& cpf,
                                     const QString& categoria,
                                     const QString& senha)
{
    QList<QStandardItem*> row;
    auto id = new QStandardItem(QString::number(m_nextId++));
    id->setEditable(false);

    auto itNome  = new QStandardItem(nome);
    auto itEmail = new QStandardItem(email);
    auto itCpf   = new QStandardItem(cpf);
    auto itCat   = new QStandardItem(categoria);
    auto itSenha = new QStandardItem(senha);
    itSenha->setEditable(false);

    row << id << itNome << itEmail << itCpf << itCat << itSenha;
    m_model->appendRow(row);
}

void PaginaAvaliadores::onNovo() {
    AvaliadorData data;
    if (!abrirDialogoAvaliador(this, data, false))
        return;

    addAvaliador(data.nome, data.email, data.cpf, data.categoria, data.senha);
    salvarNoArquivo();
    atualizarTotal();
}

void PaginaAvaliadores::onEditar() {
    const int r = selectedRow();
    if (r < 0) {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("Editar");
        msgBox.setText("Por favor, selecione um avaliador para editar.");
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setStyleSheet(R"(
            QMessageBox {
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                    stop:0 #0a0e1a, stop:1 #1a1f2e);
            }
            QLabel { color: #E0E0E0; }
            QPushButton {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                    stop:0 #00A8CC, stop:1 #0088FF);
                border-radius: 6px;
                padding: 8px 20px;
                color: #FFFFFF;
                font-weight: bold;
            }
            QPushButton:hover {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                    stop:0 #00D4FF, stop:1 #00A8FF);
            }
        )");
        msgBox.exec();
        return;
    }

    AvaliadorData data;
    data.nome      = m_model->item(r,1)->text();
    data.email     = m_model->item(r,2)->text();
    data.cpf       = m_model->item(r,3)->text();
    data.categoria = m_model->item(r,4)->text();
    data.senha     = m_model->item(r,5)->text();

    if (!abrirDialogoAvaliador(this, data, true))
        return;

    m_model->item(r,1)->setText(data.nome);
    m_model->item(r,2)->setText(data.email);
    m_model->item(r,3)->setText(data.cpf);
    m_model->item(r,4)->setText(data.categoria);
    m_model->item(r,5)->setText(data.senha);

    salvarNoArquivo();
    atualizarTotal();
}

void PaginaAvaliadores::onRemover() {
    const int r = selectedRow();
    if (r < 0) {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("Remover");
        msgBox.setText("Por favor, selecione um avaliador para remover.");
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setStyleSheet(R"(
            QMessageBox {
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                    stop:0 #0a0e1a, stop:1 #1a1f2e);
            }
            QLabel { color: #E0E0E0; }
            QPushButton {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                    stop:0 #00A8CC, stop:1 #0088FF);
                border-radius: 6px;
                padding: 8px 20px;
                color: #FFFFFF;
                font-weight: bold;
            }
            QPushButton:hover {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                    stop:0 #00D4FF, stop:1 #00A8FF);
            }
        )");
        msgBox.exec();
        return;
    }

    const QString nome = m_model->item(r,1)->text();
    const QString cpf  = m_model->item(r,3)->text();
    const QString cat  = m_model->item(r,4)->text();

    QString texto = QString(
                        " <b>Confirmar Exclusão</b><br><br>"
                        "Você está prestes a excluir o seguinte avaliador:<br><br>"
                        "<b>Nome:</b> %1<br>"
                        "<b>Categoria:</b> %2<br>"
                        "<b>CPF:</b> %3<br><br>"
                        "<span style='color: #FF6B9D;'>⚠️ Esta ação não pode ser desfeita!</span><br><br>"
                        "Deseja realmente continuar?")
                        .arg(nome, cat, cpf);

    QMessageBox box(this);
    box.setWindowTitle("Confirmar Exclusão");
    box.setIcon(QMessageBox::Warning);
    box.setTextFormat(Qt::RichText);
    box.setText(texto);
    box.setStandardButtons(QMessageBox::Cancel | QMessageBox::Yes);

    QAbstractButton* btnYes = box.button(QMessageBox::Yes);
    QAbstractButton* btnCancel = box.button(QMessageBox::Cancel);

    if (btnYes) {
        btnYes->setText("️ Excluir");
        btnYes->setStyleSheet(R"(
            QPushButton {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                    stop:0 #D946A6, stop:1 #FF6B9D);
                border-radius: 6px;
                padding: 8px 20px;
                color: #FFFFFF;
                font-weight: bold;
                min-width: 100px;
            }
            QPushButton:hover {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                    stop:0 #E956B6, stop:1 #FF7BAD);
            }
        )");
    }

    if (btnCancel) {
        btnCancel->setText("Cancelar");
        btnCancel->setStyleSheet(R"(
            QPushButton {
                background: #2a3f5f;
                border-radius: 6px;
                padding: 8px 20px;
                color: #FFFFFF;
                font-weight: bold;
                min-width: 100px;
            }
            QPushButton:hover {
                background: #3a4f6f;
            }
        )");
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
    )");

    if (box.exec() == QMessageBox::Yes) {
        m_model->removeRow(r);
        salvarNoArquivo();
        atualizarTotal();

        QMessageBox success(this);
        success.setWindowTitle("Sucesso");
        success.setText(" Avaliador removido com sucesso!");
        success.setIcon(QMessageBox::Information);
        success.setStyleSheet(R"(
            QMessageBox {
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                    stop:0 #0a0e1a, stop:1 #1a1f2e);
            }
            QLabel { color: #00FF88; font-weight: bold; }
            QPushButton {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                    stop:0 #00A8CC, stop:1 #0088FF);
                border-radius: 6px;
                padding: 8px 20px;
                color: #FFFFFF;
                font-weight: bold;
            }
        )");
        success.exec();
    }
}

void PaginaAvaliadores::onRecarregar() {
    carregarDoArquivo();
    atualizarTotal();
}

bool PaginaAvaliadores::salvarNoArquivo() const {
    QFile f(m_arquivo);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(nullptr, "Salvar",
                             "Não foi possível abrir '"+m_arquivo+"' para escrita.");
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
            s.replace(';', ',');
            cols << s;
        }
        out << cols.join(';') << '\n';
    }
    return true;
}

bool PaginaAvaliadores::carregarDoArquivo() {
    QFile f(m_arquivo);
    if (!f.exists()) {
        m_model->removeRows(0, m_model->rowCount());
        m_nextId = 1;
        return true;
    }
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(nullptr, "Carregar",
                             "Não foi possível abrir '"+m_arquivo+"' para leitura.");
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
        if (p.size() < 5) continue;

        QList<QStandardItem*> row;
        auto idItem = new QStandardItem(p[0]); idItem->setEditable(false);
        row << idItem
            << new QStandardItem(p[1])
            << new QStandardItem(p[2])
            << new QStandardItem(p[3])
            << new QStandardItem(p[4]);

        if (p.size() >= 6)
            row << new QStandardItem(p[5]);
        else
            row << new QStandardItem("");

        m_model->appendRow(row);
    }
    recomputarNextId();
    return true;
}

void PaginaAvaliadores::recomputarNextId() {
    int maxId = 0;
    for (int r = 0; r < m_model->rowCount(); ++r) {
        bool ok = false;
        int id = m_model->item(r,0)->text().toInt(&ok);
        if (ok && id > maxId) maxId = id;
    }
    m_nextId = maxId + 1;
}

void PaginaAvaliadores::onBuscaChanged(const QString& texto) {
    if (m_filter) {
        m_filter->setNomeFiltro(texto);
        atualizarTotal();
    }
}

void PaginaAvaliadores::onCategoriaChanged(int index) {
    if (!m_filter) return;

    QString filtro;
    if (index == 1) filtro = "Técnico";
    else if (index == 2) filtro = "Graduação";
    else filtro.clear();

    m_filter->setCategoriaFiltro(filtro);
    atualizarTotal();
}

void PaginaAvaliadores::atualizarTotal() {
    const int total = m_filter ? m_filter->rowCount() : m_model->rowCount();
    if (total == 1)
        m_labelTotal->setText(" 1 registro encontrado");
    else
        m_labelTotal->setText(QString(" %1 registros encontrados").arg(total));
}

void PaginaAvaliadores::onExportCsv() {
    QString filename = QFileDialog::getSaveFileName(
        this,
        "Exportar avaliadores para CSV",
        "avaliadores.csv",
        "Arquivos CSV (*.csv);;Todos os arquivos (*.*)"
        );

    if (filename.isEmpty())
        return;

    QFile f(filename);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("Exportar CSV");
        msgBox.setText(" Não foi possível abrir o arquivo para escrita.");
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setStyleSheet(R"(
            QMessageBox {
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                    stop:0 #0a0e1a, stop:1 #1a1f2e);
            }
            QLabel { color: #E0E0E0; }
            QPushButton {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                    stop:0 #00A8CC, stop:1 #0088FF);
                border-radius: 6px;
                padding: 8px 20px;
                color: #FFFFFF;
                font-weight: bold;
            }
        )");
        msgBox.exec();
        return;
    }

    QTextStream out(&f);
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    out.setCodec("UTF-8");
#endif

    out << "ID;Nome;Email;CPF;Categoria;Senha\n";

    for (int r = 0; r < m_model->rowCount(); ++r) {
        QStringList cols;
        for (int c = 0; c < m_model->columnCount(); ++c) {
            QString s = m_model->item(r, c)->text();
            s.replace(';', ',');
            cols << s;
        }
        out << cols.join(';') << '\n';
    }

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Exportar CSV");
    msgBox.setText(" Arquivo CSV exportado com sucesso!");
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setStyleSheet(R"(
        QMessageBox {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #0a0e1a, stop:1 #1a1f2e);
        }
        QLabel { color: #00FF88; font-weight: bold; }
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #00A8CC, stop:1 #0088FF);
            border-radius: 6px;
            padding: 8px 20px;
            color: #FFFFFF;
            font-weight: bold;
        }
    )");
    msgBox.exec();
}
