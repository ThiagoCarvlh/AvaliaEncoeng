#pragma once
#include <QWidget>
#include <QString>
#include <QVector>

class QTableView;
class QStandardItemModel;
class QPushButton;
class QLabel;

namespace Ui { class PaginaNotas; }

class PaginaNotas : public QWidget {
    Q_OBJECT
public:
    explicit PaginaNotas(QWidget* parent = nullptr);
    ~PaginaNotas();

    // Chamado pela JanelaPrincipal depois do login (modo avaliador)
    // Se não chamar ou passar cpf vazio -> modo admin
    void setAvaliador(const QString& cpf,
                      const QString& nome,
                      const QString& curso);

private slots:
    void onNovo();
    void onEditar();
    void onRemover();
    void onRecarregar();
    void onExportCsv();   // ✅ novo slot para exportar CSV

private:
    Ui::PaginaNotas*    ui{};
    QTableView*         m_table{};
    QStandardItemModel* m_model{};
    QPushButton *m_btnNovo{},
        *m_btnEditar{},
        *m_btnRemover{},
        *m_btnRecarregar{},
        *m_btnExportCsv{};  // ✅ novo botão
    QLabel*             m_labelTotal{};

    // Contexto do usuário logado
    QString m_cpfLogado;
    QString m_nomeLogado;
    QString m_cursoLogado;
    bool    m_modoAvaliador{false};

    // Estrutura interna de nota
    struct Nota {
        int     idNota{0};
        int     idProjeto{0};
        int     idFicha{0};      // ✅ vínculo com a ficha
        QString cpfAvaliador;
        QString nomeAvaliador;
        double  notaFinal{0.0};
    };

    QVector<Nota> m_notas;
    int           m_nextId{1};

    const QString m_arquivoNotas    = "notas.csv";
    const QString m_arquivoProjetos = "projetos.txt";
    const QString m_arquivoVinculos = "vinculos_projetos.csv";

    // Configuração de UI/estilo
    void configurarUi();
    void configurarTabelaAdmin();
    void configurarTabelaAvaliador();
    void atualizarTotalLabel(int total);

    // Persistência
    bool carregarNotasDoArquivo();
    bool salvarNotasNoArquivo() const;
    void recomputarNextId();

    // Preenchimento da tabela
    void recarregarDados();
    void preencherTabelaAdmin();
    void preencherTabelaAvaliador();

    // Helpers
    int  selectedRow() const;
    Nota* encontrarNotaPorId(int idNota);
    Nota* encontrarNotaDoAvaliador(int idProjeto, const QString& cpf);
};
