#include "paginafichas.h"
#include "ui_paginafichas.h"

#include <QTableView>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
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
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QListWidget>
#include <QSortFilterProxyModel>
#include <QTextEdit>
#include <QScrollArea>
#include <QList>
#include <QStringList>

// PDF / Impressão
#include <QPrinter>
#include <QPageSize>
#include <QPageLayout>
#include <QMarginsF>
#include <QTextDocument>

// ================== Filtro para busca + tipo ==================

class FichaFilterModel : public QSortFilterProxyModel {
public:
    explicit FichaFilterModel(QObject* parent = nullptr)
        : QSortFilterProxyModel(parent) {}

    void setNomeFiltro(const QString& n) {
        m_nomeFiltro = n.trimmed();
        invalidateFilter();
    }

    void setTipoFiltro(const QString& t) {
        m_tipoFiltro = t.trimmed();
        invalidateFilter();
    }

protected:
    bool filterAcceptsRow(int source_row,
                          const QModelIndex& source_parent) const override
    {
        if (!sourceModel())
            return true;

        // Coluna 1 = Tipo
        const QModelIndex idxTipo =
            sourceModel()->index(source_row, 1, source_parent);
        const QString tipo = sourceModel()->data(idxTipo).toString();

        const bool okNome = m_nomeFiltro.isEmpty()
                            || tipo.contains(m_nomeFiltro, Qt::CaseInsensitive);
        const bool okTipo = m_tipoFiltro.isEmpty()
                            || tipo.contains(m_tipoFiltro, Qt::CaseInsensitive);

        return okNome && okTipo;
    }

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
    QString m_tipoFiltro;
};

// ================== Helpers internos ==================

namespace {

// Forward declarations dos diálogos
bool abrirDialogoFicha(QWidget* parent, Ficha& ficha, bool edicao);
void visualizarFicha(QWidget* parent, const Ficha& ficha);

// Validações
bool tipoValido(const QString& t) {
    return t.trimmed().size() >= 3;
}

bool resolucaoValida(const QString& r) {
    return !r.trimmed().isEmpty();
}

// ===== DIÁLOGO PRINCIPAL - CRIAR/EDITAR FICHA =====


// ← ADICIONAR AQUI
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

bool abrirDialogoFicha(QWidget* parent, Ficha& ficha, bool edicao)
{
    QDialog dlg(parent);
    dlg.setWindowTitle(edicao ? "Editar Ficha de Avaliação" : "Nova Ficha de Avaliação");
    dlg.setModal(true);
    dlg.setMinimumSize(850, 700);  // ← AUMENTADO

    // ===== ESTILO MELHORADO =====
    QString style = R"(
        QDialog {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #0a0e1a, stop:1 #1a1f2e);
        }

        QLabel {
            color: #E0E0E0;
            font-size: 12px;  /* tamanho padrão legível */
        }

        QLabel#labelTitulo {
            color: #00D4FF;
            font-size: 20px;  /* título grande */
            font-weight: bold;
            padding: 12px 0;
        }

