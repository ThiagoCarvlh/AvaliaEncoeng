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


// ====== Filtro para busca + categoria (TableView moderno) ======
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
    QString categoria; // "Técnico - Automação Industrial"
    QString senha;
};

static void preencherCategorias(QComboBox* combo, bool tecnico) {
    combo->clear();
    if (tecnico) {
        combo->addItem("Automação Industrial");
        combo->addItem("Eletrônica");
        combo->addItem("Informática");
        combo->addItem("Outros (Técnico)");
    } else {
        combo->addItem("Engenharia de Software");
        combo->addItem("Eng. Controle e Automação");
        combo->addItem("Engenharia Civil");
        combo->addItem("Outros (Graduação)");
    }
}

// ==== Tela 2: formulário completo com validação em tempo real ====
static bool abrirDialogoAvaliador(QWidget* parent, AvaliadorData& data, bool edicao)
{
    QDialog dlg(parent);
    dlg.setWindowTitle(edicao ? "Editar Avaliador" : "Novo Avaliador");
    dlg.setModal(true);

    auto *mainLayout = new QVBoxLayout(&dlg);

    auto *titulo = new QLabel(edicao ? "Editar avaliador" : "Novo avaliador", &dlg);
    QFont f = titulo->font();
    f.setPointSize(f.pointSize() + 2);
    f.setBold(true);
    titulo->setFont(f);
    mainLayout->addWidget(titulo);

    // --- Dados pessoais ---
    auto *boxDados = new QGroupBox("Dados Pessoais", &dlg);
    auto *formDados = new QFormLayout(boxDados);

    auto *edNome  = new QLineEdit(data.nome, &dlg);
    auto *edEmail = new QLineEdit(data.email, &dlg);
    auto *edCpf   = new QLineEdit(data.cpf, &dlg);
    auto *edSenha = new QLineEdit(data.senha, &dlg);
    edSenha->setEchoMode(QLineEdit::Password);

    auto *lblNomeStatus  = new QLabel(" ", &dlg);
    auto *lblEmailStatus = new QLabel(" ", &dlg);
    auto *lblCpfStatus   = new QLabel(" ", &dlg);

    formDados->addRow("Nome:", edNome);
    formDados->addRow("", lblNomeStatus);
    formDados->addRow("Email:", edEmail);
    formDados->addRow("", lblEmailStatus);
    formDados->addRow("CPF:", edCpf);
    formDados->addRow("", lblCpfStatus);
    formDados->addRow("Senha:", edSenha);

    boxDados->setLayout(formDados);
    mainLayout->addWidget(boxDados);

    // --- Categoria e área ---
    auto *boxCat = new QGroupBox("Categoria e Especialização", &dlg);
    auto *layCat = new QVBoxLayout(boxCat);

    auto *linhaRadios = new QHBoxLayout();
    auto *rbTec  = new QRadioButton("Técnico", &dlg);
    auto *rbGrad = new QRadioButton("Graduação", &dlg);
    linhaRadios->addWidget(rbTec);
    linhaRadios->addWidget(rbGrad);
    linhaRadios->addStretch();
    layCat->addLayout(linhaRadios);

    auto *cbArea = new QComboBox(&dlg);
    layCat->addWidget(cbArea);

    // estado inicial
    if (data.categoria.startsWith("Técnico"))
        rbTec->setChecked(true);
    else if (data.categoria.startsWith("Graduação"))
        rbGrad->setChecked(true);
    else
        rbTec->setChecked(true);

    preencherCategorias(cbArea, rbTec->isChecked());

    // tenta selecionar área se já tiver
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

    boxCat->setLayout(layCat);
    mainLayout->addWidget(boxCat);

    // --- Botões ---
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                         Qt::Horizontal, &dlg);
    buttons->button(QDialogButtonBox::Ok)->setText("Salvar");
    mainLayout->addWidget(buttons);

    QObject::connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    // estilos dos status
    auto setOk = [](QLabel* lbl, const QString& txt){
        lbl->setText(QString("✓ %1").arg(txt));
        lbl->setStyleSheet("color:#4EC9B0;"); // verde água
    };
    auto setErr = [](QLabel* lbl, const QString& txt){
        lbl->setText(txt);
        lbl->setStyleSheet("color:#F48771;"); // vermelho
    };

    auto *btnSalvar = buttons->button(QDialogButtonBox::Ok);

    auto atualizarValidacao = [&]() {
        const bool okNome  = nomeValido(edNome->text());
        const bool okEmail = emailValido(edEmail->text());
        const bool okCpf   = cpfValido(edCpf->text());

        if (okNome)  setOk(lblNomeStatus,  "Nome válido");
        else         setErr(lblNomeStatus, "Nome muito curto");

        if (okEmail) setOk(lblEmailStatus,"Email válido");
        else         setErr(lblEmailStatus,"Email inválido");

        if (okCpf)   setOk(lblCpfStatus,  "CPF válido");
        else         setErr(lblCpfStatus, "CPF inválido (11 dígitos)");

        btnSalvar->setEnabled(okNome && okEmail && okCpf);
    };

    QObject::connect(edNome,  &QLineEdit::textChanged, &dlg,
                     [&](const QString&){ atualizarValidacao(); });
    QObject::connect(edEmail, &QLineEdit::textChanged, &dlg,
                     [&](const QString&){ atualizarValidacao(); });
    QObject::connect(edCpf,   &QLineEdit::textChanged, &dlg,
                     [&](const QString&){ atualizarValidacao(); });

    atualizarValidacao();

    // herda o tema escuro da página
    dlg.setStyleSheet(parent->styleSheet());

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
    , m_model(new QStandardItemModel(0, 6, this)) // ID, Nome, Email, CPF, Categoria, Senha
    , m_filter(new AvaliadorFilterModel(this))
    , m_btnNovo(new QPushButton("+ Add", this))
    , m_btnEditar(new QPushButton("Editar", this))
    , m_btnRemover(new QPushButton("Excluir", this))
    , m_btnRecarregar(new QPushButton("Recarregar", this))
    , m_btnExportCsv(new QPushButton("Exportar CSV", this))
    , m_editBusca(new QLineEdit(this))
    , m_comboCategoria(new QComboBox(this))
    , m_labelTotal(new QLabel(this))
{
    ui->setupUi(this);

    // para o stylesheet pegar
    this->setObjectName("paginaavaliadores");

    // Tema escuro que você passou
    this->setStyleSheet(R"(
QWidget#paginaavaliadores {
    background-color: #1E1E1E;
    color: #FFFFFF;
}
QTableView {
    background-color: #2D2D30;
    alternate-background-color: #252526;
    gridline-color: #3F3F46;
    selection-background-color: #0E639C;
    selection-color: #FFFFFF;
    border: 1px solid #3F3F46;
}
QHeaderView::section {
    background-color: #2D2D30;
    color: #CCCCCC;
    padding: 6px;
    border: 1px solid #3F3F46;
}
QLineEdit, QComboBox {
    background-color: #252526;
    border: 1px solid #3F3F46;
    padding: 4px 6px;
    color: #FFFFFF;
}
QPushButton {
    background-color: #0E639C;
    border-radius: 4px;
    padding: 6px 12px;
    color: #FFFFFF;
    border: none;
}
QPushButton#btnDanger {
    background-color: #F48771;
}
QLabel#labelTotal {
    color: #CCCCCC;
}
)");

    m_btnRemover->setObjectName("btnDanger");
    m_labelTotal->setObjectName("labelTotal");

    auto *root = ui->verticalLayout;
    root->setContentsMargins(16,16,16,16);
    root->setSpacing(10);

    // ---- "Tela principal" da parte de avaliadores (header + [+ Add]) ----
    auto *headerLayout = new QHBoxLayout();
    auto *titulo = new QLabel("Avaliadores Cadastrados", this);
    QFont ft = titulo->font();
    ft.setBold(true);
    ft.setPointSize(ft.pointSize() + 2);
    titulo->setFont(ft);

    headerLayout->addWidget(titulo);
    headerLayout->addStretch();
    headerLayout->addWidget(m_btnNovo);
    headerLayout->addWidget(m_btnExportCsv);
    root->addLayout(headerLayout);


    // ---- Linha de filtros: Buscar + Categoria ----
    auto *filterLayout = new QHBoxLayout();
    auto *lblBuscar = new QLabel("Buscar:", this);
    filterLayout->addWidget(lblBuscar);
    filterLayout->addWidget(m_editBusca);

    auto *lblCat = new QLabel("Categoria:", this);
    filterLayout->addSpacing(12);
    filterLayout->addWidget(lblCat);

    m_comboCategoria->addItem("Todos");
    m_comboCategoria->addItem("Técnico");
    m_comboCategoria->addItem("Graduação");
    filterLayout->addWidget(m_comboCategoria);
    filterLayout->addStretch();

    root->addLayout(filterLayout);

    // ---- Tabela (Tela de Listagem) ----
    root->addWidget(m_table);

    // ---- Botões de ação ----
    auto *btnLayout = new QHBoxLayout();
    btnLayout->addWidget(m_btnEditar);
    btnLayout->addWidget(m_btnRemover);
    btnLayout->addStretch();
    btnLayout->addWidget(m_btnRecarregar);
    root->addLayout(btnLayout);

    // ---- Rodapé "X registros encontrados" ----
    root->addWidget(m_labelTotal);

    // Modelo + filtro
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
    m_table->setColumnHidden(5, true); // não mostra senha

    // duplo clique = editar
    connect(m_table, &QTableView::doubleClicked,
            this, [this](const QModelIndex&){ onEditar(); });

    // conexões dos botões
    connect(m_btnNovo,       &QPushButton::clicked, this, &PaginaAvaliadores::onNovo);
    connect(m_btnEditar,     &QPushButton::clicked, this, &PaginaAvaliadores::onEditar);
    connect(m_btnRemover,    &QPushButton::clicked, this, &PaginaAvaliadores::onRemover);
    connect(m_btnRecarregar, &QPushButton::clicked, this, &PaginaAvaliadores::onRecarregar);
    connect(m_btnExportCsv,  &QPushButton::clicked, this, &PaginaAvaliadores::onExportCsv);
    // filtros
    connect(m_editBusca, &QLineEdit::textChanged,
            this, &PaginaAvaliadores::onBuscaChanged);
    connect(m_comboCategoria,
            static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &PaginaAvaliadores::onCategoriaChanged);

    // carrega do arquivo e atualiza contador
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

