#pragma once

#include <QWidget>
#include <QString>
#include <QVector>

// Forward declarations
class QTableView;
class QStandardItemModel;
class QPushButton;
class QLabel;
class QLineEdit;
class QComboBox;
class FichaFilterModel;

namespace Ui {
class PaginaFichas;
}

// ===== Estruturas de Dados =====

struct Quesito {
    QString nome;
    double notaMin{0.0};
    double notaMax{10.0};
    bool temPeso{false};
    double peso{1.0};
    bool autoCalculado{false};
    QString formula; // "MEDIA", "SOMA", etc
    int ordem{0};
};

struct SecaoAvaliacao {
    QString identificador;  // "I", "II", "A", "B"
    QString titulo;
    QString tipoCalculo;    // "media", "ponderada", "soma", "manual"
    double pesoSecao{1.0};
    QVector<Quesito> quesitos;
};

struct Ficha {
    int id{0};
    QString tipoFicha;           // "TCC", "Projeto Integrador"
    QString resolucaoNum;
    QString resolucaoAno;
    QString curso;
    QString categoriaCurso;      // "Técnico - Mecânica"
    double notaMin{0.0};
    double notaMax{10.0};
    QVector<SecaoAvaliacao> secoes;

    // Campos opcionais
    bool incluirDataAvaliacao{true};
    bool incluirProfessorAvaliador{true};
    bool incluirProfessorOrientador{false};
    bool incluirObservacoes{false};
    QString textoAprovacao;
};

// ===== Classe Principal =====

class PaginaFichas : public QWidget
{
    Q_OBJECT

public:
    explicit PaginaFichas(QWidget* parent = nullptr);
    ~PaginaFichas();

private slots:
    void onNovo();
    void onEditar();
    void onRemover();
    void onRecarregar();
    void onExportCsv();
    void onVisualizar();
    void onExportPdf();             // ✅ ADICIONADO

    // Filtros
    void onBuscaChanged(const QString& texto);
    void onTipoChanged(int index);

private:
    // Métodos privados
    bool salvarNoArquivo() const;
    bool carregarDoArquivo();
    void recomputarNextId();
    void addFichaToTable(const Ficha& ficha);
    int  selectedRow() const;
    void atualizarTotal();

    // Conversão Ficha <-> String (para CSV)
    QString fichaParaString(const Ficha& f) const;
    Ficha stringParaFicha(const QString& linha) const;

    // HTML para PDF
    QString gerarHtmlFicha(const Ficha& ficha) const;   // ✅ ADICIONADO

    // Membros da UI
    Ui::PaginaFichas* ui;

    // Modelo e Visão
    QTableView*         m_table{};
    QStandardItemModel* m_model{};
    FichaFilterModel*   m_filter{};

    // Widgets
    QPushButton* m_btnNovo{};
    QPushButton* m_btnEditar{};
    QPushButton* m_btnRemover{};
    QPushButton* m_btnRecarregar{};
    QPushButton* m_btnExportCsv{};
    QPushButton* m_btnVisualizar{};
    QPushButton* m_btnExportPdf{};     // ✅ ADICIONADO
    QLabel*      m_labelTotal{};
    QLineEdit*   m_editBusca{};
    QComboBox*   m_comboTipo{};

    // Dados
    int m_nextId{1};
    QVector<Ficha> m_fichas; // Armazena as fichas completas
    const QString m_arquivo = "fichas.txt";
};