        QGroupBox {
            border: 2px solid #2a3f5f;
            border-radius: 8px;
            margin-top: 18px;  /* mais espaço pro título */
            padding-top: 18px;
            color: #00D4FF;
            font-weight: bold;
            font-size: 13px;  /* título do groupbox legível */
        }

        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            padding: 6px 12px;
            background: #1a2332;
            border-radius: 4px;
            color: #00D4FF;  /* garante cor */
            font-size: 13px;
        }

        QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox {
            background-color: #1a2332;
            border: 2px solid #2a3f5f;
            border-radius: 6px;
            padding: 8px 12px;
            color: #FFFFFF;
            font-size: 12px;
            min-height: 36px;
        }

        QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus {
            border: 2px solid #00D4FF;
        }

        QComboBox::drop-down {
            border: none;
            width: 24px;
        }

        QComboBox::down-arrow {
            image: url(:/img/img/Seta.png);
            width: 16px;
            height: 16px;
            margin-right: 6px;
        }

        QComboBox QAbstractItemView {
            background-color: #1a2332;
            border: 2px solid #00D4FF;
            selection-background-color: #00A8CC;
            color: #FFFFFF;
        }

        QCheckBox {
            color: #E0E0E0;
            spacing: 8px;
            font-size: 12px;
            padding: 4px;
        }

        QCheckBox::indicator {
            width: 18px;
            height: 18px;
            border-radius: 4px;
            border: 2px solid #2a3f5f;
            background: #1a2332;
        }

        QCheckBox::indicator:checked {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #00D4FF, stop:1 #0088FF);
            border: 2px solid #00D4FF;
        }

        QListWidget {
            background-color: #1a2332;
            border: 2px solid #2a3f5f;
            border-radius: 6px;
            color: #E0E0E0;
            padding: 8px;
            font-size: 12px;
        }

        QListWidget::item {
            padding: 10px;
            border-radius: 4px;
        }

        QListWidget::item:hover {
            background-color: rgba(0, 168, 204, 0.15);
        }

        QListWidget::item:selected {
            background-color: rgba(0, 220, 255, 0.35);
            color: #FFFFFF;
        }

        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #00A8CC, stop:1 #0088FF);
            border: none;
            border-radius: 6px;
            padding: 10px 20px;
            color: #FFFFFF;
            font-weight: bold;
            font-size: 13px;
            min-width: 120px;
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

        QPushButton#btnDanger {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #D946A6, stop:1 #FF6B9D);
        }

        QPushButton#btnDanger:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #E956B6, stop:1 #FF7BAD);
        }
    )";

    dlg.setStyleSheet(style);

    auto* mainLayout = new QVBoxLayout(&dlg);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(30, 30, 30, 30);

    // ===== TÍTULO PRINCIPAL =====
    auto* titulo = new QLabel(edicao ? "✏️ Editar Ficha de Avaliação" : "➕ Nova Ficha de Avaliação", &dlg);
    titulo->setObjectName("labelTitulo");
    mainLayout->addWidget(titulo);

    // ===== SCROLL AREA PARA TODO O CONTEÚDO =====
    auto* scrollArea = new QScrollArea(&dlg);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }");

    auto* scrollContent = new QWidget();
    auto* scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->setSpacing(20);
    scrollLayout->setContentsMargins(0, 0, 10, 0);  // margem direita para scrollbar

    // ===== IDENTIFICAÇÃO =====
    auto* boxId = new QGroupBox("📋 Identificação da Ficha", scrollContent);
    auto* formId = new QFormLayout(boxId);
    formId->setSpacing(12);
    formId->setContentsMargins(20, 25, 20, 20);
    formId->setLabelAlignment(Qt::AlignRight);

    auto* edTipo         = new QLineEdit(ficha.tipoFicha, scrollContent);
    auto* edResolucaoNum = new QLineEdit(ficha.resolucaoNum, scrollContent);
    auto* edResolucaoAno = new QLineEdit(ficha.resolucaoAno, scrollContent);

    // RadioButtons para Técnico/Graduação
    auto* linhaRadios = new QHBoxLayout();
    auto* rbTecnico  = new QRadioButton("Técnico", scrollContent);
    auto* rbGraduacao = new QRadioButton("Graduação", scrollContent);
    linhaRadios->addWidget(rbTecnico);
    linhaRadios->addWidget(rbGraduacao);
    linhaRadios->addStretch();

    // ComboBox de cursos
    auto* cbCurso = new QComboBox(scrollContent);

    // Estado inicial baseado no curso salvo
    bool isTecnico = ficha.categoriaCurso.startsWith("Técnico") ||
                     ficha.curso.contains("Mecânica") ||
                     ficha.curso.contains("Eletrônica") ||
                     ficha.curso.contains("Eletrotécnica") ||
                     ficha.curso.contains("Automação") ||
                     ficha.curso.contains("Mecatrônica") ||
                     ficha.curso.contains("Informática") ||
                     ficha.curso.contains("Qualidade") ||
                     ficha.curso.contains("Logística") ||
                     ficha.curso.contains("Segurança");

    if (isTecnico)
        rbTecnico->setChecked(true);
    else
        rbGraduacao->setChecked(true);

    // Preenche combo baseado na seleção
    preencherCategorias(cbCurso, rbTecnico->isChecked());

    // Tenta selecionar o curso salvo
    if (!ficha.curso.isEmpty()) {
        int idx = cbCurso->findText(ficha.curso, Qt::MatchContains);
        if (idx >= 0) cbCurso->setCurrentIndex(idx);
    }

    // Conecta mudança de radio para atualizar combo
    QObject::connect(rbTecnico, &QRadioButton::toggled, scrollContent,
                     [cbCurso](bool checked) {
                         if (checked) preencherCategorias(cbCurso, true);
                     });

    QObject::connect(rbGraduacao, &QRadioButton::toggled, scrollContent,
                     [cbCurso](bool checked) {
                         if (checked) preencherCategorias(cbCurso, false);
                     });

    auto* spnNotaMin = new QDoubleSpinBox(scrollContent);
    auto* spnNotaMax = new QDoubleSpinBox(scrollContent);
    spnNotaMin->setRange(0.0, 100.0);
    spnNotaMax->setRange(0.0, 100.0);
    spnNotaMin->setValue(ficha.notaMin);
    spnNotaMax->setValue(ficha.notaMax);
    spnNotaMin->setDecimals(1);
    spnNotaMax->setDecimals(1);

    edTipo->setPlaceholderText("Ex: TCC, Projeto Integrador...");
    edResolucaoNum->setPlaceholderText("Ex: 02");
    edResolucaoAno->setPlaceholderText("Ex: 2024");

    formId->addRow("Tipo da Ficha: *", edTipo);
    formId->addRow("Resolução Nº:", edResolucaoNum);
    formId->addRow("Ano:", edResolucaoAno);
    formId->addRow("Categoria:", linhaRadios);  // ← radios
    formId->addRow("Curso:", cbCurso);          // ← combo atualizado

    auto* layoutNotas = new QHBoxLayout();
    layoutNotas->addWidget(new QLabel("De:", scrollContent));
    layoutNotas->addWidget(spnNotaMin);
    layoutNotas->addWidget(new QLabel("até:", scrollContent));
    layoutNotas->addWidget(spnNotaMax);
    layoutNotas->addStretch();
    formId->addRow("Escala de Notas:", layoutNotas);

    scrollLayout->addWidget(boxId);

    // ===== SEÇÕES =====
    auto* boxSecoes = new QGroupBox("📝 Seções de Avaliação", scrollContent);
    auto* laySecoes = new QVBoxLayout(boxSecoes);
    laySecoes->setSpacing(12);
    laySecoes->setContentsMargins(20, 25, 20, 20);

    auto* listSecoes = new QListWidget(scrollContent);
    listSecoes->setMinimumHeight(180);

    for (const auto& secao : ficha.secoes) {
        QString item = QString("%1 - %2 (%3 quesitos)")
        .arg(secao.identificador, secao.titulo)
            .arg(secao.quesitos.size());
        listSecoes->addItem(item);
    }

    auto* btnAddSecao  = new QPushButton("➕ Adicionar Seção", scrollContent);
    auto* btnEditSecao = new QPushButton("✏️ Editar Seção", scrollContent);
    auto* btnRemSecao  = new QPushButton("🗑️ Remover Seção", scrollContent);
    btnRemSecao->setObjectName("btnDanger");

    auto* layoutBtnSecoes = new QHBoxLayout();
    layoutBtnSecoes->setSpacing(10);
    layoutBtnSecoes->addWidget(btnAddSecao);
    layoutBtnSecoes->addWidget(btnEditSecao);
    layoutBtnSecoes->addWidget(btnRemSecao);
    layoutBtnSecoes->addStretch();

    laySecoes->addWidget(listSecoes);
    laySecoes->addLayout(layoutBtnSecoes);
    scrollLayout->addWidget(boxSecoes);

    // ===== CONFIGURAÇÕES ADICIONAIS =====
    auto* boxConfig = new QGroupBox("⚙️ Configurações Adicionais", scrollContent);
    auto* layConfig = new QVBoxLayout(boxConfig);
    layConfig->setSpacing(10);
    layConfig->setContentsMargins(20, 25, 20, 20);

    auto* chkData       = new QCheckBox("Incluir campo 'Data da Avaliação'", scrollContent);
    auto* chkAvaliador  = new QCheckBox("Incluir campo 'Professor Avaliador'", scrollContent);
    auto* chkOrientador = new QCheckBox("Incluir campo 'Professor Orientador'", scrollContent);
    auto* chkObs        = new QCheckBox("Incluir campo 'Observações'", scrollContent);

    chkData->setChecked(ficha.incluirDataAvaliacao);
    chkAvaliador->setChecked(ficha.incluirProfessorAvaliador);
    chkOrientador->setChecked(ficha.incluirProfessorOrientador);
    chkObs->setChecked(ficha.incluirObservacoes);

    auto* lblTextoAprov = new QLabel("Texto de aprovação (rodapé):", scrollContent);
    auto* edTextoAprovacao = new QLineEdit(ficha.textoAprovacao, scrollContent);
    edTextoAprovacao->setPlaceholderText("Ex: APROVADO PELA RESOLUÇÃO CONSUP Nº 02 DE 03/04/2012");

    layConfig->addWidget(chkData);
    layConfig->addWidget(chkAvaliador);
    layConfig->addWidget(chkOrientador);
    layConfig->addWidget(chkObs);
    layConfig->addSpacing(10);
    layConfig->addWidget(lblTextoAprov);
    layConfig->addWidget(edTextoAprovacao);

    scrollLayout->addWidget(boxConfig);
    scrollLayout->addStretch();

    // Finaliza scroll
    scrollArea->setWidget(scrollContent);
    mainLayout->addWidget(scrollArea, 1);  // stretch factor 1 = ocupa espaço disponível

    // ===== BOTÕES FINAIS (FORA DO SCROLL) =====
    auto* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(12);

    auto* btnCancel  = new QPushButton("Cancelar", &dlg);
    btnCancel->setObjectName("btnCancel");
    auto* btnPreview = new QPushButton("📄 Visualizar", &dlg);
    auto* btnSave    = new QPushButton(edicao ? "💾 Salvar" : "➕ Criar Ficha", &dlg);

    btnLayout->addStretch();
    btnLayout->addWidget(btnCancel);
    btnLayout->addWidget(btnPreview);
    btnLayout->addWidget(btnSave);
    mainLayout->addLayout(btnLayout);

    // ===== CONEXÕES =====
    QObject::connect(btnSave,   &QPushButton::clicked, &dlg, &QDialog::accept);
    QObject::connect(btnCancel, &QPushButton::clicked, &dlg, &QDialog::reject);

    QObject::connect(btnPreview, &QPushButton::clicked, &dlg, [&]() {
        ficha.tipoFicha                 = edTipo->text().trimmed();
        ficha.resolucaoNum              = edResolucaoNum->text().trimmed();
        ficha.resolucaoAno              = edResolucaoAno->text().trimmed();
        ficha.curso                     = cbCurso->currentText();
        ficha.notaMin                   = spnNotaMin->value();
        ficha.notaMax                   = spnNotaMax->value();
        ficha.incluirDataAvaliacao      = chkData->isChecked();
        ficha.incluirProfessorAvaliador = chkAvaliador->isChecked();
        ficha.incluirProfessorOrientador= chkOrientador->isChecked();
        ficha.incluirObservacoes        = chkObs->isChecked();
        ficha.textoAprovacao            = edTextoAprovacao->text().trimmed();
        visualizarFicha(&dlg, ficha);
    });

    QObject::connect(btnAddSecao, &QPushButton::clicked, &dlg, [&]() {
        QMessageBox::information(&dlg, "Em desenvolvimento",
                                 "Funcionalidade de adicionar seção será implementada em breve.");
    });

    QObject::connect(btnEditSecao, &QPushButton::clicked, &dlg, [&]() {
        if (listSecoes->currentRow() < 0) {
            QMessageBox::information(&dlg, "Editar Seção", "Selecione uma seção.");
            return;
        }
        QMessageBox::information(&dlg, "Em desenvolvimento",
                                 "Funcionalidade de editar seção será implementada em breve.");
    });

    QObject::connect(btnRemSecao, &QPushButton::clicked, &dlg, [&]() {
        int row = listSecoes->currentRow();
        if (row < 0) {
            QMessageBox::information(&dlg, "Remover Seção", "Selecione uma seção.");
            return;
        }
        if (row < ficha.secoes.size()) {
            ficha.secoes.remove(row);
            delete listSecoes->takeItem(row);
        }
    });

    // Validação
    auto validar = [&]() {
        bool ok = tipoValido(edTipo->text());
        btnSave->setEnabled(ok);
    };
    QObject::connect(edTipo, &QLineEdit::textChanged, &dlg, [&](){ validar(); });
    validar();

    if (dlg.exec() == QDialog::Accepted) {
        ficha.tipoFicha                 = edTipo->text().trimmed();
        ficha.resolucaoNum              = edResolucaoNum->text().trimmed();
        ficha.resolucaoAno              = edResolucaoAno->text().trimmed();


        const QString tipo = rbTecnico->isChecked() ? "Técnico" : "Graduação";
        const QString cursoSelecionado = cbCurso->currentText();
        ficha.curso = cursoSelecionado;
        ficha.categoriaCurso = QString("%1 - %2").arg(tipo, cursoSelecionado);

        ficha.notaMin                   = spnNotaMin->value();
        ficha.notaMax                   = spnNotaMax->value();
        ficha.incluirDataAvaliacao      = chkData->isChecked();
        ficha.incluirProfessorAvaliador = chkAvaliador->isChecked();
        ficha.incluirProfessorOrientador= chkOrientador->isChecked();
        ficha.incluirObservacoes        = chkObs->isChecked();
        ficha.textoAprovacao            = edTextoAprovacao->text().trimmed();
        return true;
    }
    return false;
}