// ====== Botão [+ Add] -> abre formulário (Tela 2) ======
void PaginaAvaliadores::onNovo() {
    AvaliadorData data;
    if (!abrirDialogoAvaliador(this, data, false))
        return;

    addAvaliador(data.nome, data.email, data.cpf, data.categoria, data.senha);
    salvarNoArquivo();
    atualizarTotal();
}

// ====== Editar avaliador selecionado ======
void PaginaAvaliadores::onEditar() {
    const int r = selectedRow();
    if (r < 0) {
        QMessageBox::information(this, "Editar", "Selecione um avaliador.");
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

// ====== Dialogo de confirmação (Tela 4) ======
void PaginaAvaliadores::onRemover() {
    const int r = selectedRow();
    if (r < 0) {
        QMessageBox::information(this, "Remover", "Selecione um avaliador.");
        return;
    }

    const QString nome = m_model->item(r,1)->text();
    const QString cpf  = m_model->item(r,3)->text();
    const QString cat  = m_model->item(r,4)->text();

    QString texto = QString(
                        "Avaliador encontrado:\n\n"
                        "Nome: %1\n"
                        "Área/Categoria: %2\n"
                        "CPF: %3\n\n"
                        "Esta ação não pode ser desfeita.\n\n"
                        "Deseja realmente excluir?")
                        .arg(nome, cat, cpf);

    QMessageBox box(this);
    box.setWindowTitle("Confirmar Exclusão");
    box.setIcon(QMessageBox::Warning);
    box.setText(texto);
    box.setStandardButtons(QMessageBox::Cancel | QMessageBox::Yes);

    QAbstractButton* btnYes = box.button(QMessageBox::Yes);
    if (btnYes) btnYes->setText("Excluir");


    // tema escuro no diálogo
    box.setStyleSheet(R"(
QMessageBox {
    background-color: #1E1E1E;
}
QLabel {
    color: #FFFFFF;
}
QPushButton {
    background-color: #2D2D30;
    color: #FFFFFF;
    border-radius: 4px;
    padding: 6px 12px;
}
)");

    if (box.exec() == QMessageBox::Yes) {
        m_model->removeRow(r);
        salvarNoArquivo();
        atualizarTotal();
    }
}

void PaginaAvaliadores::onRecarregar() {
    carregarDoArquivo();
    atualizarTotal();
}

// ====== Persistência em avaliadores.txt ======
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

// ====== Filtros (buscar + categoria) ======
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

// ====== Rodapé "X registros encontrados" ======
void PaginaAvaliadores::atualizarTotal() {
    const int total = m_filter ? m_filter->rowCount() : m_model->rowCount();
    if (total == 1)
        m_labelTotal->setText("1 registro encontrado");
    else
        m_labelTotal->setText(QString("%1 registros encontrados").arg(total));
}

void PaginaAvaliadores::onExportCsv() {
    // Pergunta onde salvar
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
        QMessageBox::warning(this, "Exportar CSV",
                             "Não foi possível abrir o arquivo para escrita.");
        return;
    }

    QTextStream out(&f);
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    out.setCodec("UTF-8");
#endif

    // Cabeçalho opcional – ajuda no Excel
    out << "ID;Nome;Email;CPF;Categoria;Senha\n";

    for (int r = 0; r < m_model->rowCount(); ++r) {
        QStringList cols;
        for (int c = 0; c < m_model->columnCount(); ++c) {
            QString s = m_model->item(r, c)->text();
            s.replace(';', ','); // evita quebrar o CSV se alguém digitar ';'
            cols << s;
        }
        out << cols.join(';') << '\n';
    }

    QMessageBox::information(this, "Exportar CSV",
                             "Arquivo CSV exportado com sucesso.");
}