// ===== VISUALIZADOR DE FICHA =====

void visualizarFicha(QWidget* parent, const Ficha& ficha)
{
    QDialog dlg(parent);
    dlg.setWindowTitle("Pré-visualização da Ficha");
    dlg.setModal(true);
    dlg.resize(900, 700);
    dlg.setStyleSheet(parent->styleSheet());

    auto* layout = new QVBoxLayout(&dlg);
    layout->setSpacing(12);
    layout->setContentsMargins(16, 16, 16, 16);

    // Título
    auto* titulo = new QLabel("📄 Pré-visualização da Ficha", &dlg);
    QFont f = titulo->font();
    f.setPointSize(f.pointSize() + 2);
    f.setBold(true);
    titulo->setFont(f);
    titulo->setStyleSheet("color: #00D4FF; padding: 8px 0;");
    layout->addWidget(titulo);

    // Scroll Area
    auto* scroll = new QScrollArea(&dlg);
    auto* textBrowser = new QTextEdit(&dlg);
    textBrowser->setReadOnly(true);

    // HTML simples para preview
    QString html = QString(R"(
<html>
<head>
<style>
body { font-family: Arial; background: white; color: black; padding: 20px; }
.header { text-align: center; border-bottom: 2px solid #333; padding-bottom: 10px; margin-bottom: 20px; }
.title { font-size: 18px; font-weight: bold; }
.section { margin: 20px 0; }
.section-title { background: #f0f0f0; padding: 8px; font-weight: bold; margin-top: 15px; }
table { width: 100%%; border-collapse: collapse; margin: 10px 0; }
th, td { border: 1px solid #666; padding: 8px; text-align: left; }
th { background: #e0e0e0; }
.footer { margin-top: 30px; text-align: center; font-size: 10px; }
</style>
</head>
<body>
<div class="header">
<div class="title">FICHA DE AVALIAÇÃO</div>
<div>%1 - Resolução Nº %2/%3</div>
<div>Curso: %4</div>
</div>

<div class="section">
<strong>Escala de Notas:</strong> %5 a %6
</div>
)")
                       .arg(ficha.tipoFicha,
                            ficha.resolucaoNum,
                            ficha.resolucaoAno,
                            ficha.curso)
                       .arg(ficha.notaMin)
                       .arg(ficha.notaMax);

    // Adiciona seções
    for (const auto& secao : ficha.secoes) {
        html += QString("<div class='section-title'>%1 - %2</div>")
        .arg(secao.identificador, secao.titulo);
        html += "<table><tr><th>Quesito</th><th>Nota</th></tr>";

        for (const auto& q : secao.quesitos) {
            html += QString("<tr><td>%1</td><td></td></tr>").arg(q.nome);
        }

        html += "</table>";
    }

    // Campos opcionais
    if (ficha.incluirDataAvaliacao)
        html += "<div class='section'>Data da Avaliação: ___/___/______</div>";
    if (ficha.incluirProfessorAvaliador)
        html += "<div class='section'>Professor Avaliador: ________________________________</div>";
    if (ficha.incluirProfessorOrientador)
        html += "<div class='section'>Professor Orientador: ________________________________</div>";
    if (!ficha.textoAprovacao.isEmpty())
        html += QString("<div class='footer'>%1</div>").arg(ficha.textoAprovacao);

    html += "</body></html>";

    textBrowser->setHtml(html);
    scroll->setWidget(textBrowser);
    scroll->setWidgetResizable(true);

    layout->addWidget(scroll);

    // Botões
    auto* btnLayout = new QHBoxLayout();
    auto* btnExportPdf = new QPushButton("📄 Exportar PDF", &dlg);
    auto* btnClose = new QPushButton("Fechar", &dlg);

    btnLayout->addWidget(btnExportPdf);
    btnLayout->addStretch();
    btnLayout->addWidget(btnClose);
    layout->addLayout(btnLayout);

    QObject::connect(btnClose, &QPushButton::clicked, &dlg, &QDialog::accept);

    QObject::connect(btnExportPdf, &QPushButton::clicked, &dlg, [&]() {
        QString filename = QFileDialog::getSaveFileName(
            &dlg,
            "Exportar para PDF",
            QString("ficha_%1.pdf").arg(ficha.tipoFicha.toLower().replace(" ", "_")),
            "Arquivos PDF (*.pdf)"
            );

        if (filename.isEmpty()) return;

        QPrinter printer(QPrinter::HighResolution);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(filename);
        printer.setPageSize(QPageSize::A4);
        printer.setPageMargins(QMarginsF(15, 15, 15, 15), QPageLayout::Millimeter);

        QTextDocument document;
        document.setHtml(html);
        document.print(&printer);

        QMessageBox::information(&dlg, "Exportar PDF",
                                 "PDF exportado com sucesso!");
    });

    dlg.exec();
}

} // namespace

// ================== CONSTRUTOR =================

PaginaFichas::PaginaFichas(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::PaginaFichas)
    , m_table(new QTableView(this))
    , m_model(new QStandardItemModel(0, 6, this))
    , m_filter(new FichaFilterModel(this))
    , m_btnNovo(new QPushButton("➕ Nova Ficha", this))
    , m_btnEditar(new QPushButton("✏️ Editar", this))
    , m_btnRemover(new QPushButton("🗑️ Excluir", this))
    , m_btnRecarregar(new QPushButton("🔄 Recarregar", this))
    , m_btnExportCsv(new QPushButton("📤 Exportar CSV", this))
    , m_btnVisualizar(new QPushButton("👁️ Visualizar", this))
    , m_btnExportPdf(new QPushButton("📄 Exportar PDF", this))
    , m_labelTotal(new QLabel(this))
    , m_editBusca(new QLineEdit(this))
    , m_comboTipo(new QComboBox(this))
{
    ui->setupUi(this);
    this->setObjectName("paginafichas");

    // ===== ESTILO (mesmo das outras páginas) =====
    this->setStyleSheet(R"(
        QWidget#paginafichas {
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
            border: 1px solid #2a3f5f;
            border-radius: 6px;
            padding: 6px 10px;    /* menor padding vertical */
            color: #FFFFFF;
            selection-background-color: #00D4FF;
        }

        QLineEdit:focus {
            border: 1px solid #00D4FF;
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

        QPushButton#btnCancel {
            background: #2a3f5f;
        }

        QPushButton#btnCancel:hover {
            background: #3a4f6f;
        }

        QLabel#labelTotalFichas {
            color: #00D4FF;
            font-size: 13px;
            font-weight: bold;
            padding: 8px 0px;
        }

        QLabel#labelBuscar, QLabel#labelTipo {
            color: #A0A0A0;
            font-weight: bold;
            font-size: 12px;
        }

        /* Diálogo / GroupBox / Checkbox / etc */
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
        QCheckBox {
            color: #E0E0E0;
            spacing: 8px;
        }
        QCheckBox::indicator {
            width: 18px;
            height: 18px;
            border-radius: 4px;
            border: 2px solid #2a3f5f;
            background: #1a2332;
        }
        QCheckBox::indicator:checked {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #00D4FF, stop:1 #0088FF);
            border: 2px solid #00D4FF;
        }
        QListWidget {
            background-color: #1a2332;
            border: 2px solid #2a3f5f;
            border-radius: 6px;
            color: #E0E0E0;
            padding: 4px;
        }
        QListWidget::item {
            padding: 8px;
            border-radius: 4px;
        }
        QListWidget::item:hover {
            background-color: rgba(0, 168, 204, 0.15);
        }
        QListWidget::item:selected {
            background-color: rgba(0, 220, 255, 0.35);
            color: #FFFFFF;
        }
        QSpinBox, QDoubleSpinBox {
            background-color: #1a2332;
            border: 1px solid #2a3f5f;
            border-radius: 6px;
            padding: 4px 6px;
            color: #FFFFFF;
        }
        QSpinBox:focus, QDoubleSpinBox:focus {
            border: 2px solid #00D4FF;
        }
        QTextEdit {
            background-color: #1a2332;
            border: 2px solid #2a3f5f;
            border-radius: 6px;
            color: #E0E0E0;
            padding: 8px;
        }
    )");

    m_btnRemover->setObjectName("btnDanger");
    m_btnRecarregar->setObjectName("btnSecondary");
    m_labelTotal->setObjectName("labelTotalFichas");

    auto* root = ui->verticalLayout;
    root->setContentsMargins(120, 40, 120, 40);
    root->setSpacing(20);

    // ===== HEADER =====
    auto* headerLayout = new QHBoxLayout();
    auto* titulo = new QLabel("📋 Fichas de Avaliação Cadastradas", this);
    titulo->setObjectName("titulo");

    headerLayout->addWidget(titulo);
    headerLayout->addStretch();
    headerLayout->addWidget(m_btnNovo);
    headerLayout->addWidget(m_btnExportCsv);
    headerLayout->addWidget(m_btnExportPdf);
    root->addLayout(headerLayout);

    // ===== FILTROS =====
    auto* filterLayout = new QHBoxLayout();
    filterLayout->setSpacing(12);

    auto* lblBuscar = new QLabel("🔍 Buscar:", this);
    lblBuscar->setObjectName("labelBuscar");
    filterLayout->addWidget(lblBuscar);

    m_editBusca->setPlaceholderText("Digite o tipo da ficha...");
    m_editBusca->setMinimumWidth(250);
    filterLayout->addWidget(m_editBusca);

    filterLayout->addSpacing(20);

    auto* lblTipo = new QLabel("📂 Tipo:", this);
    lblTipo->setObjectName("labelTipo");
    filterLayout->addWidget(lblTipo);

    m_comboTipo->addItem("Todos");
    m_comboTipo->addItem("TCC");
    m_comboTipo->addItem("Projeto Integrador");
    m_comboTipo->addItem("Estágio");
    filterLayout->addWidget(m_comboTipo);
    filterLayout->addStretch();

    root->addLayout(filterLayout);

    // ===== TABELA =====
    root->addWidget(m_table, 1);

    // ===== BOTÕES DE AÇÃO =====
    auto* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(12);
    btnLayout->addWidget(m_btnVisualizar);
    btnLayout->addWidget(m_btnEditar);
    btnLayout->addWidget(m_btnRemover);
    btnLayout->addStretch();
    btnLayout->addWidget(m_btnRecarregar);
    root->addLayout(btnLayout);

    // ===== RODAPÉ =====
    root->addWidget(m_labelTotal);

    // ===== CONFIGURAÇÃO DA TABELA =====
    m_model->setHorizontalHeaderLabels({
        "ID", "Tipo", "Resolução Nº", "Nº Seções", "Total Quesitos", "Curso"
    });

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

    auto header = m_table->horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(1, QHeaderView::Stretch);
    header->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(5, QHeaderView::ResizeToContents);

    // ===== CONEXÕES =====
    connect(m_table, &QTableView::doubleClicked,
            this, [this](const QModelIndex&) { onVisualizar(); });

    connect(m_btnNovo, &QPushButton::clicked, this, &PaginaFichas::onNovo);
    connect(m_btnEditar, &QPushButton::clicked, this, &PaginaFichas::onEditar);
    connect(m_btnRemover, &QPushButton::clicked, this, &PaginaFichas::onRemover);
    connect(m_btnRecarregar, &QPushButton::clicked, this, &PaginaFichas::onRecarregar);
    connect(m_btnExportCsv, &QPushButton::clicked, this, &PaginaFichas::onExportCsv);
    connect(m_btnVisualizar, &QPushButton::clicked, this, &PaginaFichas::onVisualizar);
    connect(m_btnExportPdf, &QPushButton::clicked, this, &PaginaFichas::onExportPdf);

    connect(m_editBusca, &QLineEdit::textChanged,
            this, &PaginaFichas::onBuscaChanged);
    connect(m_comboTipo,
            static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &PaginaFichas::onTipoChanged);

    carregarDoArquivo();
    atualizarTotal();
}

PaginaFichas::~PaginaFichas() {
    delete ui;
}

// ================== HELPERS ===================

void PaginaFichas::addFichaToTable(const Ficha& ficha) {
    int totalQuesitos = 0;
    for (const auto& secao : ficha.secoes) {
        totalQuesitos += secao.quesitos.size();
    }

    QList<QStandardItem*> row;
    auto idItem = new QStandardItem(QString::number(ficha.id));
    idItem->setEditable(false);

    row << idItem
        << new QStandardItem(ficha.tipoFicha)
        << new QStandardItem(QString("%1/%2").arg(ficha.resolucaoNum, ficha.resolucaoAno))
        << new QStandardItem(QString::number(ficha.secoes.size()))
        << new QStandardItem(QString::number(totalQuesitos))
        << new QStandardItem(ficha.curso);

    m_model->appendRow(row);
}

int PaginaFichas::selectedRow() const {
    if (!m_table->model()) return -1;
    const QModelIndex proxyIdx = m_table->currentIndex();
    if (!proxyIdx.isValid()) return -1;
    const QModelIndex srcIdx = m_filter ? m_filter->mapToSource(proxyIdx) : proxyIdx;
    return srcIdx.row();
}

// ================== SLOTS ===================

void PaginaFichas::onNovo() {
    Ficha novaFicha;
    novaFicha.id = m_nextId;

    if (!abrirDialogoFicha(this, novaFicha, false))
        return;

    novaFicha.id = m_nextId++;
    m_fichas.append(novaFicha);
    addFichaToTable(novaFicha);

    salvarNoArquivo();
    atualizarTotal();
}

void PaginaFichas::onEditar() {
    const int r = selectedRow();
    if (r < 0) {
        QMessageBox::information(this, "Editar Ficha",
                                 "Selecione uma ficha.");
        return;
    }

    if (r >= m_fichas.size()) return;

    Ficha& ficha = m_fichas[r];
    if (!abrirDialogoFicha(this, ficha, true))
        return;

    // Atualiza tabela
    m_model->item(r, 1)->setText(ficha.tipoFicha);
    m_model->item(r, 2)->setText(QString("%1/%2").arg(ficha.resolucaoNum, ficha.resolucaoAno));
    m_model->item(r, 3)->setText(QString::number(ficha.secoes.size()));

    int totalQuesitos = 0;
    for (const auto& secao : ficha.secoes) {
        totalQuesitos += secao.quesitos.size();
    }
    m_model->item(r, 4)->setText(QString::number(totalQuesitos));
    m_model->item(r, 5)->setText(ficha.curso);

    salvarNoArquivo();
    atualizarTotal();
}

void PaginaFichas::onRemover() {
    const int r = selectedRow();
    if (r < 0) {
        QMessageBox::information(this, "Excluir Ficha",
                                 "Selecione uma ficha.");
        return;
    }

    if (r >= m_fichas.size()) return;

    const Ficha& ficha = m_fichas[r];
    QString texto = QString(
                        "Ficha encontrada:\n\n"
                        "Tipo: %1\n"
                        "Resolução: %2/%3\n"
                        "Curso: %4\n\n"
                        "Esta ação não pode ser desfeita.\n\n"
                        "Deseja realmente excluir?")
                        .arg(ficha.tipoFicha, ficha.resolucaoNum,
                             ficha.resolucaoAno, ficha.curso);

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
        m_fichas.remove(r);
        m_model->removeRow(r);
        salvarNoArquivo();
        atualizarTotal();
    }
}

void PaginaFichas::onRecarregar() {
    carregarDoArquivo();
    atualizarTotal();
}

void PaginaFichas::onVisualizar() {
    const int r = selectedRow();
    if (r < 0) {
        QMessageBox::information(this, "Visualizar Ficha",
                                 "Selecione uma ficha.");
        return;
    }

    if (r >= m_fichas.size()) return;

    visualizarFicha(this, m_fichas[r]);
}

void PaginaFichas::onExportCsv() {
    QString filename = QFileDialog::getSaveFileName(
        this,
        "Exportar fichas para CSV",
        "fichas.csv",
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

    out << "ID;Tipo;ResolucaoNum;ResolucaoAno;Curso;NotaMin;NotaMax;NumSecoes\n";

    for (const auto& ficha : m_fichas) {
        out << ficha.id << ";"
            << ficha.tipoFicha << ";"
            << ficha.resolucaoNum << ";"
            << ficha.resolucaoAno << ";"
            << ficha.curso << ";"
            << ficha.notaMin << ";"
            << ficha.notaMax << ";"
            << ficha.secoes.size() << "\n";
    }

    QMessageBox::information(this, "Exportar CSV",
                             "Fichas exportadas com sucesso.");
}

void PaginaFichas::onExportPdf() {
    const int r = selectedRow();
    if (r < 0) {
        QMessageBox::information(this, "Exportar PDF",
                                 "Selecione uma ficha para exportar.");
        return;
    }

    if (r >= m_fichas.size()) return;

    const Ficha& ficha = m_fichas[r];

    QString filename = QFileDialog::getSaveFileName(
        this,
        "Exportar ficha para PDF",
        QString("ficha_%1.pdf").arg(ficha.tipoFicha.toLower().replace(" ", "_")),
        "Arquivos PDF (*.pdf)"
        );

    if (filename.isEmpty())
        return;

    // Gera HTML da ficha
    QString html = gerarHtmlFicha(ficha);

    // Cria o PDF
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(filename);
    printer.setPageSize(QPageSize::A4);
    printer.setPageMargins(QMarginsF(15, 15, 15, 15), QPageLayout::Millimeter);

    QTextDocument document;
    document.setHtml(html);
    document.print(&printer);

    QMessageBox::information(this, "Exportar PDF",
                             "PDF exportado com sucesso!");
}

// ================== HTML COMPLETO PARA PDF ===================

QString PaginaFichas::gerarHtmlFicha(const Ficha& ficha) const {
    QString html = R"(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<style>
* {
    margin: 0;
    padding: 0;
    box-sizing: border-box;
}

body {
    font-family: 'Arial', sans-serif;
    background: white;
    color: #000;
    padding: 20px;
    font-size: 11pt;
}

.header {
    text-align: center;
    border-bottom: 3px solid #000;
    padding-bottom: 15px;
    margin-bottom: 25px;
}

.logo {
    font-size: 20pt;
    font-weight: bold;
    color: #000;
    margin-bottom: 10px;
}

.title {
    font-size: 16pt;
    font-weight: bold;
    margin: 10px 0;
    text-transform: uppercase;
}

.subtitle {
    font-size: 10pt;
    margin: 5px 0;
}

.info-box {
    background: #f5f5f5;
    border: 1px solid #ccc;
    padding: 15px;
    margin: 20px 0;
    border-radius: 5px;
}

.info-row {
    margin: 8px 0;
}

.info-label {
    font-weight: bold;
    display: inline-block;
    width: 180px;
}

.section {
    margin: 25px 0;
    page-break-inside: avoid;
}

.section-header {
    background: #e0e0e0;
    padding: 10px;
    font-weight: bold;
    font-size: 12pt;
    margin: 15px 0 10px 0;
    border-left: 4px solid #000;
}

table {
    width: 100%;
    border-collapse: collapse;
    margin: 15px 0;
}

th {
    background: #d0d0d0;
    border: 1px solid #666;
    padding: 10px;
    text-align: left;
    font-weight: bold;
    font-size: 10pt;
}

td {
    border: 1px solid #999;
    padding: 10px;
    text-align: left;
    font-size: 10pt;
    vertical-align: top;
}

.quesito-nome {
    font-weight: normal;
}

.quesito-auto {
    font-style: italic;
    color: #666;
}

.nota-cell {
    width: 100px;
    text-align: center;
    background: #fafafa;
}

.campos-preenchimento {
    margin: 30px 0;
}

.campo-linha {
    margin: 15px 0;
    padding: 10px 0;
    border-bottom: 1px solid #ccc;
}

.campo-label {
    font-weight: bold;
    display: inline-block;
    margin-right: 10px;
}

.campo-underline {
    display: inline-block;
    border-bottom: 1px solid #000;
    width: 400px;
    height: 20px;
}

.footer {
    margin-top: 40px;
    padding-top: 20px;
    border-top: 2px solid #000;
    text-align: center;
    font-size: 9pt;
}

.observacoes-box {
    margin: 20px 0;
    border: 1px solid #999;
    min-height: 100px;
    padding: 10px;
}

.escala-notas {
    background: #f9f9f9;
    border: 1px solid #ddd;
    padding: 10px;
    margin: 15px 0;
    text-align: center;
    font-weight: bold;
}

@media print {
    body {
        padding: 10mm;
    }

    .section {
        page-break-inside: avoid;
    }
}
</style>
</head>
<body>
)";

    // ===== CABEÇALHO =====
    html += R"(
<div class="header">
    <div class="logo"> FUCAPI - FUNDAÇÃO CENTRO DE ANÁLISE, PESQUISA E INOVAÇÃO</div>
    <div class="title">FICHA DE AVALIAÇÃO</div>
)";

    if (!ficha.resolucaoNum.isEmpty()) {
        html += QString("<div class='subtitle'>RESOLUÇÃO Nº %1/%2</div>")
                    .arg(ficha.resolucaoNum, ficha.resolucaoAno);
    }

    html += "</div>";

    // ===== DADOS DE IDENTIFICAÇÃO =====
    html += R"(<div class="info-box">)";
    html += QString("<div class='info-row'><span class='info-label'>Tipo da Ficha:</span> %1</div>")
                .arg(ficha.tipoFicha);

    if (!ficha.curso.isEmpty()) {
        html += QString("<div class='info-row'><span class='info-label'>Curso:</span> %1</div>")
        .arg(ficha.curso);
    }

    html += R"(
    <div class='info-row'><span class='info-label'>Nome do Aluno(a):</span> <span class='campo-underline'></span></div>
    <div class='info-row'><span class='info-label'>Título do Trabalho:</span> <span class='campo-underline'></span></div>
)";

    if (ficha.incluirProfessorOrientador) {
        html += R"(<div class='info-row'><span class='info-label'>Professor Orientador:</span> <span class='campo-underline'></span></div>)";
    }

    html += "</div>";

    // ===== ESCALA DE NOTAS =====
    html += QString("<div class='escala-notas'>ESCALA DE NOTAS: %1 a %2</div>")
                .arg(ficha.notaMin).arg(ficha.notaMax);

    // ===== SEÇÕES DE AVALIAÇÃO =====
    for (const auto& secao : ficha.secoes) {
        html += QString("<div class='section'>");
        html += QString("<div class='section-header'>%1 - %2</div>")
                    .arg(secao.identificador, secao.titulo);

        if (!secao.quesitos.isEmpty()) {
            html += "<table>";
            html += "<tr><th class='quesito-nome'>Quesito</th><th class='nota-cell'>Nota</th></tr>";

            for (const auto& quesito : secao.quesitos) {
                QString nomeQuesito = quesito.nome;

                if (quesito.autoCalculado) {
                    nomeQuesito += " <span class='quesito-auto'>[AUTO-CALCULADO]</span>";
                }

                if (quesito.temPeso && quesito.peso != 1.0) {
                    nomeQuesito += QString(" <span class='quesito-auto'>(Peso: %1)</span>")
                    .arg(quesito.peso);
                }

                html += QString("<tr><td class='quesito-nome'>%1</td><td class='nota-cell'></td></tr>")
                            .arg(nomeQuesito);
            }

            html += "</table>";
        } else {
            html += "<p><i>Nenhum quesito cadastrado para esta seção.</i></p>";
        }

        html += "</div>";
    }

    // ===== CAMPOS DE PREENCHIMENTO =====
    html += "<div class='campos-preenchimento'>";

    if (ficha.incluirDataAvaliacao) {
        html += R"(
        <div class='campo-linha'>
            <span class='campo-label'>Data da Avaliação:</span>
            _____ / _____ / __________
        </div>)";
    }

    if (ficha.incluirProfessorAvaliador) {
        html += R"(
        <div class='campo-linha'>
            <span class='campo-label'>Professor Avaliador:</span>
            <span class='campo-underline'></span>
        </div>)";
    }

    if (ficha.incluirObservacoes) {
        html += R"(
        <div class='campo-linha'>
            <span class='campo-label'>Observações / Comentários:</span>
            <div class='observacoes-box'></div>
        </div>)";
    }

    html += "</div>";

    // ===== RODAPÉ =====
    if (!ficha.textoAprovacao.isEmpty()) {
        html += QString("<div class='footer'>%1</div>").arg(ficha.textoAprovacao);
    }

    html += R"(
</body>
</html>
)";

    return html;
}

// ================== PERSISTÊNCIA ===================

QString PaginaFichas::fichaParaString(const Ficha& f) const {
    QStringList parts;
    parts << QString::number(f.id);
    parts << f.tipoFicha;
    parts << f.resolucaoNum;
    parts << f.resolucaoAno;
    parts << f.curso;
    parts << f.categoriaCurso;
    parts << QString::number(f.notaMin);
    parts << QString::number(f.notaMax);
    parts << QString::number(f.secoes.size());
    parts << (f.incluirDataAvaliacao ? "1" : "0");
    parts << (f.incluirProfessorAvaliador ? "1" : "0");
    parts << (f.incluirProfessorOrientador ? "1" : "0");
    parts << (f.incluirObservacoes ? "1" : "0");
    parts << f.textoAprovacao;

    // TODO: Serializar seções completas (quesitos, etc)

    return parts.join(";");
}

Ficha PaginaFichas::stringParaFicha(const QString& linha) const {
    Ficha f;
    const QStringList p = linha.split(';');

    if (p.size() < 14) return f;

    f.id = p[0].toInt();
    f.tipoFicha = p[1];
    f.resolucaoNum = p[2];
    f.resolucaoAno = p[3];
    f.curso = p[4];
    f.categoriaCurso = p[5];
    f.notaMin = p[6].toDouble();
    f.notaMax = p[7].toDouble();
    // p[8] é numSeções (será reconstruído)
    f.incluirDataAvaliacao = (p[9] == "1");
    f.incluirProfessorAvaliador = (p[10] == "1");
    f.incluirProfessorOrientador = (p[11] == "1");
    f.incluirObservacoes = (p[12] == "1");
    f.textoAprovacao = p[13];

    // TODO: Deserializar seções completas

    return f;
}

bool PaginaFichas::salvarNoArquivo() const {
    QFile f(m_arquivo);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(nullptr, "Salvar Fichas",
                             "Não foi possível abrir o arquivo para escrita.");
        return false;
    }

    QTextStream out(&f);
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    out.setCodec("UTF-8");
#endif

    for (const auto& ficha : m_fichas) {
        out << fichaParaString(ficha) << '\n';
    }

    return true;
}

bool PaginaFichas::carregarDoArquivo() {
    QFile f(m_arquivo);
    if (!f.exists()) {
        m_fichas.clear();
        m_model->removeRows(0, m_model->rowCount());
        m_nextId = 1;
        return true;
    }

    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(nullptr, "Carregar Fichas",
                             "Não foi possível abrir o arquivo para leitura.");
        return false;
    }

    m_fichas.clear();
    m_model->removeRows(0, m_model->rowCount());

    QTextStream in(&f);
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    in.setCodec("UTF-8");
#endif

    while (!in.atEnd()) {
        const QString line = in.readLine();
        if (line.trimmed().isEmpty()) continue;

        Ficha ficha = stringParaFicha(line);
        if (ficha.id > 0) {
            m_fichas.append(ficha);
            addFichaToTable(ficha);
        }
    }

    recomputarNextId();
    return true;
}

void PaginaFichas::recomputarNextId() {
    int maxId = 0;
    for (const auto& ficha : m_fichas) {
        if (ficha.id > maxId) maxId = ficha.id;
    }
    m_nextId = maxId + 1;
}

// ================== FILTROS ===================

void PaginaFichas::onBuscaChanged(const QString& texto) {
    if (m_filter) {
        m_filter->setNomeFiltro(texto);
        atualizarTotal();
    }
}

void PaginaFichas::onTipoChanged(int index) {
    if (!m_filter) return;

    QString filtro;
    if (index == 1) filtro = "TCC";
    else if (index == 2) filtro = "Projeto Integrador";
    else if (index == 3) filtro = "Estágio";
    else filtro.clear();

    m_filter->setTipoFiltro(filtro);
    atualizarTotal();
}

void PaginaFichas::atualizarTotal() {
    const int total = m_filter ? m_filter->rowCount() : m_model->rowCount();
    if (total == 1)
        m_labelTotal->setText("📊 1 ficha encontrada");
    else
        m_labelTotal->setText(QString("📊 %1 fichas encontradas").arg(total));
}
